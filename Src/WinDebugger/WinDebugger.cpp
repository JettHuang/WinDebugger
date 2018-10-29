// \brief
//		WinDebugger Class.
//

#include "Foundation\AppHelper.h"
#include "WinDebugger.h"
#include "WinProcessHelper.h"

#include <DbgHelp.h>



#define TRACE_ERROR(msg)	TraceWindowsError(__FILE__, __LINE__, msg);

// trace msg
static VOID TraceWindowsError(const char* InFILE, int32_t InLine, const TCHAR *InMsgDeclare)
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

static const TCHAR* GetExceptionCodeDescription(DWORD InExceptionCode)
{
	struct FExceptionDesc
	{
		DWORD	ExceptionCode;
		const TCHAR		*szDesc;
	};

	static const FExceptionDesc sDescTable[] = {
		{ EXCEPTION_ACCESS_VIOLATION, TEXT("EXCEPTION_ACCESS_VIOLATION") },
		{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED, TEXT("EXCEPTION_ARRAY_BOUNDS_EXCEEDED") },
		{ EXCEPTION_BREAKPOINT, TEXT("EXCEPTION_BREAKPOINT") },
		{ EXCEPTION_DATATYPE_MISALIGNMENT, TEXT("EXCEPTION_DATATYPE_MISALIGNMENT") },
		{ EXCEPTION_FLT_DENORMAL_OPERAND, TEXT("EXCEPTION_FLT_DENORMAL_OPERAND") },
		{ EXCEPTION_FLT_DIVIDE_BY_ZERO, TEXT("EXCEPTION_FLT_DIVIDE_BY_ZERO") },
		{ EXCEPTION_FLT_INEXACT_RESULT, TEXT("EXCEPTION_FLT_INEXACT_RESULT") },
		{ EXCEPTION_FLT_INVALID_OPERATION, TEXT("EXCEPTION_FLT_INVALID_OPERATION") },
		{ EXCEPTION_FLT_OVERFLOW, TEXT("EXCEPTION_FLT_OVERFLOW") },
		{ EXCEPTION_FLT_STACK_CHECK, TEXT("EXCEPTION_FLT_STACK_CHECK") },
		{ EXCEPTION_FLT_UNDERFLOW, TEXT("EXCEPTION_FLT_UNDERFLOW") },
		{ EXCEPTION_ILLEGAL_INSTRUCTION, TEXT("EXCEPTION_ILLEGAL_INSTRUCTION") },
		{ EXCEPTION_IN_PAGE_ERROR, TEXT("EXCEPTION_IN_PAGE_ERROR") },
		{ EXCEPTION_INT_DIVIDE_BY_ZERO, TEXT("EXCEPTION_INT_DIVIDE_BY_ZERO") },
		{ EXCEPTION_INT_OVERFLOW, TEXT("EXCEPTION_INT_OVERFLOW") },
		{ EXCEPTION_INVALID_DISPOSITION, TEXT("EXCEPTION_INVALID_DISPOSITION") },
		{ EXCEPTION_NONCONTINUABLE_EXCEPTION, TEXT("EXCEPTION_NONCONTINUABLE_EXCEPTION") },
		{ EXCEPTION_PRIV_INSTRUCTION, TEXT("EXCEPTION_PRIV_INSTRUCTION") },
		{ EXCEPTION_SINGLE_STEP, TEXT("EXCEPTION_SINGLE_STEP") },
		{ EXCEPTION_STACK_OVERFLOW, TEXT("EXCEPTION_STACK_OVERFLOW") }
	};

	for (uint32_t k = 0; k < XARRAY_COUNT(sDescTable); k++)
	{
		if (sDescTable[k].ExceptionCode == InExceptionCode)
		{
			return sDescTable[k].szDesc;
		}
	} // end for k

	return TEXT("Unknown");
}

