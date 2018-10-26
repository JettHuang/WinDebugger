// \brief
//		helper functions.
//

#pragma once

#include <Windows.h>
#include <wincon.h>

#include <cstdint>
#include <cassert>
#include <cstdarg>
#include <tchar.h>

#include <vector>
#include <string>

using namespace std;


#define XARRAY_COUNT(a)		(sizeof(a)/sizeof((a)[0]))

// text color
// an empty string reverts to the normal gray on black
namespace NSConsoleColor
{
	const TCHAR* const COLOR_BLACK        = TEXT("0000");
	const TCHAR* const COLOR_DARK_RED     = TEXT("1000");
	const TCHAR* const COLOR_DARK_GREEN   = TEXT("0100");
	const TCHAR* const COLOR_DARK_BLUE    = TEXT("0010");
	const TCHAR* const COLOR_DARK_YELLOW  = TEXT("1100");
	const TCHAR* const COLOR_DARK_CYAN    = TEXT("0110");
	const TCHAR* const COLOR_DARK_PURPLE  = TEXT("1010");
	const TCHAR* const COLOR_DARK_WHITE   = TEXT("1110");
	const TCHAR* const COLOR_GRAY         = COLOR_DARK_WHITE;

	const TCHAR* const COLOR_RED          = TEXT("1001");
	const TCHAR* const COLOR_GREEN        = TEXT("0101");
	const TCHAR* const COLOR_BLUE         = TEXT("0011");
	const TCHAR* const COLOR_YELLOW       = TEXT("1101");
	const TCHAR* const COLOR_CYAN         = TEXT("0111");
	const TCHAR* const COLOR_PURPLE       = TEXT("1011");
	const TCHAR* const COLOR_WHITE        = TEXT("1111");

	const TCHAR* const COLOR_NONE         = TEXT("");
}

// string
inline int32_t appStrlen(const TCHAR *String) { return _tcslen(String); }
inline TCHAR* appStrstr(const TCHAR *String, const TCHAR *Find) { return (TCHAR *)_tcsstr(String, Find); }
inline TCHAR* appStrchr(const TCHAR *String, int32_t c) { return (TCHAR *)_tcschr(String, c); }
inline TCHAR* appStrrchr(const TCHAR *String, int32_t c) { return (TCHAR *)_tcsrchr(String, c); }
inline int32_t appStrcmp(const TCHAR *String1, const TCHAR *String2) { return _tcscmp(String1, String2); }
inline int32_t appStricmp(const TCHAR *String1, const TCHAR *String2) { return _tcsicmp(String1, String2); }
inline int32_t appStrncmp(const TCHAR *String1, const TCHAR *String2, size_t Count) { return _tcsncmp(String1, String2, Count); }
inline int32_t appStrnicmp(const TCHAR *String1, const TCHAR *String2, size_t Count) { return _tcsnicmp(String1, String2, Count); }
TCHAR* appStrncpy(TCHAR *Dest, const TCHAR *Src, int32_t MaxLen);
inline bool appIsWhitespace(TCHAR c) { return (c == TEXT(' ') || c == TEXT('\t')); }
int32_t appANSIToTCHAR(const char* Source, TCHAR *Dest, int32_t InChars);

// string to number 
inline int32_t appAtoi(const TCHAR *String) { return _tstoi(String); }
inline int64_t appAtoi64(const TCHAR *String) { return _tstoi64(String); }
inline float appAtof(const TCHAR *String) { return (float)_tstof(String); }
inline double appAtod(const TCHAR *String) { return _tcstod(String, NULL); }
inline uint32_t appStrtoi(const TCHAR* Start, TCHAR** End, int32_t Base) { return _tcstoul(Start, End, Base); }
inline uint64_t appStrtoi64(const TCHAR* Start, TCHAR** End, int32_t Base) { return _tcstoui64(Start, End, Base); }
inline TCHAR* appItoA(int Val, TCHAR *szBuffer, int Radix) { return _itot(Val, szBuffer, Radix); }
int32_t appHextoi(const TCHAR *String);

// console 
void appSetConsoleCtrlHandler(PHANDLER_ROUTINE InHandler);
void appSetConsoleTitle(const TCHAR *InTitle);
void appClearConsoleScreen();
COORD appGetConsoleCursorPosition();
void appSetConsoleCursorPosition(const COORD &InPos);
void appSetConsoleTextColor(const TCHAR *InColor);
void appConsolePrintf(const TCHAR *InFormat, ...);
TCHAR* appGetConsoleLine(TCHAR *OutLine, size_t SizeInCharacters);

// parse cmdline
// tokens  xxx
// switchs -xxx
void appParseCommandLine(const TCHAR *CmdLine, vector<wstring> &OutTokens, vector<wstring> &OutSwitchs);
// parse param value
// ie. A=yy
bool appParseParamValue(const TCHAR *InStream, const TCHAR *InMatch, TCHAR *OutValue, uint32_t InMaxLen);

//Obtaining a File Name From a File Handle
wstring appGetFinalPathNameByHandle(HANDLE hFile);
