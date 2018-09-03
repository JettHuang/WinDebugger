// exception: 从用户空间访问内核空间
//

#include <cstdio>

int main(int argc, char *argv[])
{
	printf("before to access kernel space.\n");

	*(int *)0xA0808080 = 0x01;

	printf("never run to here.\n");
	
	return 0;
}
