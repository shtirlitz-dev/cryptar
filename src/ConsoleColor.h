#pragma once

enum {
	FG_RED,  // FOREGROUND_RED

	STD_ERR, // STD_ERROR_HANDLE
};

class ConsoleColor
{
#if 0
	static const WORD NoColor = 0xFFFF;
public:
	ConsoleColor(WORD wAttributes = NoColor, DWORD nStdHandle = STD_OUTPUT_HANDLE);
	~ConsoleColor();
	void SetColor(WORD wAttributes = NoColor); // FOREGROUND_RED | FOREGROUND_INTENSITY, 0xFFFF to reset
protected:
	HANDLE hStdOut;
	WORD wDefault = NoColor;
#else
public:
	ConsoleColor(...) {}
#endif
};

