#pragma once

#include <filesystem>
#include <windows.h>

namespace config
{
    [[nodiscard]] static std::filesystem::path get_executable_dir() 
    {
    #ifdef _WIN32
        char buffer[260];
        GetModuleFileName(nullptr, buffer, 260);
        return std::filesystem::path(buffer).parent_path();
    #else
        return std::filesystem::canonical("/proc/self/exe").parent_path();
    #endif
    }
}