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

#include "pch.h"
#include "cryptar.h"
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <tchar.h>
#include "ntfs_streams.h"
#include "FileSimple.h"
#include "UnicodeStream.h"
#include "UnicodeFuncts.h"
#include "Tar.h"
#include "CommonFunc.h"
#include "ConsoleColor.h"

using namespace std;

int wmain2(int argc, TCHAR** argv) // main(int argc, char **argv)
{
	//int test_aes();
	//return test_aes();
	_setmode(_fileno(stdout), _O_U8TEXT);  // enable Unicode in console
	_setmode(_fileno(stdin), _O_U8TEXT);  // enable Unicode in console
	_setmode(_fileno(stderr), _O_U8TEXT);  // enable Unicode in console

	try
	{
		if (argc == 1) // only executable
		{
			ShowShortHelp(filesystem::path(argv[0]).filename());
			return ShowListFiles(nullptr, false);
		}

		auto cmd = argv[1];
		const wchar_t* arg2 = argc >= 2 ? argv[2] : nullptr;
		const wchar_t* arg3 = argc >= 3 ? argv[3] : nullptr;

		if (_tcscmp(cmd, L"/?") == 0 ||
			_tcscmp(cmd, L"/help") == 0 ||
			_tcscmp(cmd, L"?") == 0)
		{
			ShowUsage(filesystem::path(argv[0]).filename());
			return 0;
		}

		if (_tcscmp(cmd, L"tar") == 0)
			return Tar(argc, argv);
		if (_tcscmp(cmd, L"untar") == 0)
			return Untar(argc, argv);

		return ShowListFiles(cmd, false);
	}
	catch (MyException& e)
	{
		ConsoleColor cc(FOREGROUND_RED, STD_ERROR_HANDLE);
		String msg = e.msg;
		if (auto pos = msg.find(L"<err>"); pos != String::npos)
			msg = msg.substr(0, pos) + GetErrorMessage(e.dwError) + msg.substr(pos + 5);
		if (auto pos = msg.find(L"<path>"); pos != String::npos)
			msg = msg.substr(0, pos) + e.path + msg.substr(pos + 6);
		wcerr << msg << endl;
	}
	catch (exception& e)
	{
		ConsoleColor cc(FOREGROUND_RED, STD_ERROR_HANDLE);
		wcerr << ToWideChar(e.what(), CP_ACP) << endl;
	}
	catch (...)
	{
		ConsoleColor cc(FOREGROUND_RED, STD_ERROR_HANDLE);
		wcerr << L"Unhandled exception" << endl;
	}
	return 1;
}

void ShowStreamsOnFile(const filesystem::path& filename, bool show_all_files, const wchar_t* prefix)
{
	// Enumerate file's streams and print their sizes and names

	WIN32_FIND_STREAM_DATA fsd;
	HANDLE hFind = ::FindFirstStreamW(filename.c_str(), FindStreamInfoStandard, &fsd, 0);
	for (bool ok = hFind != INVALID_HANDLE_VALUE; ok; ok = !!::FindNextStreamW(hFind, &fsd))
	{
		if (wcscmp(fsd.cStreamName, L"::$DATA") == 0) // this is the main stream
			continue;
		wstring_view stream_name = RemoveAtEnd(fsd.cStreamName, L":$DATA"); // name without ":$DATA" in the end
		int space = show_all_files ? 35 : 15;
		wcout << setw(space) << FileSizeStr(fsd.StreamSize.QuadPart) << L" " << prefix;
		//ConsoleColor cc(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		ConsoleColor cc(FOREGROUND_GREEN | FOREGROUND_BLUE);
		wcout << stream_name << endl;
	}
	if (hFind != INVALID_HANDLE_VALUE)
		::FindClose(hFind);
}

void PrintInfo(const std::filesystem::directory_entry& item)
{
	// no info about item.last_write_time() found
	// but considering it as the same value as FILETIME - works!
	ULONGLONG ftime = item.last_write_time().time_since_epoch().count();
	FILETIME* pft = (FILETIME*)&ftime;
	SYSTEMTIME st;
	FileTimeToLocalFileTime(pft, pft);
	FileTimeToSystemTime(pft, &st);
	TCHAR stime[40];
	swprintf_s(stime, L"%02u.%02u.%04u  %02u:%02u", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);

	wcout << stime << L" " << setw(17);
	if (item.is_regular_file())
	{
		wcout << FileSizeStr(item.file_size());
	}
	else if (item.is_directory())
	{
		wcout << L"<DIR>         ";
	}
	else
	{
		wcout << L"?";
	}
	wcout << L" " << item.path().filename().c_str() << endl;

}

void ListFiles(const filesystem::path& dir, bool show_all_files)
{
	TCHAR szVolName[MAX_PATH], szFSName[MAX_PATH];
	DWORD dwSN, dwMaxLen, dwVolFlags;
	auto root = dir.root_path();
	if (!::GetVolumeInformation(root.c_str(), szVolName, MAX_PATH, &dwSN,
		&dwMaxLen, &dwVolFlags, szFSName, MAX_PATH))
	{
		ConsoleColor cc(FOREGROUND_RED);
		wcout << L"Cannot get volume information for " << root.c_str() << endl;
	}
	else
	{
		wcout << root.c_str() << L" - Volume Name: " << szVolName << ", File System: " << szFSName << endl;
		if (!(dwVolFlags & FILE_NAMED_STREAMS)) {
			ConsoleColor cc(FOREGROUND_RED);
			wcout << L"Named streams are not supported on " << root.c_str() << endl;
		}
	}

	wcout << L"Directory: " << dir.c_str() << endl << endl;

	ShowStreamsOnFile(dir, show_all_files, L".\\");
	for (auto& item : filesystem::directory_iterator(dir))
	{
		if (show_all_files)
			PrintInfo(item);
		ShowStreamsOnFile(item.path(), show_all_files, item.path().filename().c_str());
	}
}

int ShowListFiles(const wchar_t* dir, bool all)
{
	filesystem::path cvt;
	if (!dir)
		cvt = filesystem::current_path();
	else
		cvt = filesystem::canonical(dir);
	ListFiles(cvt, all);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// printing
//////////////////////////////////////////////////////////////////////////


void ShowCopyright()
{
	wcout << L"Copyright (c) 2019 Vsevolod Lukyanin\n";
}

void ShowShortHelp(filesystem::path filename)
{
	ShowCopyright();
	wcout << L"\ntype '" << filename.c_str() << L" /?' to get help\n\n";
}

void ShowUsage(filesystem::path filename)
{
	ShowCopyright();
	wcout << L"\ncommand line arguments for " << filename.c_str() << L":\n\n";
	wcout << L"(no args)         - shows short help and lists streams in the current directory\n";
	wcout << L"<dir>             - lists streams in the specified directory\n";
	wcout << L"dir|ls <dir>      - lists all files and streams in the current or specified directory\n";
	wcout << L"copy <src> <dest> - copies contents from src to dest\n";
	wcout << L"type <src>        - writes specified stream to stdout\n";
	wcout << L"echo <dest>       - copies stdin to the specified stream\n";
	wcout << L"del <src>         - deletes specified stream\n";
	wcout << L"tar|untar /?      - more help about tar-function\n";
	wcout << L"del <src>         - deletes specified stream\n";
	wcout << L"/?                - shows this help\n\n";
	wcout << L"where <src> and <dest> are file names or stream names, e.g. file.txt:stream1 or :s1:$DATA\n";
	wcout << L"      <dir> is some directory name, including . or ..\n";
}

