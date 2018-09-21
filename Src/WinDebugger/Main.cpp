// \brief
//		WinDebugger Main.cpp
//

#include <cstdio>
#include "Foundation/AppHelper.h"
#include "WinDebugger.h"


static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	BOOL bHandled = FALSE;
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		bHandled = TRUE;
		break;
	default:
		break;
	}

	appConsolePrintf(TEXT("Ctrl-C: ThreadId=%d\n"), GetCurrentThreadId());
	return bHandled;
}

int main(int argc, char *argv[])
{
	FWinDebugger Debugger;

	setlocale(LC_CTYPE, "");
	appSetConsoleCtrlHandler(ConsoleCtrlHandler);
	Debugger.MainLoop();

	return 0;
}
