// \brief
//		WinDebugger Class.
//

#include "Foundation\AppHelper.h"
#include "WinDebugger.h"


#define TRACE_ERROR(msg)	TraceWindowsError(__FILE__, __LINE__, msg);

// trace msg
static VOID TraceWindowsError(const char* InFILE, int32_t InLine, const TCHAR *InMsgDeclare)
{
	TCHAR szMsg[MAX_PATH];
	TCHAR szFile[MAX_PATH];

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		0, // Default language
		(LPTSTR)szMsg,
		MAX_PATH,
		NULL
	);

	appANSIToTCHAR(InFILE, szFile, XARRAY_COUNT(szFile));
	appConsolePrintf(TEXT("TraceError: %s at %s:%d: %s\n"), InMsgDeclare, szFile, InLine, szMsg);
}

// display debug event
static VOID DisplayDebugEvent(const LPDEBUG_EVENT InDbgEvent)
{
	static const int32_t MAX_DBG_EVENT = 9;
	static TCHAR* DbgEventName[MAX_DBG_EVENT + 1] = {
		TEXT("EXCEPTION_DEBUG_EVENT"),
		TEXT("CREATE_THREAD_DEBUG_EVENT"),
		TEXT("CREATE_PROCESS_DEBUG_EVENT"),
		TEXT("EXIT_THREAD_DEBUG_EVENT"),
		TEXT("EXIT_PROCESS_DEBUG_EVENT"),
		TEXT("LOAD_DLL_DEBUG_EVENT"),
		TEXT("UNLOAD_DLL_DEBUG_EVENT"),
		TEXT("OUTPUT_DEBUG_STRING_EVENT"),
		TEXT("RIP_EVENT"),
		TEXT("Unknown Debug Event")
	};

	appConsolePrintf(TEXT("DebugEvent from process %d thread %d: %s.\n"),
		InDbgEvent->dwProcessId, InDbgEvent->dwThreadId,
		DbgEventName[InDbgEvent->dwDebugEventCode> MAX_DBG_EVENT ? MAX_DBG_EVENT : InDbgEvent->dwDebugEventCode - 1]);
}

FWinDebugger::FWinDebugger()
{
	DebuggeeCtx.Reset();
}

FWinDebugger::~FWinDebugger()
{}


VOID FWinDebugger::MainLoop()
{
	DEBUG_EVENT DbgEvt;                   // debugging event information 
	DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 
	BOOL bExit = FALSE;

	WaitForUserCommand();
	while (!bExit)
	{
		// Wait for a debugging event to occur. The second parameter indicates 
		// number of milliseconds to wait for a debugging event. If the parameter
		// is INFINITE the function does not return until a debugging event occurs. 

		if (!WaitForDebugEvent(&DbgEvt, INFINITE))
		{
			TRACE_ERROR(TEXT("WaitForDebugEvent"));
			bExit = TRUE;
			continue;
		}

		DebuggeeCtx.pDbgEvent = &DbgEvt;

		//DisplayDebugEvent(&DbgEvt);
		appSetConsoleTextColor(NSConsoleColor::COLOR_GREEN);
		appConsolePrintf(TEXT("DebugEvent from process %d : thread %d>\n"), DbgEvt.dwProcessId, DbgEvt.dwThreadId);
		appSetConsoleTextColor(NSConsoleColor::COLOR_NONE);
		// Process the debugging event code. 
		switch (DbgEvt.dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:
			OnExceptionDebugEvent(DbgEvt);
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			// As needed, examine or change the thread's registers 
			// with the GetThreadContext and SetThreadContext functions; 
			// and suspend and resume thread execution with the 
			// SuspendThread and ResumeThread functions. 
			OnCreateThreadDebugEvent(DbgEvt);
			break;

		case CREATE_PROCESS_DEBUG_EVENT:
			// As needed, examine or change the registers of the 
			// process's initial thread with the GetThreadContext and 
			// SetThreadContext functions; read from and write to the 
			// process's virtual memory with the ReadProcessMemory and 
			// WriteProcessMemory functions; and suspend and resume 
			// thread execution with the SuspendThread and ResumeThread 
			// functions. Be sure to close the handle to the process image 
			// file with CloseHandle.
			OnCreateProcessDebugEvent(DbgEvt);
			break;

		case EXIT_THREAD_DEBUG_EVENT:
			// Display the thread's exit code. 
			OnExitThreadDebugEvent(DbgEvt);
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			// Display the process's exit code.
			OnExitProcessDebugEvent(DbgEvt);
			bExit = TRUE;
			break;

		case LOAD_DLL_DEBUG_EVENT:
			// Read the debugging information included in the newly 
			// loaded DLL. Be sure to close the handle to the loaded DLL 
			// with CloseHandle.
			OnLoadDllDebugEvent(DbgEvt);
			break;

		case UNLOAD_DLL_DEBUG_EVENT:
			// Display a message that the DLL has been unloaded.
			OnUnloadDllDebugEvent(DbgEvt);
			break;

		case OUTPUT_DEBUG_STRING_EVENT:
			// Display the output debugging string. 
			OnOutputDebugStringEvent(DbgEvt);
			break;

		}

		DebuggeeCtx.pDbgEvent = NULL;
		// Resume executing the thread that reported the debugging event. 
		ContinueDebugEvent(DbgEvt.dwProcessId,
			DbgEvt.dwThreadId, dwContinueStatus);
	}
}

