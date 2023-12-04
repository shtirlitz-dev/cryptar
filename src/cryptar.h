#pragma once

#include <string>
#include <string_view>

using Char = wchar_t;
using String = std::basic_string<Char>;
using StringView = std::basic_string_view<Char>;

struct MyException
{
	String msg;   // can contain "<path>" and "<err>" for inserting path and error description
	String path;
	uint32_t dwError;
};

String GetErrorMessage(uint32_t dw);

