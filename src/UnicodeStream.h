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
#include <optional>

#include "CoroGenerator.h"

class CharStream
{
public:
	virtual int Read(char* buf, int count) = 0;
};

Coro::generator<std::wstring> GetStrings(CharStream* pStream);
