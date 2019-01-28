/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[MAX_BS];
/*-------------------------------------------------------------------------
 *  * init_bsm- initialize bsm_tab
 *   *-------------------------------------------------------------------------
 *    */
SYSCALL init_bsm() {

	int i;
	for (i = 0; i < MAX_BS; i++) {

		bsm_tab[i].ispriv = 0;
	        bsm_tab[i].bs_npages = 0;
	        bsm_tab[i].bs_pid = -1;
	        bsm_tab[i].bs_refcnt = 0;
     		bsm_tab[i].bs_sem = -1;
       		bsm_tab[i].bs_status = UNMAPPED;
	        bsm_tab[i].bs_vpno = 4096;		
	}
	return OK;
}

/*-------------------------------------------------------------------------
 *  * get_bsm - get a free entry from bsm_tab 
 *   *-------------------------------------------------------------------------
 *    */

SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
	disable(ps);
	int i = 0;
	for(i = 0; i < MAX_BS; i++)
		{
		if(bsm_tab[i].bs_status == UNMAPPED)
			{
			*avail = i;
			restore(ps);
			return OK;		
			}
		}
	restore(ps);
	return SYSERR;
}
/*
SYSCALL get_bsm() {

	int i;
	int temp;
	for (i = 0; i < MAX_BS; i++) {
		if (bsm_tab[i].bs_status == UNMAPPED)
	//		temp = i;
			return i;
	}
	return -1;

}
*/

/*-------------------------------------------------------------------------
 *  * free_bsm - free an entry from bsm_tab 
 *   *-------------------------------------------------------------------------
 *    */
SYSCALL free_bsm(int i) {
	if (bsm_tab[i].bs_refcnt != 0 || i<0 || i >= MAX_BS) {
		return SYSERR;
	}

	bsm_tab[i].ispriv = 0;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_pid = -1;
        bsm_tab[i].bs_refcnt = 0;
        bsm_tab[i].bs_sem = -1;
        bsm_tab[i].bs_status = UNMAPPED;
        bsm_tab[i].bs_vpno = -1;

	return OK;

}

/*-------------------------------------------------------------------------
 *  * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *   *-------------------------------------------------------------------------
 *    */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth) {

	unsigned int i;
	unsigned int offset = (vaddr / NBPG) & 0x000fffff;

	for (i = 0; i < MAX_BS; i++) {
		bs_map_t *bsptr = &proctab[pid].bs_list[i];
		if ((bsptr->bs_status == MAPPED) && (bsptr->bs_vpno + bsptr->bs_npages > offset) && (bsptr->bs_vpno <= offset)) {
			*store = i;
			*pageth = offset - bsptr->bs_vpno;
			return OK;
		}
	}
	*store = -1;
	*pageth = -1;
	return SYSERR;
}

/*-------------------------------------------------------------------------
 *  * bsm_map - add an mapping into bsm_tab 
 *   *-------------------------------------------------------------------------
 *    */
SYSCALL bsm_map(int pid, int vpno, int source, int npages) {

	if (vpno < NBPG || (source < 0 ) || ( source > MAX_BS) ||(npages < 1) || ( npages >128)) {
		return SYSERR;
	}

	if (bsm_tab[source].bs_status == UNMAPPED) {
		bsm_tab[source].bs_status = MAPPED;
		bsm_tab[source].bs_npages = npages;
	}

	if (proctab[pid].bs_list[source].bs_status == UNMAPPED)
		bsm_tab[source].bs_refcnt++;

	proctab[pid].bs_list[source].ispriv = 0;
	proctab[pid].bs_list[source].bs_npages = npages;
	proctab[pid].bs_list[source].bs_pid = pid;
	proctab[pid].bs_list[source].bs_refcnt = 0;
	proctab[pid].bs_list[source].bs_sem = -1;
	proctab[pid].bs_list[source].bs_status = MAPPED;
	proctab[pid].bs_list[source].bs_vpno = vpno;
	return OK;

}

/*-------------------------------------------------------------------------
 *  * bsm_unmap - delete an mapping from bsm_tab
 *   *-------------------------------------------------------------------------
 *    */
SYSCALL bsm_unmap(int pid, int vpno, int flag) {

	if (vpno < NBPG) {
		return SYSERR;
	}

	int store, pageth;
	if (bsm_lookup(pid, (vpno * NBPG), &store, &pageth) == SYSERR || (store == -1) || (pageth == -1)) {
		return SYSERR;
	}


	if (flag == PRIVATE_HEAP) {
		proctab[pid].vmemlist = NULL;
	} else {
		int temp = vpno;
		while (proctab[pid].bs_list[store].bs_npages + proctab[pid].bs_list[store].bs_vpno > temp) {
			unsigned long a = temp * NBPG;
			unsigned long pg_offset;
			virt_addr_t* virt;
			virt = (virt_addr_t*)&a;
			pg_offset = virt->pg_offset/1023;

			pd_t *pd = proctab[pid].pdbr + (a >> 22) * sizeof(pd_t);
			pt_t *pt = (pd->pd_base) * NBPG + sizeof(pt_t) * pg_offset;

			if (frm_tab[pt->pt_base - FRAME0].fr_status == MAPPED)
				free_frm(pt->pt_base - FRAME0);

			if (pd->pd_pres == 0)
				break;
			temp++;
		}
	}

	proctab[pid].bs_list[store].ispriv = 0;
	proctab[pid].bs_list[store].bs_npages = 0;
	proctab[pid].bs_list[store].bs_pid = -1;
	proctab[pid].bs_list[store].bs_refcnt = 0;
	proctab[pid].bs_list[store].bs_sem = -1;
	proctab[pid].bs_list[store].bs_status = UNMAPPED;
	proctab[pid].bs_list[store].bs_vpno = -1;

	bsm_tab[store].bs_refcnt--;
	return OK;
}
