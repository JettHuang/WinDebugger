// 在代码中插入断点指令
//

#include <cstdio>
#include <Windows.h>


int main(int argc, char *argv[])
{
	printf("before int 3.\n");

	// manual breakpoint
	__asm INT 3;
	printf("Hello INT 3!\n");
	
	__asm
	{
		mov eax, eax
		_emit 0xcd         // int 3指令
		_emit 0x03		   // ie: 0x013717DC 处(位于 Example_HiInt3_d.exe 中)引发的异常: 0xC0000005: 读取位置 0xFFF48B9D 时发生访问冲突。
		nop
		nop
	}

	// or use windows API
	DebugBreak();

	return 0;
}
