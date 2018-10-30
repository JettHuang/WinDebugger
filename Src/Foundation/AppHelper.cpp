// \brief
//		helper functions
//

#include <Windows.h>
#include <psapi.h>
#include <strsafe.h>

#include "AppHelper.h"


int32_t appHextoi(const TCHAR *String)
{
	// omit the previous space chars
	for (; *String; String++)
	{
		if (!isspace(*String))
		{
			break;
		}
	} // end for

	int32_t Result = 0;
	int32_t Count = 0;
	for (; *String && Count < 8; String++, Count++)
	{
		TCHAR a = *String;
		if (a >= TEXT('0') && a <= TEXT('9'))
		{
			Result <<= 4; // multi by 16
			Result += a - TEXT('0');
		}
		else if ((a >= TEXT('a') && a <= TEXT('f')) || (a >= TEXT('A') && a <= TEXT('F')))
		{
			Result <<= 4; // multi by 16
			Result += 10 + ((a | 0x20) - TEXT('a'));
		}
		else
		{
			break;
		}
	} // end for 

	return Result;
}

// string
TCHAR* appStrncpy(TCHAR *Dest, const TCHAR *Src, int32_t MaxLen)
{
	assert(MaxLen > 0);
	_tcsncpy(Dest, Src, MaxLen - 1);
	Dest[MaxLen - 1] = 0;
	return Dest;
}

int32_t appANSIToTCHAR(const char* Source, TCHAR *Dest, int32_t InChars)
{
	return MultiByteToWideChar(CP_ACP, 0, Source, -1, Dest, InChars);
}

// console 
void appSetConsoleCtrlHandler(PHANDLER_ROUTINE InHandler)
{
	::SetConsoleCtrlHandler(InHandler, TRUE);
}

void appSetConsoleTitle(const TCHAR *InTitle)
{
	::SetConsoleTitle(InTitle);
}

void appClearConsoleScreen()
{
	COORD coordScreen = { 0, 0 };    // home for the cursor 
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	// Get the number of character cells in the current buffer. 
	if (!GetConsoleScreenBufferInfo(OutputHandle, &csbi))
	{
		return;
	}

	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.

	if (!FillConsoleOutputCharacter(OutputHandle,        // Handle to console screen buffer 
		(TCHAR) ' ',     // Character to write to the buffer
		dwConSize,       // Number of cells to write 
		coordScreen,     // Coordinates of first cell 
		&cCharsWritten))// Receive number of characters written
	{
		return;
	}

	// Get the current text attribute.

	if (!GetConsoleScreenBufferInfo(OutputHandle, &csbi))
	{
		return;
	}

	// Set the buffer's attributes accordingly.

	if (!FillConsoleOutputAttribute(OutputHandle,         // Handle to console screen buffer 
		csbi.wAttributes, // Character attributes to use
		dwConSize,        // Number of cells to set attribute 
		coordScreen,      // Coordinates of first cell 
		&cCharsWritten)) // Receive number of characters written
	{
		return;
	}

	// Put the cursor at its home coordinates.
	SetConsoleCursorPosition(OutputHandle, coordScreen);
}

COORD appGetConsoleCursorPosition()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(OutputHandle, &csbi);
	return csbi.dwCursorPosition;
}

void appSetConsoleCursorPosition(const COORD &InPos)
{
	HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(OutputHandle, InPos);
}

