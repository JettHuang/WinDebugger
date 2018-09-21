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

	// Debug Event Handler
	VOID OnExceptionDebugEvent(const LPDEBUG_EVENT);
	VOID OnCreateThreadDebugEvent(const LPDEBUG_EVENT);
	VOID OnCreateProcessDebugEvent(const LPDEBUG_EVENT);
	VOID OnExitThreadDebugEvent(const LPDEBUG_EVENT);
	VOID OnExitProcessDebugEvent(const LPDEBUG_EVENT);
	VOID OnLoadDllDebugEvent(const LPDEBUG_EVENT);
	VOID OnUnloadDllDebugEvent(const LPDEBUG_EVENT);
	VOID OnOutputDebugStringEvent(const LPDEBUG_EVENT);
	VOID OnRipEvent(const LPDEBUG_EVENT);

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
		const LPDEBUG_EVENT *pDbgEvent;

		void Reset()
		{
			hProcess = INVALID_HANDLE_VALUE;
			pDbgEvent = NULL;
		}
	};

protected:
	FDebuggeeContext	DebuggeeCtx;

	// user commands table
	static const FCommandMeta sUserCommands[];
};

