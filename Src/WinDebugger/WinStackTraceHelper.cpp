// \brief
//		capture stack trace
//

#include "WinStackTraceHelper.h"
#include "Foundation/AppHelper.h"

#include <sstream>
#include <iomanip>


INT FWinStackTraceHelper::CaptureStackTrace(HANDLE InProcess, HANDLE InThread, const CONTEXT &InContext, DWORD64* OutBackTrace, DWORD InMaxDepth)
{
	STACKFRAME64	StackFrame64;
	BOOL			bStackWalkSucceeded = TRUE;
	DWORD			CurrentDepth = 0;
	DWORD			MachineType = IMAGE_FILE_MACHINE_I386; // x86
	CONTEXT			ContextCopy = InContext;

	__try
	{
		// zero out stack frame.
		memset(&StackFrame64, 0, sizeof(StackFrame64));

		StackFrame64.AddrPC.Mode    = AddrModeFlat;
		StackFrame64.AddrStack.Mode = AddrModeFlat;
		StackFrame64.AddrFrame.Mode = AddrModeFlat;
		// win32
		StackFrame64.AddrPC.Offset = ContextCopy.Eip;
		StackFrame64.AddrStack.Offset = ContextCopy.Esp;
		StackFrame64.AddrFrame.Offset = ContextCopy.Ebp;

		while (CurrentDepth < InMaxDepth)
		{
			bStackWalkSucceeded = StackWalk64(MachineType, InProcess, InThread, &StackFrame64, &ContextCopy,
											  NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL);
			if (!bStackWalkSucceeded)
			{
				break;
			}
			// stop if the frame pointer PC is null
			if (StackFrame64.AddrFrame.Offset == 0 || StackFrame64.AddrPC.Offset == 0)
			{
				break;
			}

			OutBackTrace[CurrentDepth++] = StackFrame64.AddrPC.Offset;
		} // end while
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		TRACE_ERROR(TEXT("StackWalk64 Exception Occured."));
	}

	for (DWORD k=CurrentDepth; k<InMaxDepth; k++)
	{
		OutBackTrace[k] = NULL;
	}

	return CurrentDepth;
}

std::wstring FWinStackTraceHelper::ProgramCounterToSymbolInfo(HANDLE InProcess, DWORD64 InProgramCounter)
{
	std::wostringstream    SymbolDescBuilder;

	const INT kMaxNameLength = 512;
	CHAR  SymbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + kMaxNameLength];
	PIMAGEHLP_SYMBOL64 Symbol = NULL;
	DWORD SymbolDisplacement = 0;
	DWORD64 SymbolDisplacement64 = 0;

	Symbol = (PIMAGEHLP_SYMBOL64)SymbolBuffer;
	Symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	Symbol->MaxNameLength = kMaxNameLength;

	// get symbol from address
	if (SymGetSymFromAddr64(InProcess, InProgramCounter, &SymbolDisplacement64, Symbol))
	{

		// skip any funky chars in the beginning of a function name.
		INT Offset = 0;
		while (Symbol->Name[Offset] < 32 || Symbol->Name[Offset] > 127)
		{
			Offset++;
		}

		TCHAR  szFunctionName[kMaxNameLength] = { 0 };
		appANSIToTCHAR((const CHAR*)(Symbol->Name + Offset), szFunctionName, XARRAY_COUNT(szFunctionName));

		SymbolDescBuilder << szFunctionName;
	}
	else
	{
		//TRACE_ERROR(TEXT("ProgramCounterToSymbolInfo:"));
	}

	// Filename:Line
	{
		DWORD dwDisplacement = 0;
		IMAGEHLP_LINE64  Line64;

		Line64.SizeOfStruct = sizeof(Line64);
		if (SymGetLineFromAddr64(InProcess, InProgramCounter, &dwDisplacement, &Line64))
		{
			SymbolDescBuilder << TEXT("      #") << Line64.FileName << TEXT(":") << Line64.LineNumber;
		}
		else
		{
			SymbolDescBuilder << TEXT(" N/A:??");
			//TRACE_ERROR(TEXT("ProgramCounterToSymbolInfo:"));
		}
	}

	// get module information from address
	{
		IMAGEHLP_MODULE64 ImageHelpModule;
		ImageHelpModule.SizeOfStruct = sizeof(ImageHelpModule);

		if (SymGetModuleInfo64(InProcess, InProgramCounter, &ImageHelpModule))
		{
			SymbolDescBuilder << TEXT(" @") << ImageHelpModule.ImageName;
		}
	}

	return SymbolDescBuilder.str();
}

