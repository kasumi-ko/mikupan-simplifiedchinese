#include "libcdvd.h"

#include "mikupan/io/mikupan_file.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <mutex>
#include <string>
#include <vector>

#define MIKUPAN_CD_HD_LSN_BASE 0x70000000u
#define MIKUPAN_CD_FILE_LSN_BASE 0x71000000u
constexpr u_int kSectorSize = 2048;

struct MikuPanCdFile {
    u_int lsn;
    std::string path;
};

static std::mutex cd_files_mutex;
static std::vector<MikuPanCdFile> cd_files;
static u_int cd_next_file_lsn = MIKUPAN_CD_FILE_LSN_BASE;
static int cd_last_error;
/* Opened through SDL_IOFromFile so the emulated CD can read paths that are not
 * plain filesystem paths -- notably Android content:// URIs and assets:// asset
 * paths. stdio fopen() cannot open those, which previously made every sceCdRead
 * fail on Android and hung ICdvdInitOnce in its load-retry loop. */
static SDL_IOStream* cd_stream_file;

static u_char DecToBcd(u_char dec)
{
    return (u_char)(((dec / 10) << 4) | (dec % 10));
}

static int ResolveDataFile(const char* name, char* out, size_t out_size)
{
    if (!MikuPan_ResolveCdPath(name, out, out_size)) {
        return 0;
    }

    return out[0] != '\0';
}

static int ReadStreamBytes(SDL_IOStream* io, Sint64 offset, void* buf,
                           size_t size, bool round_to_sector)
{
    if (io == NULL) {
        cd_last_error = 1;
        return 0;
    }

    if (offset != 0 && SDL_SeekIO(io, (Sint64)offset, SDL_IO_SEEK_SET) < 0) {
        SDL_CloseIO(io);
        cd_last_error = 1;
        return 0;
    }

    size_t payload = size;
    if (round_to_sector)
    {
        const Sint64 length = SDL_GetIOSize(io);
        if (length < offset)
        {
            SDL_CloseIO(io);
            cd_last_error = 1;
            return 0;
        }

        const uint64_t available = length - offset;
        const uint64_t padding = (kSectorSize - length % kSectorSize) % kSectorSize;
        if (size > available + padding)
        {
            SDL_CloseIO(io);
            cd_last_error = 1;
            return 0;
        }
        if (size > available)
        {
            payload = static_cast<size_t>(available);
        }
    }

    u_char* dst = (u_char*)buf;
    size_t remaining = payload;
    while (remaining > 0) {
        size_t got = SDL_ReadIO(io, dst, remaining);
        if (got == 0) {
            break;
        }
        dst += got;
        remaining -= got;
    }

    SDL_CloseIO(io);
    if (remaining != 0)
    {
        cd_last_error = 1;
        return 0;
    }

    if (payload < size)
    {
        memset(dst, 0, size - payload);
    }
    cd_last_error = 0;
    return 1;
}

static int ReadFileBytes(const char* path, Sint64 offset, void* buf,
                         size_t size, bool round_to_sector = false)
{
    return ReadStreamBytes(SDL_IOFromFile(path, "rb"), offset, buf, size,
                           round_to_sector);
}

static std::string FindFileByLsn(u_int lsn)
{
    std::lock_guard lock(cd_files_mutex);
    for (const auto& file : cd_files) {
        if (file.lsn == lsn) {
            return file.path;
        }
    }

    return {};
}

static u_int MapLooseFile(const char* path)
{
    std::lock_guard lock(cd_files_mutex);
    for (const auto& file : cd_files) {
        if (file.path == path) {
            return file.lsn;
        }
    }

    const u_int lsn = cd_next_file_lsn++;
    cd_files.push_back({lsn, path});
    return lsn;
}

int sceCdInit(int init_mode)
{
    (void)init_mode;
    return 1;
}

int sceCdMmode(int media)
{
    (void)media;
    return 1;
}

int sceCdReadClock(sceCdCLOCK* rtc)
{
    SDL_Time now;
    SDL_DateTime dt;
    int year;

    if (!SDL_GetCurrentTime(&now) || !SDL_TimeToDateTime(now, &dt, true)) {
        rtc->stat = 0x80;
        return 0;
    }

    year = dt.year - 2000;
    if (year < 0)
        year = 0;
    else if (year > 99)
        year = 99;

    rtc->stat = 0;
    rtc->second = DecToBcd((u_char)dt.second);
    rtc->minute = DecToBcd((u_char)dt.minute);
    rtc->hour = DecToBcd((u_char)dt.hour);
    rtc->pad = 0;
    rtc->day = DecToBcd((u_char)dt.day);
    rtc->month = DecToBcd((u_char)dt.month);
    rtc->year = DecToBcd((u_char)year);

    return 1;
}

