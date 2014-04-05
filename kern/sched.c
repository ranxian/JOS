#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield_round_robin(void)
{
	struct Env *idle = NULL;
	int cur_envx, idle_envx, i;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.

	if (curenv != NULL) {
		cur_envx = ENVX(curenv->env_id);
		if (curenv->env_status == ENV_RUNNING)
			curenv->env_status = ENV_RUNNABLE;
	} else {
		cur_envx = 0;
	}

	for (i = 0; i < NENV; i++) {
		// Get env from next env, and will return curenv itself 
		// if no runnable env.
		idle_envx = (cur_envx+i+1) % NENV;
		if (envs[idle_envx].env_status == ENV_RUNNABLE) {
			idle = &envs[idle_envx];
			break;
		}
	}

	if (idle != NULL)
		env_run(idle);

	// sched_halt never returns
	sched_halt();
}

// may not return
static void
draw_lottery(struct Env *runnable_envs[], int nenv)
{
	if (nenv <= 0)
		panic("draw_lottery: should not pass nenv=%d", nenv);

	static unsigned lottery_mul = 214013;
	static unsigned lottery_add = 2531011;
	static unsigned lottery_seed = 234;
	int pos[NENV] = {0};
	int i, sum;

	for (i = 0; i < nenv; i++) {
		pos[i+1] = pos[i] + runnable_envs[i]->env_lottery;
	}
	sum = pos[i];

	lottery_seed = lottery_seed * lottery_mul + lottery_add;
	int winner = lottery_seed % sum;;

	for (i = 0; i < nenv; i++) {
		if (winner >= pos[i] && winner < pos[i+1])
			env_run(runnable_envs[i]);
	}

	panic("draw_lottery: should not return when nenv=%d", nenv);
}

void
sched_yield(void)
{
	int i, nrunnable = 0;
	struct Env *runnable_envs[NENV];

	if (curenv != NULL && curenv->env_status == ENV_RUNNING)
		curenv->env_status = ENV_RUNNABLE;

	for (i = 0; i < NENV; i++) {
		if (envs[i].env_status == ENV_RUNNABLE) {
			runnable_envs[nrunnable++] = &envs[i];
		}
	}

	if (nrunnable != 0)
		draw_lottery(runnable_envs, nrunnable);

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"hlt\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

