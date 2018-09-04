// 在用户态代码中插入INT 1指令会违反保护规则
//

#include <cstdio>

int main(int argc, char *argv[])
{
	printf("Try INT 1!\n");
	
	_asm int 1;

	return 0;
}
