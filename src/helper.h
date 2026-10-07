#pragma once

#include "PluginDefinition.h"
#include "menuCmdID.h"

#include <vector>
#include <string>
#include <filesystem>

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