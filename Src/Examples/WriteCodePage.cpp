// 写代码段导致非法访问异常
//

#include <cstdio>
#include <Windows.h>


int main(int argc, char *argv[])
{
	printf("before to access code pages.\n");

	HINSTANCE hLib;
	int *ProcAddr = NULL;

	hLib = LoadLibrary(TEXT("Kernel32.dll"));
	if (hLib != NULL)
	{
		ProcAddr = (int *)GetProcAddress(hLib, "ReadFile");
		if (ProcAddr != NULL)
		{
			*ProcAddr = 0;
		}
	}

	printf("never run to here.\n");

	return 0;
}