// create a debuggee process
BOOL FWinDebugger::DebugNewProcess(const TCHAR *InExeFilename, const TCHAR *InParams)
{
	STARTUPINFO           StartupInfo;
	PROCESS_INFORMATION   ProcessInfo;

	memset(&StartupInfo, NULL, sizeof(STARTUPINFO));
	memset(&ProcessInfo, NULL, sizeof(PROCESS_INFORMATION));

	StartupInfo.cb = sizeof(STARTUPINFO);

	TCHAR Params[1024];
	memset(Params, 0, sizeof(Params));
	if (InParams)
	{
		appStrncpy(Params, InParams, XARRAY_COUNT(Params));
	}

	DebuggeeCtx.Reset();
	//-- create the Debuggee process
	if (!::CreateProcess(
		InExeFilename,
		Params,
		(LPSECURITY_ATTRIBUTES)0L,
		(LPSECURITY_ATTRIBUTES)0L,
		TRUE,
		DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
		(LPVOID)0L,
		(LPTSTR)0L,
		&StartupInfo, &ProcessInfo))
	{
		TRACE_ERROR(TEXT("CreateProcess"));
		return(FALSE);
	}
	else
	{
		CloseHandle(ProcessInfo.hThread);
		DebuggeeCtx.hProcess = ProcessInfo.hProcess;
	}

	return(TRUE);
}

BOOL FWinDebugger::DebugActiveProcess(DWORD InProcessId)
{
	if (!::DebugActiveProcess(InProcessId))
	{
		TRACE_ERROR(TEXT("DebugActiveProcess"));
		return FALSE;
	}

	return TRUE;
}

BOOL FWinDebugger::DebugActiveProcessStop(DWORD InProcessId)
{
	if (!::DebugActiveProcessStop(InProcessId))
	{
		TRACE_ERROR(TEXT("DebugActiveProcessStop"));
		return FALSE;
	}

	return TRUE;
}

BOOL FWinDebugger::DebugKillProcess(HANDLE InhProcess)
{
	if (InhProcess != INVALID_HANDLE_VALUE)
	{
		return ::TerminateProcess(InhProcess, -1);
	}

	return FALSE;
}

// Debug Event Handler
VOID FWinDebugger::OnExceptionDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	// Process the exception code. When handling 
	// exceptions, remember to set the continuation 
	// status parameter (dwContinueStatus). This value 
	// is used by the ContinueDebugEvent function. 
	printf("-Debuggee breaks into debugger; press any key to continue.\n");
	getchar();
	//return TRUE;

	switch (InDbgEvent.u.Exception.ExceptionRecord.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		// First chance: Pass this on to the system. 
		// Last chance: Display an appropriate error. 
		break;

	case EXCEPTION_BREAKPOINT:
		// First chance: Display the current 
		// instruction and register values. 
		break;

	case EXCEPTION_DATATYPE_MISALIGNMENT:
		// First chance: Pass this on to the system. 
		// Last chance: Display an appropriate error. 
		break;

	case EXCEPTION_SINGLE_STEP:
		// First chance: Update the display of the 
		// current instruction and register values. 
		break;

	case DBG_CONTROL_C:
		// First chance: Pass this on to the system. 
		// Last chance: Display an appropriate error. 
		break;

	default:
		// Handle other exceptions. 
		break;
	}
}

VOID FWinDebugger::OnCreateThreadDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	appConsolePrintf(TEXT("CREATE_THREAD_DEBUG_INFO: \n"));
	appConsolePrintf(TEXT("    hThread:   0x%08x\n"), InDbgEvent.u.CreateThread.hThread);
	appConsolePrintf(TEXT("    LocalBase: 0x%08x\n"), InDbgEvent.u.CreateThread.lpThreadLocalBase);
	appConsolePrintf(TEXT("    StartAddr: 0x%08x\n"), InDbgEvent.u.CreateThread.lpStartAddress);
}