int sceCdRead(u_int lsn, u_int sectors, void* buf, sceCdRMode* mode)
{
    char path[1024];
    const uint64_t bytes = uint64_t(sectors) * kSectorSize;

    (void)mode;

    if (buf == NULL || bytes > SIZE_MAX)
    {
        cd_last_error = 1;
        return 0;
    }

    if (lsn >= MIKUPAN_CD_HD_LSN_BASE && lsn < MIKUPAN_CD_FILE_LSN_BASE) 
    {
        const Sint64 offset = Sint64(lsn - MIKUPAN_CD_HD_LSN_BASE) * kSectorSize;
        auto* index = static_cast<SDL_IOStream*>(MikuPan_OpenDataFile("IMG_HD.BIN"));
        return ReadStreamBytes(index, offset, buf, static_cast<size_t>(bytes), true);
    }

    if (lsn >= MIKUPAN_CD_FILE_LSN_BASE)
    {
        const auto file = FindFileByLsn(lsn);
        if (file.empty())
        {
            cd_last_error = 1;
            return 0;
        }
        return ReadFileBytes(file.c_str(), 0, buf, static_cast<size_t>(bytes), true);
    }

    if (!ResolveDataFile("\\IMG_BD.BIN;1", path, sizeof(path))) 
    {
        cd_last_error = 1;
        return 0;
    }

    return ReadFileBytes(path, Sint64(lsn) * kSectorSize, buf,
                         static_cast<size_t>(bytes), true);
}

int sceCdReadFile(const char* name, u_int offset, void* buf, u_int size)
{
    char path[1024];

    if (buf == NULL || name == NULL || !ResolveDataFile(name, path, sizeof(path)))
    {
        cd_last_error = 1;
        return 0;
    }

    return ReadFileBytes(path, offset, buf, size);
}

int sceCdSync(int mode)
{
    (void)mode;
    return 0;
}

int sceCdBreak(void)
{
    return 1;
}

int sceCdGetError(void)
{
    return cd_last_error;
}

int sceCdStRead(u_int size, u_int* buf, u_int mode, u_int* err)
{
    (void)mode;

    if (err != NULL) {
        *err = 0;
    }

    if (cd_stream_file == NULL || buf == NULL) {
        return 0;
    }

    /* Mirror fread(buf, 0x800, size, ...): fill as much as possible and report
     * the number of complete 0x800-byte sectors read. */
    u_char* dst = (u_char*)buf;
    size_t want = (size_t)size * 0x800u;
    size_t total = 0;
    while (total < want) {
        size_t got = SDL_ReadIO(cd_stream_file, dst + total, want - total);
        if (got == 0) {
            break;
        }
        total += got;
    }

    return (int)(total / 0x800u);
}

int sceCdStStop()
{
    if (cd_stream_file != NULL) {
        SDL_CloseIO(cd_stream_file);
        cd_stream_file = NULL;
    }

    return 1;
}

int sceCdDiskReady(int mode)
{
    (void)mode;
    return 2;
}

int sceCdStInit(u_int bufmax, u_int bankmax, u_int iop_bufaddr)
{
    (void)bufmax;
    (void)bankmax;
    (void)iop_bufaddr;
    return 1;
}

int sceCdSearchFile(sceCdlFILE* fp, const char* name)
{
    char path[1024];
    u_int size;
    u_int lsn;

    if (fp == NULL || name == NULL || !ResolveDataFile(name, path, sizeof(path))) 
    {
        return 0;
    }

    size = MikuPan_GetFileSize(path);

    if (size == 0) 
    {
        return 0;
    }

    if (strstr(path, "IMG_HD.BIN") != NULL || strstr(path, "img_hd.bin") != NULL) 
    {
        lsn = MIKUPAN_CD_HD_LSN_BASE;
        if (MikuPan_IsJapaneseGameData())
        {
            size = MikuPan_GetDataFileSize("IMG_HD.BIN");
        }
    } 
    else if (strstr(path, "IMG_BD.BIN") != NULL || strstr(path, "img_bd.bin") != NULL) 
    {
        lsn = 0;
    } 
    else
    {
        lsn = MapLooseFile(path);
    }

    memset(fp, 0, sizeof(*fp));
    fp->lsn = lsn;
    fp->size = size;
    strncpy(fp->name, name, sizeof(fp->name) - 1);

    return 1;
}

int sceCdStStart(u_int lbn, sceCdRMode* mode)
{
    (void)mode;

    sceCdStStop();

    if (lbn == MIKUPAN_CD_HD_LSN_BASE || lbn == 0)
    {
        const char* name = lbn == 0 ? "IMG_BD.BIN" : "IMG_HD.BIN";
        cd_stream_file = static_cast<SDL_IOStream*>(MikuPan_OpenDataFile(name));
        return cd_stream_file != NULL;
    }

    const auto file = FindFileByLsn(lbn);
    if (file.empty())
    {
        return 0;
    }

    cd_stream_file = SDL_IOFromFile(file.c_str(), "rb");
    return cd_stream_file != NULL;
}
