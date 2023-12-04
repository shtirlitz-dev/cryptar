/***********************************************************************

  Copyright (c) 2019 Vsevolod Lukyanin
  All rights reserved.

  This file is a part of project ntfs_file_streams:
  https://github.com/shtirlitz-dev/ntfs_file_streams

  The tool that enables to list, create, copy, delete, show, write and
  archive NTFS files streams.
  About file streams:
  https://docs.microsoft.com/en-us/windows/win32/fileio/file-streams

***********************************************************************/

#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <stdexcept>

void ShowCopyright();
void ShowShortHelp(std::filesystem::path filename);
void ShowUsage(std::filesystem::path filename);

int ShowListFiles(const wchar_t* dir, bool all);

std::wstring GetErrorMessage(DWORD dw);

