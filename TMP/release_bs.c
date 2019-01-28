#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {
	STATWORD ps;
	if (bs_id < 0 || bs_id > MAX_BS) {
		return SYSERR;
	}
	disable(ps);
	free_bsm(bs_id);
	write_cr3(proctab[currpid].pdbr);
	restore(ps);
	return OK;
}