static const TCHAR* GetSymTypeString(DWORD InSymType)
{
	struct FSymTypeDesc
	{
		DWORD	SymType;
		const TCHAR		*szDesc;
	};

	static const FSymTypeDesc sDescTable[] = {
		{ SymNone, TEXT("SymNone") },
		{ SymCoff, TEXT("SymCoff") },
		{ SymCv,   TEXT("SymCv") },
		{ SymPdb,  TEXT("SymPdb") },
		{ SymExport, TEXT("SymExport") },
		{ SymDeferred, TEXT("SymDeferred") },
		{ SymSym,      TEXT("SymSym .sym file") },
		{ SymDia,      TEXT("SymDia") },
		{ SymVirtual,  TEXT("SymVirtual") }
	};

	for (uint32_t k = 0; k < XARRAY_COUNT(sDescTable); k++)
	{
		if (sDescTable[k].SymType == InSymType)
		{
			return sDescTable[k].szDesc;
		}
	} // end for k

	return TEXT("Unknown Format");
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
			ContinueDebugEvent(TRUE);
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
			ContinueDebugEvent(TRUE);
			break;

		case EXIT_THREAD_DEBUG_EVENT:
			// Display the thread's exit code. 
			OnExitThreadDebugEvent(DbgEvt);
			ContinueDebugEvent(TRUE);
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			// Display the process's exit code.
			OnExitProcessDebugEvent(DbgEvt);
			ContinueDebugEvent(TRUE);
			bExit = TRUE;
			break;

		case LOAD_DLL_DEBUG_EVENT:
			// Read the debugging information included in the newly 
			// loaded DLL. Be sure to close the handle to the loaded DLL 
			// with CloseHandle.
			OnLoadDllDebugEvent(DbgEvt);
			ContinueDebugEvent(TRUE);
			break;

		case UNLOAD_DLL_DEBUG_EVENT:
			// Display a message that the DLL has been unloaded.
			OnUnloadDllDebugEvent(DbgEvt);
			ContinueDebugEvent(TRUE);
			break;

		case OUTPUT_DEBUG_STRING_EVENT:
			// Display the output debugging string. 
			OnOutputDebugStringEvent(DbgEvt);
			ContinueDebugEvent(TRUE);
			break;

		}

		DebuggeeCtx.pDbgEvent = NULL;
	}
}

VOID FWinDebugger::ContinueDebugEvent(BOOL InbHandled)
{
	if (DebuggeeCtx.pDbgEvent)
	{
		// Resume executing the thread that reported the debugging event. 
		::ContinueDebugEvent(DebuggeeCtx.pDbgEvent->dwProcessId,
			DebuggeeCtx.pDbgEvent->dwThreadId, InbHandled ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED);
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
	switch (InDbgEvent.u.Exception.ExceptionRecord.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		// First chance: Pass this on to the system. 
		// Last chance: Display an appropriate error. 
		if (!DebuggeeCtx.bCatchFirstChanceException && InDbgEvent.u.Exception.dwFirstChance)
		{
			ContinueDebugEvent(FALSE);
			return;
		}
		break;

	case EXCEPTION_BREAKPOINT:
		// First chance: Display the current 
		// instruction and register values. 
		if (!DebuggeeCtx.bCatchFirstChanceException && InDbgEvent.u.Exception.dwFirstChance)
		{
			ContinueDebugEvent(FALSE);
			return;
		}
		break;

	case EXCEPTION_DATATYPE_MISALIGNMENT:
		// First chance: Pass this on to the system. 
		// Last chance: Display an appropriate error. 
		if (!DebuggeeCtx.bCatchFirstChanceException && InDbgEvent.u.Exception.dwFirstChance)
		{
			ContinueDebugEvent(FALSE);
			return;
		}
		break;

	case EXCEPTION_SINGLE_STEP:
		// First chance: Update the display of the 
		// current instruction and register values. 
		break;

	case DBG_CONTROL_C:
		// First chance: Pass this on to the system. 
		// Last chance: Display an appropriate error. 
		if (!DebuggeeCtx.bCatchFirstChanceException && InDbgEvent.u.Exception.dwFirstChance)
		{
			ContinueDebugEvent(FALSE);
			return;
		}
		break;

	default:
		// Handle other exceptions. 
		break;
	}

	DisplayException(InDbgEvent.dwProcessId, InDbgEvent.dwThreadId, InDbgEvent.u.Exception);
	WaitForUserCommand();
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

	// initialize symbol handler
	::SymInitialize(DebuggeeCtx.hProcess, NULL, FALSE);
	DWORD64 RealBaseAddr = SymLoadModule64(DebuggeeCtx.hProcess, InDbgEvent.u.CreateProcessInfo.hFile, NULL, NULL, (DWORD64)InDbgEvent.u.CreateProcessInfo.lpBaseOfImage, 0);
	if (RealBaseAddr)
	{
		appConsolePrintf(TEXT("    Symbol Loaded Successfully.\n"));
	}
	else
	{
		appConsolePrintf(TEXT("    Symbol Loaded Failed.\n"));
	}

	::CloseHandle(InDbgEvent.u.CreateProcessInfo.hFile);
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

	::SymCleanup(DebuggeeCtx.hProcess);
}

VOID FWinDebugger::OnLoadDllDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	const wstring ImageFile = appGetFinalPathNameByHandle(InDbgEvent.u.LoadDll.hFile);

	appConsolePrintf(TEXT("LOAD_DLL_DEBUG_INFO: \n"));
	appConsolePrintf(TEXT("    Image: %s\n"), ImageFile.c_str());
	appConsolePrintf(TEXT("    BaseAddr Of DLL: 0x%08x\n"), InDbgEvent.u.LoadDll.lpBaseOfDll);

	DWORD64 RealBaseAddr = SymLoadModule64(DebuggeeCtx.hProcess, InDbgEvent.u.LoadDll.hFile, NULL, NULL, (DWORD64)InDbgEvent.u.LoadDll.lpBaseOfDll, 0);
	if (RealBaseAddr)
	{
		appConsolePrintf(TEXT("    Symbol Loaded Successfully.\n"));
	}
	else
	{
		appConsolePrintf(TEXT("    Symbol Loaded Failed.\n"));
	}

	CloseHandle(InDbgEvent.u.LoadDll.hFile);
}

