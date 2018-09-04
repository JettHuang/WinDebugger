// 手工设置数据断点(通过调试寄存器进行debug)
//

#include <cstdio>
#include <cstdlib>
#include <Windows.h>


int main(int argc, char *argv[])
{
	CONTEXT cxt;
	HANDLE hThread = GetCurrentThread();
	DWORD dwTestVar = 0;

	if (!IsDebuggerPresent())
	{
		printf("This sample can only run within a debugger.\n");
		return -1;
	}
	
	cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_FULL;
	if (!GetThreadContext(hThread, &cxt))
	{
		printf("Failed to get thread context.\n");
		return -1;
	}

	cxt.Dr0 = (DWORD)&dwTestVar;
	cxt.Dr7 = 0xF0001; //4 bytes length read& write breakponits
	if (!SetThreadContext(hThread, &cxt))
	{
		printf("Failed to set thread context.\n");
		return -1;
	}

	dwTestVar = 1;
	GetThreadContext(hThread, &cxt);
	printf("Break into debuger with DR6=%X.\n", cxt.Dr6);
	dwTestVar = 2;

	return 0;
}
