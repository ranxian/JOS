// User-level page fault handler support.
// Rather than register the C page fault handler directly with the
// kernel as the page fault handler, we register the assembly language
// wrapper in pfentry.S, which in turns calls the registered C
// function.

#include <inc/lib.h>


// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _fault_upcall(void);

// Pointer to currently installed C-language pgfault handler.
//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _pgfault_upcall routine when a page fault occurs.
//

void set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
{
	set_fault_handler(T_PGFLT, handler);
}

void set_fault_handler(int faultno, void (*handler)(struct UTrapframe *utf))
{
	int r;
	envid_t envid;
	static int uxstack_mapped = 0;
	envid = sys_getenvid();

	if (uxstack_mapped == 0 ) {
		if ((r = sys_page_alloc(envid, (void *)UXSTACKTOP-PGSIZE, PTE_P|PTE_U|PTE_W)) < 0)
			panic("sys_page_alloc: %e", r);
		uxstack_mapped = 1;
		sys_env_set_fault_upcall(envid, _fault_upcall);
	}

	sys_env_set_fault_handler(envid, faultno, handler);
}