VOID FWinDebugger::OnUnloadDllDebugEvent(const DEBUG_EVENT &InDbgEvent)
{
	appConsolePrintf(TEXT("UNLOAD_DLL_DEBUG_INFO: \n"));
	appConsolePrintf(TEXT("    BaseAddr Of DLL: 0x%08x\n"), InDbgEvent.u.UnloadDll.lpBaseOfDll);
	BOOL bSuccess = SymUnloadModule64(DebuggeeCtx.hProcess, (DWORD64)InDbgEvent.u.UnloadDll.lpBaseOfDll);
	if (bSuccess)
	{
		appConsolePrintf(TEXT("    Symbol Unloaded Successfully.\n"));
	}
	else
	{
		appConsolePrintf(TEXT("    Symbol Unloaded Failed.\n"));
	}
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

VOID FWinDebugger::DisplayException(uint32_t InProcessId, uint32_t InThreadId, const EXCEPTION_DEBUG_INFO &InException)
{
	DWORD ExceptionCode = InException.ExceptionRecord.ExceptionCode;

	appConsolePrintf(TEXT("Exception Occurred, PID=%d, TID=%d, bFirstChance=%d:\n"), InProcessId, InThreadId, InException.dwFirstChance);
	appConsolePrintf(TEXT("Exception Code: %s(0x%08x)\n"), GetExceptionCodeDescription(ExceptionCode), ExceptionCode);
	appConsolePrintf(TEXT("EIP: 0x%08x\n"), InException.ExceptionRecord.ExceptionAddress);

	if (ExceptionCode == EXCEPTION_ACCESS_VIOLATION
		|| ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
	{
		const TCHAR *szAction = TEXT("..");
		switch ((uint32_t)(InException.ExceptionRecord.ExceptionInformation[0]))
		{
		case 0:
			szAction = TEXT("READ"); break;
		case 1:
			szAction = TEXT("WRITE"); break;
		case 8:
			szAction = TEXT("DEP"); break;
		default:
			break;
		}
		appConsolePrintf(TEXT("%s memory 0x%08x\n"), szAction, InException.ExceptionRecord.ExceptionInformation[1]);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// USER COMMANDS
const FWinDebugger::FCommandMeta FWinDebugger::sUserCommands[] =
{
	{ TEXT("help"),   TEXT("help"),					   TEXT("help [cmd]"),				     &FWinDebugger::Command_Help },
	{ TEXT("run"),    TEXT("debug a new process"),     TEXT("run filename [param0 param1]"), &FWinDebugger::Command_NewProcess },
	{ TEXT("attach"), TEXT("attach a active process"), TEXT("attach pid"),                   &FWinDebugger::Command_AttachProcess },
	{ TEXT("detach"), TEXT("detach current debuggee"), TEXT("detach"),						 &FWinDebugger::Command_DetachProcess },
	{ TEXT("stop"),   TEXT("ternimate debuggee"),	   TEXT("stop debugging"),				 &FWinDebugger::Command_StopDebug },
	{ TEXT("go"),	  TEXT("continue execute"),        TEXT("go [u]"),						 &FWinDebugger::Command_Go },
	{ TEXT("list"),   TEXT("list system info"),		   TEXT("list [processes, threads, modules, heaps]"), &FWinDebugger::Command_List },
	{ TEXT("registers"), TEXT("dump current thread context"), TEXT("registers"),             &FWinDebugger::Command_DisplayThreadContext},
	{ TEXT("memory"), TEXT("dump debuggee memory"),    TEXT("memory addr bytes"),            &FWinDebugger::Command_DisplayMemory },
	{ TEXT("l"), TEXT("list source code"),             TEXT("l "),                           &FWinDebugger::Command_ListSourceCode }
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

BOOL FWinDebugger::Command_Go(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	BOOL bHandled = TRUE;
	if (InTokens.size() >= 1)
	{
		bHandled = FALSE;
	}

	ContinueDebugEvent(bHandled);
	return TRUE;
}

BOOL FWinDebugger::Command_List(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	if (InTokens.size() != 1)
	{
		return FALSE;
	}

	const wstring &StrSubCmd = InTokens[0];
	DWORD ProcessId = GetProcessId(DebuggeeCtx.hProcess);
	if (!appStricmp(StrSubCmd.c_str(), TEXT("processes")))
	{
		std::vector<FSnapshotTool::FSnapProcessInfo> OutProcesses;
		FSnapshotTool Snapshot(ProcessId, FSnapshotTool::SNAP_PROCESS);

		Snapshot.GetProcessList(OutProcesses);
		for (uint32_t k = 0; k < OutProcesses.size(); k++)
		{
			const FSnapshotTool::FSnapProcessInfo &Entry = OutProcesses[k];

			appConsolePrintf(TEXT("%04d: pid:%8d, ppid:%8d, threads count:%4d, %s\n"), k, Entry.ProcessId, Entry.ParentId, Entry.ThreadsCnt, Entry.ExeFilename.c_str());
		}
	}
	else if (!appStricmp(StrSubCmd.c_str(), TEXT("threads")))
	{
		std::vector<FSnapshotTool::FSnapThreadInfo> OutThreads;
		FSnapshotTool Snapshot(ProcessId, FSnapshotTool::SNAP_THREAD);

		Snapshot.GetThreadList(OutThreads);
		for (uint32_t k = 0; k < OutThreads.size(); k++)
		{
			const FSnapshotTool::FSnapThreadInfo &Entry = OutThreads[k];

			appConsolePrintf(TEXT("%04d: tid:%8d\n"), k, Entry.ThreadId);
		}
	}
	else if (!appStricmp(StrSubCmd.c_str(), TEXT("modules")))
	{
		std::vector<FSnapshotTool::FSnapModuleInfo> OutModules;
		FSnapshotTool Snapshot(ProcessId, FSnapshotTool::SNAP_MODULE);

		Snapshot.GetModuleList(OutModules);
		for (uint32_t k = 0; k < OutModules.size(); k++)
		{
			const FSnapshotTool::FSnapModuleInfo &Entry = OutModules[k];
			
			// display debug symbol loaded information.
			const TCHAR *szSymInfo = TEXT("N/A");
			IMAGEHLP_MODULEW64 ImgModule;
			ImgModule.SizeOfStruct = sizeof(ImgModule);
			BOOL bSuccess = ::SymGetModuleInfo64(DebuggeeCtx.hProcess, (DWORD64)Entry.BaseAddr, &ImgModule);
			if (bSuccess)
			{
				szSymInfo = GetSymTypeString(ImgModule.SymType);
			}
			appConsolePrintf(TEXT("%4d, base addr:0x%p, size:%8d, exe:%s, Symbol:%s\n"), k, Entry.BaseAddr, Entry.BaseSize, Entry.ExeFilename.c_str(), szSymInfo);

		}
	}
	else if (!appStricmp(StrSubCmd.c_str(), TEXT("heaps")))
	{
		std::vector<FSnapshotTool::FSnapHeapInfo> OutHeaps;
		FSnapshotTool Snapshot(ProcessId, FSnapshotTool::SNAP_HEAP);

		Snapshot.GetHeapList(OutHeaps);
		for (uint32_t k = 0; k < OutHeaps.size(); k++)
		{
			const FSnapshotTool::FSnapHeapInfo &Entry = OutHeaps[k];

			appConsolePrintf(TEXT("heap %04d, %s:\n"), k, FSnapshotTool::GetHeapFlagsDesc(Entry.FlagValue));
			for (uint32_t m = 0; m < Entry.Blocks.size(); m++)
			{
				const FSnapshotTool::FSnapHeapBlock &Block = Entry.Blocks[m];
				appConsolePrintf(TEXT("    addr:0x%p, size:%d, hHandle:%d, desc:%s\n"), Block.Address, Block.BlockSize, Block.hHandle, FSnapshotTool::GetHeapBlockFlagsDesc(Block.Flags));
			} // end for m
		} // end for k
	}

	return FALSE;
}

// InTokens: register names
// InSwitchs: -h -b
BOOL FWinDebugger::Command_DisplayThreadContext(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	if (!DebuggeeCtx.pDbgEvent)
	{
		return FALSE;
	}

	
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, DebuggeeCtx.pDbgEvent->dwThreadId);
	if (hThread == NULL)
	{
		return FALSE;
	}

	CONTEXT ThreadContext;
	ThreadContext.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

	if (GetThreadContext(hThread, &ThreadContext))
	{
		// display registers
		if (ThreadContext.ContextFlags & CONTEXT_CONTROL)
		{
			TCHAR szBuffer[64];
			appConsolePrintf(TEXT("ebp:%08x,  cs:%08x, eip:%08x,  ss:%08x, esp:%08x, eflags:%s\n"), 
				ThreadContext.Ebp, ThreadContext.SegCs, ThreadContext.Eip, ThreadContext.SegSs, ThreadContext.Esp, 
				appItoA(ThreadContext.EFlags, szBuffer, 2));
		}

		if (ThreadContext.ContextFlags & CONTEXT_INTEGER)
		{
			appConsolePrintf(TEXT("edi:%08x, esi:%08x, ebx:%08x, edx:%08x, ecx:%08x, eax:%08x\n"), ThreadContext.Edi, ThreadContext.Esi,
				ThreadContext.Ebx, ThreadContext.Edx, ThreadContext.Ecx, ThreadContext.Eax);
		}
		
		if (ThreadContext.ContextFlags & CONTEXT_SEGMENTS)
		{
			appConsolePrintf(TEXT(" gs:%08x,  fs:%08x,  es:%08x,  ds:%08x\n"), ThreadContext.SegGs, ThreadContext.SegFs, ThreadContext.SegEs, ThreadContext.SegDs);
		}

		if (ThreadContext.ContextFlags & CONTEXT_DEBUG_REGISTERS)
		{
			appConsolePrintf(TEXT("dr0:%08x, dr1:%08x, dr2:%08x, dr3:%08x, dr6:%08x, dr7:%08x\n"), ThreadContext.Dr0, ThreadContext.Dr1,
				ThreadContext.Dr2, ThreadContext.Dr3, ThreadContext.Dr6, ThreadContext.Dr7);
		}
	}
	
	CloseHandle(hThread);
	return FALSE;
}

// addr, bytes
BOOL FWinDebugger::Command_DisplayMemory(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	if (!DebuggeeCtx.pDbgEvent || DebuggeeCtx.hProcess == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	if (InTokens.size() < 2)
	{
		return FALSE;
	}

	int32_t DestAddr = appHextoi(InTokens[0].c_str());
	int32_t Bytes = appAtoi(InTokens[1].c_str());

	if (Bytes <= 0)    { Bytes = 20; }
	if (Bytes >= 4096) { Bytes = 4096; }

	const int32_t kBytesPerLine = 20;
	for (int32_t k = 0; k < Bytes;)
	{
		appConsolePrintf(TEXT("%p:"), (void*)DestAddr);
		for (int32_t col = 0; col < kBytesPerLine && k < Bytes; col++, k++, DestAddr++)
		{
			unsigned char Ch;
			if (::ReadProcessMemory(DebuggeeCtx.hProcess, (void*)DestAddr, &Ch, 1, NULL))
			{
				appConsolePrintf(TEXT(" %02X"), Ch);
			}
			else
			{
				appConsolePrintf(TEXT(" ??"));
			}
		} // end for col
		appConsolePrintf(TEXT("\n"));
	} // end for k

	return FALSE;
}

// list source code
BOOL FWinDebugger::Command_ListSourceCode(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
{
	if (!DebuggeeCtx.pDbgEvent || DebuggeeCtx.hProcess == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, DebuggeeCtx.pDbgEvent->dwThreadId);
	if (hThread == NULL)
	{
		return FALSE;
	}

	CONTEXT ThreadContext;
	ThreadContext.ContextFlags = CONTEXT_CONTROL;
	if (GetThreadContext(hThread, &ThreadContext))
	{
		DWORD64 qwAddr = ThreadContext.Eip;
		DWORD dwDisplacement = 0;
		IMAGEHLP_LINE64  Line64;

		Line64.SizeOfStruct = sizeof(Line64);
		if (SymGetLineFromAddr64(DebuggeeCtx.hProcess, qwAddr, &dwDisplacement, &Line64))
		{
			appConsolePrintf(TEXT("list source: eip:%p, displacement:%d, file:%s, line:%d\n"), ThreadContext.Eip, dwDisplacement, Line64.FileName,
				Line64.LineNumber);
		}
		else
		{
			TRACE_ERROR(TEXT("List Source Code"));
		}
	}

	CloseHandle(hThread);
	return FALSE;
}
