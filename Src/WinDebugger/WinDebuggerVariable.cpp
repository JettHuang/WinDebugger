// \brief
//		WinDebugger Class: implement variables commands.
//

#include "Foundation\AppHelper.h"
#include "WinDebugger.h"
#include "WinProcessHelper.h"
#include "WinVariableTypeHelper.h"

#include <DbgHelp.h>
#include <vector>


// variable information
struct FVariableInfo
{
	uint32_t    TypeIndex;        // Type Index of symbol
	uint32_t    Size;
	uint64_t    ModBase;          // Base Address of module containing this symbol
	uint32_t    Flags;
	uint64_t    Value;            // Value of symbol, ValuePresent should be 1
	uint64_t    Address;          // Address of symbol including base address of module
	uint32_t    Register;         // register holding value or pointer to value
	std::wstring Name;            // Name of symbol
};

struct FSymEnumContext
{
	std::vector<FVariableInfo>  Variables;
};

// variables enum callback
static 
BOOL CALLBACK PsymEnumeratesymbolsCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
	FSymEnumContext *Ctx = reinterpret_cast<FSymEnumContext*>(UserContext);
	if (!Ctx)
	{
		return FALSE;
	}

	if (pSymInfo->Tag != SymTagData)
	{
		return TRUE;
	}

	Ctx->Variables.push_back(FVariableInfo());
	FVariableInfo &SymVariable = Ctx->Variables.back();
	
	SymVariable.TypeIndex = pSymInfo->TypeIndex;
	SymVariable.Size = pSymInfo->Size;
	SymVariable.ModBase = pSymInfo->ModBase;
	SymVariable.Flags = pSymInfo->Flags;
	SymVariable.Value = pSymInfo->Value;
	SymVariable.Address = pSymInfo->Address;
	SymVariable.Register = pSymInfo->Register;
	SymVariable.Name = pSymInfo->Name;

	return TRUE;
}

static std::wstring GetSymFlagsString(uint32_t InFlags)
{
	std::wstring szDesc;

	if (InFlags & SYMFLAG_VALUEPRESENT)
	{
		szDesc += TEXT("SYMFLAG_VALUEPRESENT ");
	}
	if (InFlags & SYMFLAG_REGISTER)
	{
		szDesc += TEXT("SYMFLAG_REGISTER ");
	}
	if (InFlags & SYMFLAG_REGREL)
	{
		szDesc += TEXT("SYMFLAG_REGREL ");
	}
	if (InFlags & SYMFLAG_FRAMEREL)
	{
		szDesc += TEXT("SYMFLAG_FRAMEREL ");
	}
	if (InFlags & SYMFLAG_PARAMETER)
	{
		szDesc += TEXT("SYMFLAG_PARAMETER ");
	}
	if (InFlags & SYMFLAG_LOCAL)
	{
		szDesc += TEXT("SYMFLAG_LOCAL ");
	}
	if (InFlags & SYMFLAG_CONSTANT)
	{
		szDesc += TEXT("SYMFLAG_CONSTANT ");
	}
	if (InFlags & SYMFLAG_EXPORT)
	{
		szDesc += TEXT("SYMFLAG_EXPORT ");
	}
	if (InFlags & SYMFLAG_FORWARDER)
	{
		szDesc += TEXT("SYMFLAG_FORWARDER ");
	}
	if (InFlags & SYMFLAG_FUNCTION)
	{
		szDesc += TEXT("SYMFLAG_FUNCTION ");
	}
	if (InFlags & SYMFLAG_VIRTUAL)
	{
		szDesc += TEXT("SYMFLAG_VIRTUAL ");
	}
	if (InFlags & SYMFLAG_THUNK)
	{
		szDesc += TEXT("SYMFLAG_THUNK ");
	}
	if (InFlags & SYMFLAG_TLSREL)
	{
		szDesc += TEXT("SYMFLAG_TLSREL ");
	}
	if (InFlags & SYMFLAG_SLOT)
	{
		szDesc += TEXT("SYMFLAG_SLOT ");
	}
	if (InFlags & SYMFLAG_ILREL)
	{
		szDesc += TEXT("SYMFLAG_ILREL ");
	}
	if (InFlags & SYMFLAG_METADATA)
	{
		szDesc += TEXT("SYMFLAG_METADATA ");
	}
	if (InFlags & SYMFLAG_CLR_TOKEN)
	{
		szDesc += TEXT("SYMFLAG_CLR_TOKEN ");
	}
	if (InFlags & SYMFLAG_NULL)
	{
		szDesc += TEXT("SYMFLAG_NULL ");
	}
	if (InFlags & SYMFLAG_FUNC_NO_RETURN)
	{
		szDesc += TEXT("SYMFLAG_FUNC_NO_RETURN ");
	}
	if (InFlags & SYMFLAG_SYNTHETIC_ZEROBASE)
	{
		szDesc += TEXT("SYMFLAG_SYNTHETIC_ZEROBASE ");
	}
	if (InFlags & SYMFLAG_PUBLIC_CODE)
	{
		szDesc += TEXT("SYMFLAG_PUBLIC_CODE ");
	}

	return szDesc;
}

