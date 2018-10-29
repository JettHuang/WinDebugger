// \brief
//		Win Debugger class.
//
// ref: https://docs.microsoft.com/zh-cn/windows/desktop/Debug/basic-debugging
//

#pragma once

#include <Windows.h>
#include <string>
#include <vector>

using namespace std;


class FWinDebugger
{
public:
	FWinDebugger();
	~FWinDebugger();

	VOID MainLoop();

protected:
	// create a debuggee process
	BOOL DebugNewProcess(const TCHAR *InExeFilename, const TCHAR *InParams);
	// attach to an active process and debug it
	BOOL DebugActiveProcess(DWORD ProcessId);
	// detach debuggee process
	BOOL DebugActiveProcessStop(DWORD ProcessId);
	// kill debuggee
	BOOL DebugKillProcess(HANDLE hProcess);
	// continue debuggee
	VOID ContinueDebugEvent(BOOL InbHandled);

	// Debug Event Handler
	VOID OnExceptionDebugEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnCreateThreadDebugEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnCreateProcessDebugEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnExitThreadDebugEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnExitProcessDebugEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnLoadDllDebugEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnUnloadDllDebugEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnOutputDebugStringEvent(const DEBUG_EVENT &InDbgEvent);
	VOID OnRipEvent(const DEBUG_EVENT &InDbgEvent);

	// display exception brief information.
	VOID DisplayException(uint32_t InProcessId, uint32_t InThreadId, const EXCEPTION_DEBUG_INFO &InException);

	// user interaction
	VOID WaitForUserCommand();
	// dispatch user command
	// return  TRUE: stop wait next user command. FALSE: continue wait next user command
	BOOL DispatchUserCommand(const wstring &InCmd, const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	// user command handlers
	BOOL Command_Help(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_NewProcess(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_AttachProcess(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_DetachProcess(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_StopDebug(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_Go(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_List(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_DisplayThreadContext(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_DisplayMemory(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	BOOL Command_ListSourceCode(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);

	// command meta
	typedef BOOL(FWinDebugger::*PtrCommandFunction)(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs);
	struct FCommandMeta
	{
		const TCHAR*	mName;
		const TCHAR*	mDesc;
		const TCHAR*	mUsage;
		PtrCommandFunction	mFunc;
	};
	
	// DebuggeeContext
	struct FDebuggeeContext
	{
		HANDLE				 hProcess;
		const DEBUG_EVENT   *pDbgEvent;
		BOOL				 bCatchFirstChanceException;

		void Reset()
		{
			hProcess = INVALID_HANDLE_VALUE;
			pDbgEvent = NULL;
			bCatchFirstChanceException = FALSE;
		}
	};

protected:
	FDebuggeeContext	DebuggeeCtx;

	// user commands table
	static const FCommandMeta sUserCommands[];
};

