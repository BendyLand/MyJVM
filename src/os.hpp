#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__CYGWIN__)
    #define OS_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
    #define OS_MACOS
#elif defined(__linux__)
    #define OS_LINUX
#elif defined(__FreeBSD__)
    #define OS_FREEBSD
#elif defined(__unix__)
    #define OS_UNIX
#else
    #define OS_UNKNOWN
#endif

#if defined(OS_LINUX)
    #include <sys/wait.h>
    #include <string.h>
#endif

#if defined(OS_MACOS) || defined(OS_LINUX) || defined(OS_UNIX) || defined(OS_FREEBSD)
    #include <unistd.h>
    #include <sys/wait.h>  // for waitpid
    #define OS_UNIX_LIKE
#endif

#if defined(OS_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

#ifndef OS_UNIX_LIKE_DEFINED
    #if defined(OS_UNIX_LIKE)
         #define OS_UNIX_LIKE_DEFINED 1
    #else
         #define OS_UNIX_LIKE_DEFINED 0
    #endif
#endif

#ifndef OS_WINDOWS_DEFINED
    #if defined(OS_WINDOWS)
         #define OS_WINDOWS_DEFINED 1
    #else
         #define OS_WINDOWS_DEFINED 0
    #endif
#endif

class OS
{
public:
    static std::string detect_os();
    static std::pair<int, std::string> run_command(std::string& args);
    static std::pair<int, std::string> run_command(const std::vector<std::string>& args);
private:
    static std::pair<int, std::string> run_command_unix(const std::vector<std::string>& args);
    static std::pair<int, std::string> run_command_windows(const std::string& command);
};

std::vector<std::string> split(std::string str, char delim);
std::vector<std::string> split_preserve_quotes(std::string str, char delim);

