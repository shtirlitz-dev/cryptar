#include "cryptar.h"
#include "ConsoleColor.h"
#include "Tar.h"

#include <fcntl.h>
#include <io.h>
#include <tchar.h>
#include <filesystem>
#include <iostream>
#include <ranges>

namespace {

	void PrintLine(std::wstring_view sv, bool withBreak = true)
	{
		std::wcout << sv;
		if (withBreak)
			std::wcout << L"\n";
	}
	void PrintLine(std::string_view sv, bool withBreak = true)
	{
		String str(sv.begin(), sv.end());
		std::wcout << str;
		if (withBreak)
			std::wcout << L"\n";
	}

	void PrintLineSubst(const char* line)
	{
		auto fnd = strstr(line, "{prog}");
		if (!fnd)
			PrintLine(line);
		else {
			PrintLine(std::string_view(line, fnd - line), false);
			PrintLine("cryptar.exe", false);
			PrintLine(fnd + 6);
		}
	}

	void ShowCopyright()
	{
		PrintLine("Copyright (c) 2019-2023 Vsevolod Lukyanin");
	}

	int ShowShortHelp()
	{
		ShowCopyright();
		PrintLineSubst("type '{prog} /?' to get help");
		return 0;
	}

	int ShowHelp(const std::filesystem::path& filename)
	{
		ShowCopyright();
		const char* help[] = {
			"\nCommand line arguments for '{prog} tar:'",
			"tar [options] <tar-file> [<item1> <item2> ...]",
			"where <itemN> are files or directories to add to <tar-file>",
			"If <itemN> are not specified, all items in the current directory are added",
			"Default file extension of tar-file is .ctar",
			"options:",
			"  /t             - test: valid console output but tar-file is not created",
			"  /p:password    - password to encrypt tar-file",
			"  /e:mask1;mask2 - masks to exclude files or directories",
			"\nCommand line arguments for '{prog} untar:'",
			"untar [options] <tar-file> [<dir>]",
			"where <dir> is directory to extract files from <tar-file> to, default is current",
			"options:",
			"  /t             - test: only list directories, files and streams",
			"  /o             - overwrite existing files",
			"  /p:password    - password to decrypt tar-file",
		};
		std::ranges::for_each(help, PrintLineSubst);

		return 0;
	}


}

int wmain(int argc, wchar_t** argv)
{
	_setmode(_fileno(stdout), _O_U8TEXT);  // enable Unicode in console
	_setmode(_fileno(stdin), _O_U8TEXT);  // enable Unicode in console
	_setmode(_fileno(stderr), _O_U8TEXT);  // enable Unicode in console

	if (argc == 1) // only executable
		return ShowShortHelp();

	StringView cmd = argv[1];
	StringView key = cmd[0] == '/' || cmd[0] == '-' ? cmd.substr(1) : StringView{};
	if (cmd == L"?" || key == L"?" || key == L"help")
		return ShowHelp(argv[0]);

	try
	{

		if (cmd == L"tar")
			return Tar(argc, argv);
		if (cmd == L"untar")
			return Untar(argc, argv);
	}
	catch (MyException& e)
	{
		ConsoleColor cc(FG_RED, STD_ERR);
		auto msg = e.msg;
		if (auto pos = msg.find(L"<err>"); pos != String::npos)
			msg = msg.substr(0, pos) + GetErrorMessage(e.dwError) + msg.substr(pos + 5);
		if (auto pos = msg.find(L"<path>"); pos != String::npos)
			msg = msg.substr(0, pos) + e.path + msg.substr(pos + 6);
		PrintLine(msg);
	}
	catch (std::exception& e)
	{
		ConsoleColor cc(FG_RED, STD_ERR);
		PrintLine(e.what());
	}
	catch (...)
	{
		ConsoleColor cc(FG_RED, STD_ERR);
		PrintLine("Unhandled exception");
	}
	return ShowShortHelp();
}

String GetErrorMessage(uint32_t dw)
{
	return String{};

	// TODO
	/*
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	wstring ret = (LPTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
	*/
}
