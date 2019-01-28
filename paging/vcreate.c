/* vcreate.c - vcreate */
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
 *  * static unsigned long esp;
 *   * */

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  *  *  create  -  create a process to start running a procedure
 *   *   *------------------------------------------------------------------------
 *    *    */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);
	int pid;
	pid = create(procaddr,ssize,priority,name,nargs,args);
	int bs_store;
	if(get_bsm(&bs_store) == SYSERR)
		return SYSERR;
	if (bsm_map(pid, 4096, bs_store, hsize) == SYSERR) {
                restore(ps);
                return SYSERR;
        }	
	bsm_tab[bs_store].bs_npages = hsize;
	bsm_tab[bs_store].bs_pid = pid;
	bsm_tab[bs_store].bs_sem = 0;
	bsm_tab[bs_store].bs_status = MAPPED;
	bsm_tab[bs_store].bs_vpno = 4096;
	bsm_tab[bs_store].ispriv = 1;
			
	proctab_init(pid, bs_store, hsize);	
	struct mblock *mbptr;
	mbptr = BACKING_STORE_BASE+(bs_store*BACKING_STORE_UNIT_SIZE);
	mbptr->mlen = hsize*NBPG;
	mbptr->mnext = NULL;

	restore(ps);
	return pid;
}

void proctab_init(int pid, int arg1, int arg2){
	int i = pid;
	struct pentry* pptr;
	pptr = &proctab[i]; 
	pptr->store = arg1;
	pptr->vhpnpages = arg2;
	pptr->vhpno = 4096;
	pptr->vmemlist->mnext = 4096*NBPG;
}
/*------------------------------------------------------------------------
 *  *  * newpid  --  obtain a new (free) process id
 *   *   *------------------------------------------------------------------------
 *    *    */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}




/*
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>


LOCAL	newpid();

SYSCALL vcreate(procaddr, ssize, hsize, priority, name, nargs, args)
	int *procaddr; 
	int ssize; 
	int hsize; 
	int priority; 
	char *name; 
	int nargs; 
	long args; 

{
	STATWORD ps;
	disable(ps);

	int source = 0;
	int pid = create(procaddr, ssize, priority, name, nargs, args);

        if ((get_bsm(&source)==SYSERR) || hsize<0 || hsize>128) {
                restore(ps);
                return SYSERR;
        }
	proctab[pid].vhpno = 4096;
        proctab[pid].vhpnpages = hsize;
	
	if (bsm_map(pid, 4096, source, hsize) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	bsm_tab[source].ispriv = PRIVATE_HEAP;
	bsm_tab[source].bs_refcnt = 1;

	proctab[pid].store = source;
	proctab[pid].vhpno = 4096;
	proctab[pid].vhpnpages = hsize;
	proctab[pid].bs_list[source].ispriv = PRIVATE_HEAP;

	struct mblock *heap_begin=(struct mblock*)((4096)<<12);
	proctab[pid].vmemlist->mlen=NBPG*hsize;
  	proctab[pid].vmemlist->mnext=heap_begin;
	
	restore(ps);
	return pid;
}
*/
/*
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;	
	int	ssize;	
	int	hsize;		
	int	priority;		
	char	*name;			
	int	nargs;			
	long	args;			
					
{
	STATWORD ps;
	disable(ps);
	int pid;
	pid = create(procaddr,ssize,priority,name,nargs,args);
	int bs_store = get_bsm();
	if(bs_store == SYSERR)
		return SYSERR;

//	bsm_tab[bs_store].bs_npages = hsize;
//	bsm_tab[bs_store].bs_pid = pid;
//	bsm_tab[bs_store].bs_sem = 0;
//	bsm_tab[bs_store].bs_status = MAPPED;
//	bsm_tab[bs_store].bs_vpno = 4096;
	bsm_tab[bs_store].ispriv = PRIVATE_HEAP;
	bsm_tab[bs_store].bs_refcnt = 1;		
	proctab_init(pid, bs_store, hsize);
	proctab[pid].bs_list[bs_store].ispriv = PRIVATE_HEAP;	
//	struct mblock *mbptr;
//	mbptr = BACKING_STORE_BASE+(bs_store*BACKING_STORE_UNIT_SIZE);

	proctab[pid].vmemlist = getmem(sizeof(struct mblock));
	proctab[pid].vmemlist->mlen = hsize * 4096;
	proctab[pid].vmemlist->mnext = NULL;
//	mbptr->mlen = hsize*NBPG;
//	kprintf("mbptr->mlen: %d\n",mbptr->mlen);
//	mbptr->mnext = NULL;

	restore(ps);
	return pid;
}

void proctab_init(int pid, int arg1, int arg2){
	int i = pid;
	struct pentry* pptr;
	pptr = &proctab[i]; 
	pptr->store = arg1;
	pptr->vhpnpages = arg2;
	pptr->vhpno = 4096;
//	proctab[pid].bs_list[source].ispriv = PRIVATE_HEAP;
//	pptr->vmemlist->mnext = 4096*NBPG;
}

*/
/*------------------------------------------------------------------------
 *  * newpid  --  obtain a new (free) process id
 *   *------------------------------------------------------------------------
 *    */
/*
LOCAL	newpid()
{
	int	pid;			
	int	i;

	for (i=0 ; i<NPROC ; i++) {	
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
*/
