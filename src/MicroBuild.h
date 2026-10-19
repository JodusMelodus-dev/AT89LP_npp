#pragma once

#include "PluginDefinition.h"
#include "menuCmdID.h"

#include <vector>
#include <string>
#include <Windows.h>
#include <sstream>
#include <optional>
#include <filesystem>
#include <utility>

template <typename... Args>
std::wstring format(Args &&...args)
{
    std::wostringstream stream;
    (stream << ... << std::forward<Args>(args));
    return stream.str();
}

std::optional<std::filesystem::path> getCurrentDirectory(NppData &nppData)
{
    TCHAR currentDirectory[MAX_PATH];
    ::SendMessage(
        nppData._nppHandle,
        NPPM_GETCURRENTDIRECTORY,
        MAX_PATH,
        reinterpret_cast<LPARAM>(currentDirectory));
    std::filesystem::path currentDirectoryPath(currentDirectory);
    if (currentDirectoryPath.stem() != "src")
    {
        ::MessageBox(NULL, TEXT("Please build with main.c open"), TEXT("Error"), MB_OK);
        return std::nullopt;
    }
    return currentDirectoryPath;
}

std::optional<std::filesystem::path> getBuildDir(NppData &nppData)
{
    std::optional<std::filesystem::path> currentDirectoryPath = getCurrentDirectory(nppData);
    if (!currentDirectoryPath)
    {
        return std::nullopt;
    }

    std::filesystem::path buildDir = currentDirectoryPath.value().parent_path() / "build";
    std::filesystem::create_directory(buildDir);
    return buildDir;
}

std::optional<std::filesystem::path> getSourceDir(NppData &nppData)
{
    std::optional<std::filesystem::path> currentDirectoryPath = getCurrentDirectory(nppData);
    if (!currentDirectoryPath)
    {
        return std::nullopt;
    }

    std::filesystem::path srcDir = currentDirectoryPath.value().parent_path() / "src";
    std::filesystem::create_directory(srcDir);
    return srcDir;
}

std::vector<std::filesystem::path> getSourceFiles(NppData &nppData)
{
    std::optional<std::filesystem::path> srcDirectory = getSourceDir(nppData);
    if (!srcDirectory)
    {
        return {};
    }

    std::vector<std::filesystem::path> sourceFiles;

    for (const auto &entry : std::filesystem::directory_iterator(srcDirectory.value()))
    {
        const auto extension = entry.path().extension();
        if (entry.is_regular_file() && (extension == ".c"))
        {
            sourceFiles.push_back(entry.path());
        }
    }

    return sourceFiles;
}

std::vector<std::filesystem::path> getLinkFiles(NppData &nppData)
{
    std::optional<std::filesystem::path> buildDirectory = getBuildDir(nppData);
    if (!buildDirectory)
    {
        return {};
    }

    std::vector<std::filesystem::path> linkFiles;

    for (const auto &entry : std::filesystem::directory_iterator(buildDirectory.value()))
    {
        const auto extension = entry.path().extension();
        if (entry.is_regular_file() && (extension == ".rel"))
        {
            linkFiles.push_back(entry.path());
        }
    }

    return linkFiles;
}

bool runCommand(std::wstring command)
{
    STARTUPINFO startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};

    if (!CreateProcess(
            nullptr,
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo))
    {
        ::MessageBox(NULL, L"Failed to start command process", L"Error", MB_OK);
        return 1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE); // Replace with atomic checks to check if previous processes completed

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return true;
}

bool buildSourceFile(NppData &nppData, std::filesystem::path path, std::wstring modelSize, std::wstring xramSize, std::wstring iramSize, std::wstring codeSize)
{
    std::optional<std::filesystem::path> buildDirectory = getBuildDir(nppData);
    if (!buildDirectory)
    {
        return false;
    }

    std::filesystem::path fileName = path.stem();
    std::filesystem::path buildFile = buildDirectory.value() / fileName;

    std::wstring command = L"cmd.exe /C sdcc -mmcs51 -c"
                           L" --model-" +
                           modelSize +
                           L" -I \"C:\\c_libraries\"" +
                           L" --xram-size " + xramSize +
                           L" --iram-size " + iramSize +
                           L" --code-size " + codeSize +
                           L" \"" +
                           path.wstring() +
                           L"\" -o " +
                           buildFile.wstring() + L".rel";

    return runCommand(command);
}

bool linkFiles(NppData &nppData, std::wstring modelSize, std::wstring xramSize, std::wstring iramSize, std::wstring codeSize)
{
    std::optional<std::filesystem::path> buildDirectory = getBuildDir(nppData);
    if (!buildDirectory)
    {
        return false;
    }

    std::vector<std::filesystem::path> linkFiles = getLinkFiles(nppData);

    std::wstring command = L"cmd.exe /C sdcc -mmcs51"
                           L" --model-" +
                           modelSize +
                           L" -I \"C:\\c_libraries\"" +
                           L" --xram-size " + xramSize +
                           L" --iram-size " + iramSize +
                           L" --code-size " + codeSize +
                           L" --out-fmt-ihx ";

    for (const auto &file : linkFiles)
    {
        command += file.wstring() + L" ";
    }

    std::filesystem::path output_ihx = buildDirectory.value() / "output.ihx";
    std::filesystem::path output_hex = buildDirectory.value() / "output.hex";

    command += L"-o " + output_ihx.wstring();

    runCommand(command);
    runCommand(L"cmd.exe /C packihx " + output_ihx.wstring() + L" > " + output_hex.wstring());

    return true;
}

bool buildSourceFiles(NppData &nppData)
{
    std::vector<std::filesystem::path> sourceFiles = getSourceFiles(nppData);
    std::optional<std::filesystem::path> buildDir = getBuildDir(nppData);
    if (!buildDir)
    {
        return false;
    }

    if (std::filesystem::exists(buildDir.value()))
    {
        std::filesystem::remove_all(buildDir.value());
    }

    for (const auto &file : sourceFiles)
    {
        if (!buildSourceFile(nppData, file, L"small", L"256", L"256", L"65536"))
        {
            ::MessageBoxW(NULL, format("Failed to build source file: ", file.native()).c_str(), TEXT("Error"), MB_OK);
            return false;
        }
    }
    return true;
}