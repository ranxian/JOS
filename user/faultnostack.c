// test user fault handler being called with no exception stack mapped

#include <inc/lib.h>

void _fault_upcall();

void
umain(int argc, char **argv)
{
	sys_env_set_fault_upcall(0, (void*) _fault_upcall);
	*(int*)0 = 0;
}
