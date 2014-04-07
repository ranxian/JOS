// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	// cprintf("%p, eip=%p\n", utf->utf_err, utf->utf_eip);
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;
	int pn = PGNUM(addr);
	void *rounded = ROUNDDOWN(addr, PGSIZE);

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	
	if (!(utf->utf_err&FEC_WR))
		panic("pgfault: Not a write access");
	if (!((uvpt[pn]&PTE_P) && (uvpt[pn]&PTE_COW)))
		panic("pgfault: Not COW page");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	if ((r = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	memmove(PFTEMP, rounded, PGSIZE);
	if ((r = sys_page_map(0, PFTEMP, 0, rounded, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	pte_t pte;
	pte = uvpt[pn];
	void *va = (void *)(pn*PGSIZE);

	if (pte & PTE_P) {
		// cprintf("here\n");
		if (pte & PTE_SHARE) {
			cprintf("a share page\n");
			if ((r = sys_page_map(0, va, envid, va,
			 PGOFF(pte) & (PTE_P|PTE_U|PTE_W|PTE_AVAIL))) < 0)
			{
				panic("sys_page_map: %e", r);
			}
		} else if ((pte&PTE_W) || (pte&PTE_COW)) {
			if ((r = sys_page_map(0, va, envid, va, PTE_P|PTE_U|PTE_COW)) < 0)
				panic("sys_page_map: %e", r);
			if ((r = sys_page_map(0, va, 0, va, PTE_P|PTE_U|PTE_COW)) < 0)
				panic("sys_page_map: %e", r);
		} else {
			if ((r = sys_page_map(0, va, envid, va, PTE_P|PTE_U)) < 0)
				panic("sys_page_map: %e", r);
		}
	}
	
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	extern void _fault_upcall(void);
	// LAB 4: Your code here.
	envid_t envid;
	int r;
	extern unsigned char end[];
	uint8_t *addr;
	uintptr_t va;
	// Set pagefault handler
	set_pgfault_handler(pgfault);
	// fork
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		// We're the child.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// We're the parent
	// Map pages
	for (va = 0; va < UXSTACKTOP-PGSIZE-PGSIZE; va += PGSIZE) {
		if (uvpd[PDX(va)] & PTE_P)
			duppage(envid, PGNUM(va));
	}

	// Setup page fault handler
	sys_env_set_fault_upcall(envid, _fault_upcall);
	sys_env_set_fault_handler(envid, T_PGFLT, pgfault);
	// Alloc UXSTACK
	if ((r = sys_page_alloc(envid, (void *)UXSTACKTOP-PGSIZE, PTE_P|PTE_U|PTE_W)) < 0)
			panic("sys_page_alloc: %e", r);
	// Start child, set it runnable
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
