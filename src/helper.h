#pragma once

#include "PluginDefinition.h"
#include "menuCmdID.h"

#include <vector>
#include <string>
#include <Windows.h>
#include <filesystem>

std::filesystem::path getBuildDir(NppData &nppData)
{
    TCHAR fileName[MAX_PATH];
    ::SendMessage(
        nppData._nppHandle,
        NPPM_GETCURRENTDIRECTORY,
        MAX_PATH,
        reinterpret_cast<LPARAM>(fileName));

    std::filesystem::path path(fileName);

    if (path.stem() != "src")
    {
        ::MessageBox(NULL, TEXT("Only works when you're in src directory"), TEXT("ERROR"), MB_OK);
        return std::filesystem::path();
    }

    std::filesystem::path buildDir = path.parent_path() / "build";
    std::filesystem::create_directory(buildDir);
    return buildDir;
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
        ::MessageBox(NULL, L"Failed to start SDCC", L"Error", MB_OK);
        return 1;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return 0;
}

bool buildSourceFile(NppData &nppData, std::filesystem::path path, std::wstring modelSize, std::wstring xramSize, std::wstring iramSize, std::wstring codeSize)
{
    std::filesystem::path fileName = path.stem();
    std::filesystem::path buildDirectory = getBuildDir(nppData);
    std::filesystem::path buildFile = buildDirectory / fileName;

    std::wstring command = L"cmd.exe /C sdcc -mmcs51 -c"
                           L" --model-" +
                           modelSize +
                           L" -I C:\\c_libraries" +
                           L" --xram-size " + xramSize +
                           L" --iram-size " + iramSize +
                           L" --code-size " + codeSize +
                           L" " +
                           path.wstring() +
                           L" -o " +
                           buildFile.wstring() + L".rel";

    return runCommand(command);
}

std::vector<std::filesystem::path> getLinkFiles(NppData &nppData)
{
    std::filesystem::path buildDirectory = getBuildDir(nppData);
    std::vector<std::filesystem::path> linkFiles;

    for (const auto &entry : std::filesystem::directory_iterator(buildDirectory))
    {
        const auto extension = entry.path().extension();
        if (entry.is_regular_file() && (extension == ".rel"))
        {
            linkFiles.push_back(entry.path());
        }
    }

    return linkFiles;
}

bool linkFiles(NppData &nppData, std::wstring modelSize, std::wstring xramSize, std::wstring iramSize, std::wstring codeSize)
{
    std::vector<std::filesystem::path> linkFiles = getLinkFiles(nppData);

    std::filesystem::path buildDirectory = getBuildDir(nppData);

    std::wstring command = L"cmd.exe /C sdcc -mmcs51"
                           L" --model-" +
                           modelSize +
                           L" -I C:\\c_libraries" +
                           L" --xram-size " + xramSize +
                           L" --iram-size " + iramSize +
                           L" --code-size " + codeSize +
                           L" --out-fmt-ihx ";

    for (const auto &file : linkFiles)
    {
        command += file.wstring() + L" ";
    }

    std::filesystem::path output_ihx = buildDirectory / "output.ihx";
    std::filesystem::path output_hex = buildDirectory / "output.hex";

    command += L"-o " + output_ihx.wstring();

    runCommand(command);
    runCommand(L"cmd.exe /C packihx " + output_ihx.wstring() + L" > " + output_hex.wstring());

    return 0;
}

std::vector<std::filesystem::path> getSourceFiles(NppData &nppData)
{
    TCHAR path[MAX_PATH];
    ::SendMessage(
        nppData._nppHandle,
        NPPM_GETCURRENTDIRECTORY,
        MAX_PATH,
        reinterpret_cast<LPARAM>(path));

    std::filesystem::path srcDirectory(path);

    std::vector<std::filesystem::path> sourceFiles;

    for (const auto &entry : std::filesystem::directory_iterator(srcDirectory))
    {
        const auto extension = entry.path().extension();
        if (entry.is_regular_file() && (extension == ".c"))
        {
            sourceFiles.push_back(entry.path());
        }
    }

    return sourceFiles;
}

bool buildSourceFiles(NppData &nppData)
{
    std::vector<std::filesystem::path> sourceFiles = getSourceFiles(nppData);

    std::filesystem::path buildDir = getBuildDir(nppData);
    if (std::filesystem::exists(buildDir))
    {
        std::filesystem::remove_all(buildDir);
    }

    for (const auto &file : sourceFiles)
    {
        buildSourceFile(nppData, file, L"small", L"256", L"256", L"65536");
    }
    return 0;
}