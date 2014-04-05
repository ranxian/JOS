// buggy program - causes a divide by zero exception, and handle it

#include <inc/lib.h>

int zero;

void
handler(struct UTrapframe *utf)
{
	cprintf("What a shame! I divide zero!\n");
	exit();
}

void
umain(int argc, char **argv)
{
	zero = 0;
	set_fault_handler(T_DIVIDE, handler);
	cprintf("1/0 is %08x!\n", 1/zero);
}