void appSetConsoleTextColor(const TCHAR *InColor)
{
	HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	if (!InColor || appStricmp(InColor, TEXT("")) == 0)
	{
		SetConsoleTextAttribute(OutputHandle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	}
	else
	{
		TCHAR String[9];
		appStrncpy(String, InColor, XARRAY_COUNT(String));
		for (TCHAR *S = String; *S; S++)
		{
			*S -= TEXT('0');
		}
		SetConsoleTextAttribute(OutputHandle,
			(String[0] ? FOREGROUND_RED   : 0) |
			(String[1] ? FOREGROUND_GREEN : 0) |
			(String[2] ? FOREGROUND_BLUE  : 0) |
			(String[3] ? FOREGROUND_INTENSITY : 0) |
			(String[4] ? BACKGROUND_RED   : 0) |
			(String[5] ? BACKGROUND_GREEN : 0) |
			(String[6] ? BACKGROUND_BLUE  : 0) |
			(String[7] ? BACKGROUND_INTENSITY : 0)
			);
	}
}

void appConsolePrintf(const TCHAR *InFormat, ...)
{
	va_list args;
	va_start(args, InFormat);
	_vtprintf(InFormat, args);
	va_end(args);
}

TCHAR* appGetConsoleLine(TCHAR *OutLine, size_t SizeInCharacters)
{
	assert(OutLine);

	return _getts_s(OutLine, SizeInCharacters);
}

// parse cmdline
static bool ParseToken(const TCHAR *&Str, wstring &Arg, bool bUseEscape)
{
	Arg.clear();
	if (!Str) {
		return false;
	}

	while (appIsWhitespace(*Str))
	{
		Str++;
	}

	if (*Str == TEXT('"'))
	{
		// get quoted string ie: "xxxbadsdwewe\"dsewe"
		Str++;
		while (*Str && *Str != TEXT('"'))
		{
			TCHAR Ch = *Str++;
			if (bUseEscape && Ch == TEXT('\\'))
			{
				Ch = *Str++; // discart '\\'
				if (!Ch)
				{
					break;
				}
			}
			Arg += Ch;
		}

		if (*Str == TEXT('"'))
		{
			Str++;
		}
	}
	else
	{
		// get quoted string ie: addr="xxxdsdsds"
		bool bInQuote = false;
		while (1)
		{
			TCHAR Ch = *Str;
			if ((Ch == 0) || (appIsWhitespace(Ch) && !bInQuote))
			{
				break;
			}
			Str++;

			if (bUseEscape && Ch == TEXT('\\') && bInQuote)
			{
				Arg += Ch;
				Ch = *Str;
				if (!Ch)
				{
					break;
				}
				Str++;
			}
			else if (Ch == TEXT('"'))
			{
				bInQuote = !bInQuote;
			}

			Arg += Ch;
		} // end while
	}

	return Arg.length() > 0;
}

void appParseCommandLine(const TCHAR *CmdLine, vector<wstring> &OutTokens, vector<wstring> &OutSwitchs)
{
	wstring NextToken;
	OutTokens.clear();
	OutSwitchs.clear();
	while (ParseToken(CmdLine, NextToken, false))
	{
		TCHAR FirstCh = NextToken[0];
		if (FirstCh == TEXT('-') || FirstCh == TEXT('/'))
		{
			OutSwitchs.push_back(NextToken.substr(1));
		}
		else
		{
			OutTokens.push_back(NextToken);
		}
	} // end while
}

bool appParseParamValue(const TCHAR *InStream, const TCHAR *InMatch, TCHAR *OutValue, uint32_t InMaxLen)
{
	assert(InStream && InMatch && OutValue);
	
	while (*InStream == *InMatch)
	{
		if (*InStream == TEXT('\0'))
		{
			return false;
		}

		InStream++;
		InMatch++;
	}
	if (*InStream == TEXT('\0'))
	{
		return false;
	}

	uint32_t Num = 0;
	InMaxLen -= 1;
	while (*InStream)
	{
		if (Num >= InMaxLen)
		{
			return false;
		}
		*OutValue++ = *InStream++;
		Num++;
	} // end while

	*OutValue = TEXT('\0');
	return true;
}

// ref: https://docs.microsoft.com/zh-cn/windows/desktop/Memory/obtaining-a-file-name-from-a-file-handle
wstring appGetFinalPathNameByHandle(HANDLE hFile)
{
	TCHAR pszFilename[MAX_PATH + 1] = { 0 };
	HANDLE hFileMap;

	// Get the file size.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

	if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
	{
		return TEXT("");
	}

	// Create a file mapping object.
	hFileMap = CreateFileMapping(hFile,
		NULL,
		PAGE_READONLY,
		0,
		1,
		NULL);

	if (hFileMap)
	{
		// Create a file mapping to get the file name.
		void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

		if (pMem)
		{
			if (GetMappedFileName(GetCurrentProcess(),
				pMem,
				pszFilename,
				MAX_PATH))
			{

				// Translate path with device name to drive letters.
				const DWORD BUFSIZE = 512;
				TCHAR szTemp[BUFSIZE];
				szTemp[0] = '\0';

				if (GetLogicalDriveStrings(BUFSIZE - 1, szTemp))
				{
					TCHAR szName[MAX_PATH];
					TCHAR szDrive[3] = TEXT(" :");
					BOOL bFound = FALSE;
					TCHAR* p = szTemp;

					do
					{
						// Copy the drive letter to the template string
						*szDrive = *p;

						// Look up each device name
						if (QueryDosDevice(szDrive, szName, MAX_PATH))
						{
							size_t uNameLen = _tcslen(szName);

							if (uNameLen < MAX_PATH)
							{
								bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
									&& *(pszFilename + uNameLen) == _T('\\');

								if (bFound)
								{
									// Reconstruct pszFilename using szTempFile
									// Replace device path with DOS path
									TCHAR szTempFile[MAX_PATH];
									StringCchPrintf(szTempFile,
										MAX_PATH,
										TEXT("%s%s"),
										szDrive,
										pszFilename + uNameLen);
									StringCchCopyN(pszFilename, MAX_PATH + 1, szTempFile, _tcslen(szTempFile));
								}
							}
						}

						// Go to the next NULL character.
						while (*p++);
					} while (!bFound && *p); // end of string
				}
			}

			UnmapViewOfFile(pMem);
		}

		CloseHandle(hFileMap);
	}

	return pszFilename;
}

// trace msg
VOID TraceWindowsError(const char* InFILE, int32_t InLine, const TCHAR *InMsgDeclare)
{
	TCHAR szMsg[MAX_PATH];
	TCHAR szFile[MAX_PATH];

	DWORD ErrorCode = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		ErrorCode,
		0, // Default language
		(LPTSTR)szMsg,
		MAX_PATH,
		NULL
	);

	appANSIToTCHAR(InFILE, szFile, XARRAY_COUNT(szFile));
	appConsolePrintf(TEXT("TraceError: %s at %s:%d: %s(%d)\n"), InMsgDeclare, szFile, InLine, szMsg, ErrorCode);
}