VOID FWinDebugger::OnCreateProcessDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	const wstring ImageFile = appGetFinalPathNameByHandle(InDbgEvent.u.CreateProcessInfo.hFile);

	appConsolePrintf(TEXT("CREATE_PROCESS_DEBUG_EVENT: \n"));
	appConsolePrintf(TEXT("    Image: %s\n"), ImageFile.c_str());
	appConsolePrintf(TEXT("    BaseAddr Of Image: 0x%08x\n"), InDbgEvent.u.CreateProcessInfo.lpBaseOfImage);
	appConsolePrintf(TEXT("    hProcess: 0x%08x, hThread: 0x%08x\n"), InDbgEvent.u.CreateProcessInfo.hProcess, InDbgEvent.u.CreateProcessInfo.hThread);
	appConsolePrintf(TEXT("    StartAddr: 0x%08x\n"), InDbgEvent.u.CreateProcessInfo.lpStartAddress);

	CloseHandle(InDbgEvent.u.CreateProcessInfo.hFile);
}

VOID FWinDebugger::OnExitThreadDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	appConsolePrintf(TEXT("EXIT_THREAD_DEBUG_EVENT: \n"));
	appConsolePrintf(TEXT("    ExitCode:   %d\n"), InDbgEvent.u.ExitThread.dwExitCode);
}

VOID FWinDebugger::OnExitProcessDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	appConsolePrintf(TEXT("EXIT_PROCESS_DEBUG_EVENT: \n"));
	appConsolePrintf(TEXT("    ExitCode:   %d\n"), InDbgEvent.u.ExitProcess.dwExitCode);
}

VOID FWinDebugger::OnLoadDllDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	const wstring ImageFile = appGetFinalPathNameByHandle(InDbgEvent.u.LoadDll.hFile);

	appConsolePrintf(TEXT("LOAD_DLL_DEBUG_INFO: \n"));
	appConsolePrintf(TEXT("    Image: %s\n"), ImageFile.c_str());
	appConsolePrintf(TEXT("    BaseAddr Of DLL: 0x%08x\n"), InDbgEvent.u.LoadDll.lpBaseOfDll);

	CloseHandle(InDbgEvent.u.LoadDll.hFile);
}

VOID FWinDebugger::OnUnloadDllDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	appConsolePrintf(TEXT("UNLOAD_DLL_DEBUG_INFO: \n"));
	appConsolePrintf(TEXT("    BaseAddr Of DLL: 0x%08x\n"), InDbgEvent.u.UnloadDll.lpBaseOfDll);
}

VOID FWinDebugger::OnOutputDebugStringEvent(const DEBUG_EVENT &InDbgEvent)
{
	wstring DebugString;

	const DWORD nChars = InDbgEvent.u.DebugString.nDebugStringLength;
	if (InDbgEvent.u.DebugString.fUnicode)
	{
		TCHAR *szBuffer = new TCHAR[nChars];
		if (szBuffer)
		{
			::ReadProcessMemory(DebuggeeCtx.hProcess, InDbgEvent.u.DebugString.lpDebugStringData, szBuffer, nChars * sizeof(TCHAR), NULL);
			DebugString = szBuffer;
		}
		delete[] szBuffer;
	}
	else
	{
		char *szBuffer = new char[nChars];
		TCHAR *szUnicode = new TCHAR[nChars];
		if (szBuffer && szUnicode)
		{
			::ReadProcessMemory(DebuggeeCtx.hProcess, InDbgEvent.u.DebugString.lpDebugStringData, szBuffer, nChars * sizeof(char), NULL);
			appANSIToTCHAR(szBuffer, szUnicode, nChars);
			DebugString = szUnicode;
		}
		delete[] szBuffer;
		delete[] szUnicode;
	}

	appConsolePrintf(TEXT("OUTPUT_DEBUG_STRING_INFO: \n"));
	appConsolePrintf(TEXT("    %s\n"), DebugString.c_str());
}

VOID FWinDebugger::OnRipEvent(const DEBUG_EVENT &InDbgEvent)
{
	appConsolePrintf(TEXT("RIP_INFO: \n"));
	appConsolePrintf(TEXT("    dwError=%d, dwType=%d\n"), InDbgEvent.u.RipInfo.dwError, InDbgEvent.u.RipInfo.dwType);
}

