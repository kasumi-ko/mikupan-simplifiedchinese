#include "mikupan_disc_import.h"

#include <archive.h>
#include <archive_entry.h>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
namespace fs = std::filesystem;
constexpr uint64_t SectorSize = 2048;
constexpr size_t IndexSize = 1850 * 8;
using Reader = std::unique_ptr<archive, decltype(&archive_read_free)>;

void CheckArchive(archive* reader, int result)
{
    if (result != ARCHIVE_OK)
    {
        const char* detail = archive_error_string(reader);
        throw std::runtime_error(detail ? detail : "Could not read the ISO.");
    }
}

Reader OpenIso(const fs::path& iso)
{
    Reader reader(archive_read_new(), archive_read_free);
    if (!reader)
        throw std::runtime_error("Could not allocate the ISO reader.");
    CheckArchive(reader.get(), archive_read_support_filter_none(reader.get()));
    CheckArchive(reader.get(), archive_read_support_format_iso9660(reader.get()));
    CheckArchive(reader.get(), archive_read_set_options(reader.get(), "iso9660:!rockridge,iso9660:!joliet"));
    CheckArchive(reader.get(), archive_read_open_filename_w(reader.get(), iso.c_str(), 64 * 1024));
    return reader;
}

fs::path DiscPath(const char* name)
{
    if (!name)
        throw std::runtime_error("Missing ISO file name.");

    std::string canonical(name);
    for (char& ch : canonical)
    {
        if (ch >= 'a' && ch <= 'z')
            ch = char(ch - 'a' + 'A');
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')
              || ch == '/' || ch == '.' || ch == '_' || ch == '-'))
            throw std::runtime_error("Unsupported ISO file name.");
    }

    const fs::path path(canonical);
    if (path.empty() || path.has_root_path() || path.filename().empty())
        throw std::runtime_error("Invalid ISO file path.");
    for (const auto& component : path)
    {
        const auto part = component.string();
        if (part.empty() || part.back() == '.')
            throw std::runtime_error("Invalid ISO file name.");
        const auto stem = part.substr(0, part.find('.'));
        if (stem == "AUX" || stem == "CON" || stem == "NUL" || stem == "PRN"
            || (stem.size() == 4 && stem.back() >= '0' && stem.back() <= '9'
                && (stem.starts_with("COM") || stem.starts_with("LPT"))))
            throw std::runtime_error("Reserved Windows file name in the ISO.");
    }
    return path;
}

void CheckNoLink(const fs::path& path)
{
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        throw std::runtime_error("The game data path contains a Windows junction or symbolic link.");
}

void CheckBootConfig(std::string_view config)
{
    const auto trim = [](std::string_view text) {
        const auto first = text.find_first_not_of(" \t\r");
        return first == text.npos ? std::string_view{}
            : text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
    };
    while (!config.empty())
    {
        const auto end = config.find('\n');
        const auto line = config.substr(0, end);
        const auto assignment = line.find('=');
        if (assignment != line.npos && trim(line.substr(0, assignment)) == "BOOT2"
            && trim(line.substr(assignment + 1)) == "cdrom0:\\SLPS_250.74;1")
            return;
        if (end == config.npos)
            break;
        config.remove_prefix(end + 1);
    }
    throw std::runtime_error("The ISO does not boot Japanese SLPS_250.74.");
}

void CheckIsoRegion(const fs::path& iso)
{
    auto reader = OpenIso(iso);
    archive_entry* entry;
    int status;
    while ((status = archive_read_next_header(reader.get(), &entry)) == ARCHIVE_OK)
    {
        const char* name = archive_entry_pathname(entry);
        if (name && std::string_view(name) == "SYSTEM.CNF")
        {
            std::array<char, 4096> config;
            const auto size = archive_entry_size(entry);
            if (archive_entry_filetype(entry) != AE_IFREG || size <= 0
                || uint64_t(size) > config.size())
                throw std::runtime_error("Invalid SYSTEM.CNF.");
            const auto read = archive_read_data(reader.get(), config.data(), size_t(size));
            if (read != size)
                throw std::runtime_error("Truncated SYSTEM.CNF.");
            CheckBootConfig({config.data(), size_t(read)});
            return;
        }
        CheckArchive(reader.get(), archive_read_data_skip(reader.get()));
    }
    if (status != ARCHIVE_EOF)
        CheckArchive(reader.get(), status);
    throw std::runtime_error("The ISO is missing SYSTEM.CNF.");
}

void Extract(archive* reader, const fs::path& target, uint64_t remaining)
{
    fs::create_directories(target.parent_path());
    std::ofstream out(target, std::ios::binary);
    if (!out)
        throw std::runtime_error("Could not create a staged game data file.");
    std::array<char, 64 * 1024> buffer;
    while (remaining)
    {
        const auto wanted = size_t(std::min<uint64_t>(remaining, buffer.size()));
        const auto count = archive_read_data(reader, buffer.data(), wanted);
        if (count <= 0 || uint64_t(count) > remaining)
            throw std::runtime_error("Truncated ISO file data.");
        out.write(buffer.data(), count);
        if (!out)
            throw std::runtime_error("Could not write game data. Check available disk space.");
        remaining -= uint64_t(count);
    }
    out.close();
    if (!out)
        throw std::runtime_error("Could not finish writing game data.");
}

uint32_t ReadLE32(const uint8_t* bytes)
{
    return uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8)
        | (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
}

