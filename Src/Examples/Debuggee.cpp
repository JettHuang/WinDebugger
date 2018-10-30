// WinDebugger测试用例
//

#include <cstdio>
#include <Windows.h>


#define VAR_WATCH() printf("nDividend=%d, nDivisor=%d, nResult=%d.\n", nDividend, nDivisor, nResult)

struct FWorld
{
	char Name[128];
	int ActorCount;
};

enum Keybord
{
	Key_A = 0,
	Key_B,
	Key_C,
	Key_Max
};

struct FNode
{
	FNode *_prev;
	int   Value;
};

typedef int   MyInt;

typedef void(*PtrCallback)(int a, int b);


int main(int argc, char *argv[])
{
	FNode node;

	FWorld world = { 0 };
	Keybord key = Key_A;
	MyInt X = 100;
	PtrCallback PtrFunc = NULL;

	int nDividend = 22, nResult = 100;
	int nDivisor = 0;

	OutputDebugString(TEXT("Hi, Debugger!!"));

	__try
	{
		printf("Before div in __try block:");
		VAR_WATCH();

		// nResult = nDividend / nDivisor;
		__asm
		{
			mov eax, nDividend
			cdq
			idiv nDivisor
			mov nResult, eax
		};

		printf("After div in __try block:");
		VAR_WATCH();
	}
	__except (printf("In __except block:"), VAR_WATCH(),
		GetExceptionCode() == EXCEPTION_INT_DIVIDE_BY_ZERO ?
		(nDivisor = 1, printf("Divide Zero exception detected:"), VAR_WATCH(), EXCEPTION_CONTINUE_EXECUTION) // 进行了异常处理, 并继续恢复执行
		: EXCEPTION_CONTINUE_SEARCH) // 继续查找异常处理
	{
		printf("In handler block. Never execute me.\n"); // EXCEPTION_EXECUTE_HANDLER
	}

	getchar(); // pause.
	return 0;
}
