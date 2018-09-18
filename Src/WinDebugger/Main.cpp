// \brief
//		WinDebugger Main.cpp
//

#include <cstdio>
#include "Foundation/AppHelper.h"


int main(int argc, char *argv[])
{
	TCHAR InputLine[1024];

	appSetConsoleTitle(TEXT("Hello World..."));

	appSetConsoleTextColor(NSConsoleColor::COLOR_RED);
	for (int i = 0; i < 20; i++)
	{
		appConsolePrintf(TEXT("step %d\n"), i);
	}

	COORD Cursor = appGetConsoleCursorPosition();
	appConsolePrintf(TEXT("Cursor:<%d, %d>\n"), Cursor.X, Cursor.Y);

	TCHAR *pLine = appGetConsoleLine(InputLine, XARRAY_COUNT(InputLine));

	appClearConsoleScreen();
	return 0;
}
