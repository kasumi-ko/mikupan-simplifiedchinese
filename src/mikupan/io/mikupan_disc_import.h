#pragma once

#include <filesystem>
#include <string>

// Copies the supported Japanese disc into the provided game data directory.
// Existing byte-identical files are reused and conflicting files are preserved.
bool MikuPan_ImportDiscIso(const std::filesystem::path& iso,
                          const std::filesystem::path& binary,
                          std::string& error);
