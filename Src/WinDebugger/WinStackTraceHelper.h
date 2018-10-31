// \brief
//		capture stack trace 
//

#pragma once

#include <Windows.h>
#include <dbghelp.h>
#include <string>


class FWinStackTraceHelper
{
public:
	static INT CaptureStackTrace(HANDLE InProcess, HANDLE InThread, const CONTEXT &InContext, DWORD64* OutBackTrace, DWORD InMaxDepth);
	static std::wstring ProgramCounterToSymbolInfo(HANDLE InProcess, DWORD64 InProgramCounter);
};