void ValidateGameData(const fs::path& stage, const std::set<fs::path>& files)
{
    for (const auto* required : {"SYSTEM.CNF", "SLPS_250.74", "IMG_HD.BIN", "IMG_BD.BIN",
                                 "MOVIE/SCN0010.PSS"})
    {
        if (!files.contains(fs::path(required)))
            throw std::runtime_error("The SLPS_250.74 ISO is missing a required game file.");
    }

    const auto index = stage / "IMG_HD.BIN";
    const auto archive_size = fs::file_size(stage / "IMG_BD.BIN");
    if (archive_size == 0 || fs::file_size(index) != IndexSize)
        throw std::runtime_error("The ISO does not have the 1,850-entry Japanese archive index.");

    std::array<uint8_t, IndexSize> bytes;
    std::ifstream in(index, std::ios::binary);
    if (!in.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
        throw std::runtime_error("Could not read IMG_HD.BIN.");
    for (size_t i = 0; i < bytes.size(); i += 8)
    {
        const auto offset = uint64_t(ReadLE32(bytes.data() + i)) * SectorSize;
        if (offset > archive_size || ReadLE32(bytes.data() + i + 4) > archive_size - offset)
            throw std::runtime_error("An IMG_HD.BIN entry lies outside IMG_BD.BIN.");
    }
}

bool SameFile(const fs::path& source, const fs::path& destination)
{
    if (!fs::is_regular_file(destination) || fs::file_size(source) != fs::file_size(destination))
        return false;
    std::ifstream left(source, std::ios::binary);
    std::ifstream right(destination, std::ios::binary);
    std::array<char, 64 * 1024> first, second;
    for (uint64_t remaining = fs::file_size(source); remaining;)
    {
        const auto count = size_t(std::min<uint64_t>(remaining, first.size()));
        if (!left.read(first.data(), count) || !right.read(second.data(), count)
            || !std::equal(first.begin(), first.begin() + count, second.begin()))
            return false;
        remaining -= count;
    }
    return true;
}

void Publish(const fs::path& stage, const fs::path& binary, const std::set<fs::path>& files)
{
    std::vector<fs::path> remaining;
    for (const auto& file : files)
    {
        for (auto part = file; !part.empty(); part = part.parent_path())
            CheckNoLink(binary / part);
        if (!fs::exists(binary / file))
            remaining.push_back(file);
        else if (!SameFile(stage / file, binary / file))
            throw std::runtime_error("binary already contains a different " + file.generic_string()
                                     + ". The existing files were preserved.");
    }

    // Make the archive pair visible after the other disc files.
    std::stable_partition(remaining.begin(), remaining.end(), [](const fs::path& file) {
        return file != "IMG_HD.BIN" && file != "IMG_BD.BIN";
    });
    size_t moved = 0;
    try
    {
        for (const auto& file : remaining)
        {
            const auto destination = binary / file;
            fs::create_directories(destination.parent_path());
            if (!MoveFileW((stage / file).c_str(), destination.c_str()))
                throw std::runtime_error("Could not move game files into binary. Check folder access.");
            ++moved;
        }
    }
    catch (...)
    {
        while (moved)
            DeleteFileW((binary / remaining[--moved]).c_str());
        throw;
    }
}
}

bool MikuPan_ImportDiscIso(const fs::path& iso, const fs::path& binary, std::string& error)
{
    try
    {
        CheckIsoRegion(iso);
        auto reader = OpenIso(iso);

        CheckNoLink(binary);
        fs::create_directories(binary);
        std::unique_ptr<void, decltype(&CloseHandle)> lock(
            CreateFileW((binary / ".mikupan-import.lock").c_str(), GENERIC_WRITE | DELETE, 0, nullptr,
                        CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr),
            CloseHandle);
        if (lock.get() == INVALID_HANDLE_VALUE)
        {
            lock.release();
            throw std::runtime_error("Could not open binary for import. Check folder access or an active import.");
        }

        const auto stage = binary / (".mikupan-iso-" + std::to_string(GetCurrentProcessId())
                                     + "-" + std::to_string(GetTickCount64()));
        if (!fs::create_directory(stage))
            throw std::runtime_error("Could not create the temporary game data directory.");
        struct TemporaryDirectory
        {
            fs::path path;
            ~TemporaryDirectory()
            {
                std::error_code ec;
                fs::remove_all(path, ec);
            }
        } temporary{stage};

        std::set<fs::path> files;
        archive_entry* entry;
        int status;
        while ((status = archive_read_next_header(reader.get(), &entry)) == ARCHIVE_OK)
        {
            const char* name = archive_entry_pathname(entry);
            const auto type = archive_entry_filetype(entry);
            if (type == AE_IFDIR)
                continue;
            const auto size = archive_entry_size(entry);
            const auto file = DiscPath(name);
            if (type != AE_IFREG || archive_entry_hardlink(entry) || size < 0
                || !files.insert(file).second)
                throw std::runtime_error("The ISO contains an unsupported or duplicate file entry.");
            Extract(reader.get(), stage / file, uint64_t(size));
            if (file == "SYSTEM.CNF")
            {
                if (size > 4096)
                    throw std::runtime_error("Invalid SYSTEM.CNF.");
                std::ifstream in(stage / file, std::ios::binary);
                CheckBootConfig(std::string((std::istreambuf_iterator<char>(in)), {}));
            }
        }
        if (status != ARCHIVE_EOF)
            CheckArchive(reader.get(), status);
        ValidateGameData(stage, files);
        Publish(stage, binary, files);
        error.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}
