// \brief
//		WinDebugger Main.cpp
//

#include <cstdio>
#include "Foundation/AppHelper.h"

static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		break;
	case CTRL_BREAK_EVENT:
		break;
	default:
		break;
	}

	return FALSE;
}

int main(int argc, char *argv[])
{
	TCHAR InputLine[1024];

	appSetConsoleCtrlHandler(ConsoleCtrlHandler);
	appSetConsoleTitle(TEXT("Hello World..."));

	appSetConsoleTextColor(NSConsoleColor::COLOR_RED);
	for (int i = 0; i < 20; i++)
	{
		appConsolePrintf(TEXT("step %d\n"), i);
	}

	COORD Cursor = appGetConsoleCursorPosition();
	appConsolePrintf(TEXT("Cursor:<%d, %d>\n"), Cursor.X, Cursor.Y);

	appClearConsoleScreen();

	while (1)
	{
		TCHAR *pLine = appGetConsoleLine(InputLine, XARRAY_COUNT(InputLine));
		vector<wstring> Tokens, Switchs;
		appParseCommandLine(pLine, Tokens, Switchs);
		for (size_t k = 0; k < Tokens.size(); k++)
		{
			appConsolePrintf(TEXT("token %d: %s"), k, Tokens[k].c_str());
		}
		for (size_t k = 0; k < Switchs.size(); k++)
		{
			appConsolePrintf(TEXT("switch %d: %s"), k, Switchs[k].c_str());
		}
	}

	return 0;
}
