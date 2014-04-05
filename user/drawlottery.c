#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int count = 0;
	sys_env_set_lottery(sys_getenvid(), 1);
	if (fork() == 0) {
		// child
		sys_env_set_lottery(sys_getenvid(), 10);
		while (true) {
			cprintf("child counts to %d\n", count);
			sys_yield();
			count += 1;
		}
	}

	while (true) {
		cprintf("parent counts to %d\n", count);
		sys_yield();
		count += 1;
	}
}