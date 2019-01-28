/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 *  * xmmap - xmmap
 *   *-------------------------------------------------------------------------
 *    */
SYSCALL xmmap(int virtpage, bsd_t source, int npages) {

	STATWORD ps;
	if ((virtpage < 4096) || (source < 0) || (source > MAX_BS) || (npages < 1) || (npages > 128)) {
		return SYSERR;
	}

	disable(ps);

	bs_map_t *bsptr = &bsm_tab[source];
	if ((bsptr->bs_status == UNMAPPED) || (bsptr->ispriv == PRIVATE_HEAP)) {
		restore(ps);
		return OK;
	}

	if (npages > bsm_tab[source].bs_npages) {
		restore(ps);
		return SYSERR;
	}

	if (bsm_map(currpid, virtpage, source, npages) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 *  * xmunmap - xmunmap
 *   *-------------------------------------------------------------------------
 *    */
SYSCALL xmunmap(int virtpage) {
	STATWORD ps;
	if ((virtpage < 4096)) {
		return SYSERR;
	}
	disable(ps);
	if (bsm_unmap(currpid, virtpage, 0) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	write_cr3(proctab[currpid].pdbr);
	restore(ps);
	return OK;
}