static void* CalculateVariableAbsAddress(const FVariableInfo &InVariable, HANDLE InProcess, const CONTEXT &InContext)
{
	if ((InVariable.Flags & SYMFLAG_REGREL) == 0) {
		return (void*)(InVariable.Address);
	}

	//如果当前EIP指向函数的第一条指令，则EBP的值仍然是属于
	//上一个函数的，所以此时不能使用EBP，而应该使用ESP-4作为符号的基地址

	DWORD64 displacement;
	SYMBOL_INFO symbolInfo = { 0 };
	symbolInfo.SizeOfStruct = sizeof(SYMBOL_INFO);

	SymFromAddr(InProcess, InContext.Eip, &displacement, &symbolInfo);
	//如果是函数的第一条指令，则不能使用EBP
	if (displacement == 0) {
		return (void *)(InContext.Esp - 4 + InVariable.Address);
	}

	return (void *)(InContext.Ebp + InVariable.Address);
}

// 显示变量
static VOID DisplayVariables(const std::vector<FVariableInfo> &InVariables, HANDLE InProcess, const CONTEXT &InContext)
{
	for (size_t k = 0; k < InVariables.size(); k++)
	{
		const FVariableInfo &SymVariable = InVariables[k];

#if 0
		appConsolePrintf(TEXT("ModBase:0x%08x, TypeIndex:%d, Size:%d, Flags:0x%08x, Value:0x%08x, Address:0x%08x, Register:%d, Name:%s\n"),
			(uint32_t)SymVariable.ModBase, (uint32_t)SymVariable.TypeIndex, (uint32_t)SymVariable.Size, (uint32_t)SymVariable.Flags,
			(uint32_t)SymVariable.Value, (uint32_t)SymVariable.Address, (uint32_t)SymVariable.Register, SymVariable.Name.c_str());
		appConsolePrintf(TEXT("Flags: %s\n"), GetSymFlagsString(SymVariable.Flags).c_str());
#else
		void *DataAbsAddr = CalculateVariableAbsAddress(SymVariable, InProcess, InContext);
		void *pBuffer = new BYTE[SymVariable.Size];
		if (pBuffer && ReadProcessMemory(InProcess, DataAbsAddr, pBuffer, SymVariable.Size, NULL))
		{
			appConsolePrintf(TEXT("%s"), SymVariable.Name.c_str());
			FSymTypeInfo* pSymTypeInfo = FSymTypeInfoHelper::BuildSymTypeInfo(InProcess, SymVariable.ModBase, SymVariable.TypeIndex);
			if (pSymTypeInfo)
			{
				appConsolePrintf(TEXT("(%s): "), pSymTypeInfo->TypeName().c_str());
				appConsolePrintf(TEXT(" %s\n"), pSymTypeInfo->FormatValue(pBuffer).c_str());
			}
		}
		delete pBuffer;
#endif
	} // end for k
}

// list global variables in current module.
BOOL FWinDebugger::Command_ListGlobalVariables(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
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
		DWORD64 ModuleBaseAddr = SymGetModuleBase64(DebuggeeCtx.hProcess, ThreadContext.Eip);
		if (!ModuleBaseAddr)
		{
			TRACE_ERROR(TEXT("SymGetModuleBase64 Failed."));
		}
		else
		{
			const TCHAR *szExpression = NULL;
			if (InTokens.size() >= 1)
			{
				szExpression = InTokens[0].c_str();
			}

			FSymEnumContext EnumCtx;
			if (SymEnumSymbols(DebuggeeCtx.hProcess, ModuleBaseAddr, szExpression, &PsymEnumeratesymbolsCallback, (void*)&EnumCtx))
			{
				DisplayVariables(EnumCtx.Variables, DebuggeeCtx.hProcess, ThreadContext);
			}
			else
			{
				TRACE_ERROR(TEXT("List global variables."));
			}
		}
	}

	CloseHandle(hThread);
	return FALSE;
}

BOOL FWinDebugger::Command_ListLocalVariables(const vector<wstring> &InTokens, const vector<wstring> &InSwitchs)
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
		IMAGEHLP_STACK_FRAME StackFrame = { 0 };
		StackFrame.InstructionOffset = ThreadContext.Eip;

		if (!SymSetContext(DebuggeeCtx.hProcess, &StackFrame, NULL) && (GetLastError() != ERROR_SUCCESS))
		{
			TRACE_ERROR(TEXT("SymSetContext Failed."));
		}
		else
		{
			const TCHAR *szExpression = NULL;
			if (InTokens.size() >= 1)
			{
				szExpression = InTokens[0].c_str();
			}

			FSymEnumContext EnumCtx;
			if (SymEnumSymbols(DebuggeeCtx.hProcess, 0, szExpression, &PsymEnumeratesymbolsCallback, (void*)&EnumCtx))
			{
				DisplayVariables(EnumCtx.Variables, DebuggeeCtx.hProcess, ThreadContext);
			}
			else
			{
				TRACE_ERROR(TEXT("List local variables."));
			}
		}
	}

	CloseHandle(hThread);
	return FALSE;
	return FALSE;
}

