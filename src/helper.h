#pragma once

#include "PluginDefinition.h"
#include "menuCmdID.h"

#include <vector>
#include <string>
#include <Windows.h>
#include <filesystem>

bool runCommand()
{
    std::wstring command = L"cmd.exe /K sdcc -mmcs51 --model-small -I C:\\c_libraries main.c";

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

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return 0;
}

std::vector<std::filesystem::path> getSourceFiles(NppData &nppData)
{
    TCHAR fileName[MAX_PATH];
    ::SendMessage(
        nppData._nppHandle,
        NPPM_GETFULLCURRENTPATH,
        MAX_PATH,
        reinterpret_cast<LPARAM>(fileName));

    std::filesystem::path filePath(fileName);
    std::filesystem::path srcDirectory = filePath.parent_path();

    std::vector<std::filesystem::path> sourceFiles;

    for (const auto &entry : std::filesystem::directory_iterator(srcDirectory))
    {
        const auto extension = entry.path().extension();
        if (entry.is_regular_file() && (extension == ".c" || extension == ".h"))
        {
            sourceFiles.push_back(entry.path());
        }
    }

    return sourceFiles;
}

bool runBuild()
{
    return runCommand();
}
