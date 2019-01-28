#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t store, unsigned int npages) {

	STATWORD ps;
	if ((store < 0 ) || ( store > MAX_BS) ||(npages < 1) || ( npages >128)) {
		return SYSERR;
	}

	disable(ps);

	if (bsm_tab[store].bs_status == UNMAPPED) {
		if (bsm_map(currpid, 4096, store, npages) == SYSERR) {
			restore(ps);
			return SYSERR;
		}
		restore(ps);
		return npages;
	} else {
		restore(ps);
		return bsm_tab[store].bs_npages;
	}
}