///////////////////////////////////////////////////////////////////////////////////////////
// USER COMMANDS
const FWinDebugger::FCommandMeta FWinDebugger::sUserCommands[] =
{
	{ TEXT("help"),   TEXT("help"),					   TEXT("help [cmd]"),				     &FWinDebugger::Command_Help },
	{ TEXT("run"),    TEXT("debug a new process"),     TEXT("run filename [param0 param1]"), &FWinDebugger::Command_NewProcess },
	{ TEXT("attach"), TEXT("attach a active process"), TEXT("attach pid"),                   &FWinDebugger::Command_AttachProcess },
	{ TEXT("detach"), TEXT("detach current debuggee"), TEXT("detach"),						 &FWinDebugger::Command_DetachProcess },
	{ TEXT("stop"),   TEXT("ternimate debuggee"),	   TEXT("detach"),						 &FWinDebugger::Command_StopDebug }
};

VOID FWinDebugger::WaitForUserCommand()
{
	vector<wstring> Tokens, Switchs;
	TCHAR szCmdBuffer[1024];
	BOOL bQuitWait = FALSE;

	do 
	{
		appConsolePrintf(TEXT(">"));

		TCHAR *pCmdLine = appGetConsoleLine(szCmdBuffer, XARRAY_COUNT(szCmdBuffer));
		appParseCommandLine(pCmdLine, Tokens, Switchs);

		if (Tokens.size() > 0)
		{
			wstring Command = Tokens[0];
			Tokens.erase(Tokens.begin());
		
			bQuitWait = DispatchUserCommand(Command, Tokens, Switchs);
		}
	} while (!bQuitWait);
}

BOOL FWinDebugger::DispatchUserCommand(const wstring &InCmd, const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	for (int32_t k = 0; k < XARRAY_COUNT(sUserCommands); k++)
	{
		const FCommandMeta &CmdMeta = sUserCommands[k];
		if (!appStricmp(InCmd.c_str(), CmdMeta.mName))
		{
			PtrCommandFunction PtrFunc = CmdMeta.mFunc;
			return (this->*PtrFunc)(InTokens, InSwitchs);
		}
	} // end for k

	return FALSE;
}

BOOL FWinDebugger::Command_Help(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	if (InTokens.empty())
	{
		// list all commands
		for (int32_t k = 0; k < XARRAY_COUNT(sUserCommands); k++)
		{
			const FCommandMeta &CmdMeta = sUserCommands[k];
			appConsolePrintf(TEXT("%d. %s\t: %s, usage:%s\n"), k, CmdMeta.mName, CmdMeta.mDesc, CmdMeta.mUsage);
		} // end for k
	}
	else
	{
		const wstring &CmdName = InTokens[0];
		for (int32_t k = 0; k < XARRAY_COUNT(sUserCommands); k++)
		{
			const FCommandMeta &CmdMeta = sUserCommands[k];
			if (!appStricmp(CmdName.c_str(), CmdMeta.mName))
			{
				appConsolePrintf(TEXT("%d. %s\t: %s, usage:%s\n"), k, CmdMeta.mName, CmdMeta.mDesc, CmdMeta.mUsage);
				break;
			}
		} // end for k
	}

	return FALSE;
}

// user command handlers
BOOL FWinDebugger::Command_NewProcess(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	const TCHAR *szExeFilename = NULL;
	const TCHAR *szParams = NULL;

	if (InTokens.size() > 0) 
	{
		szExeFilename = InTokens[0].c_str();
	}
	if (InTokens.size() > 1)
	{
		szParams = InTokens[1].c_str();
	}

	BOOL bSuccess = FALSE;
	if (szExeFilename != NULL)
	{
		bSuccess = DebugNewProcess(szExeFilename, szParams);
	}
	return bSuccess;
}

BOOL FWinDebugger::Command_AttachProcess(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	BOOL bSuccess = FALSE;
	DWORD Pid = -1;

	if (InTokens.size() > 0)
	{
		Pid = appAtoi(InTokens[0].c_str());
		bSuccess = DebugActiveProcess(Pid);
	}

	return TRUE;
}

BOOL FWinDebugger::Command_DetachProcess(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	BOOL bSuccess = FALSE;
	if (DebuggeeCtx.hProcess != INVALID_HANDLE_VALUE)
	{
		DWORD ProcessId = GetProcessId(DebuggeeCtx.hProcess);
		bSuccess = DebugActiveProcessStop(ProcessId);
	}

	return bSuccess;
}

BOOL FWinDebugger::Command_StopDebug(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	return DebugKillProcess(DebuggeeCtx.hProcess);
}
