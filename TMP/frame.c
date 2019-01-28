#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
#define TRUE	1
#define FALSE	-1
extern int page_replace_policy;

struct cir_queue* head;
struct cir_queue* curr;
struct cir_queue* tail;

int global_pt[4];

int init_global_pt();
int create_PageTable(int pid);
int create_PageDirectory(int pid);
fr_map_t frm_tab[NFRAMES];

SYSCALL init_frm()
{	STATWORD ps;
	 disable(ps);
	 int i = 0;
	 for(i = 0; i < NFRAMES; i++)
  	{
		frm_tab[i].fr_dirty = 0;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_status = UNMAPPED;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_vpno = 0;
		frm_tab[i].count = 0;
  	}

  restore(ps);
  return OK;
}

SYSCALL get_frm(int* avail)
{
	STATWORD ps;
	disable(ps);
	int i = 0;
	*avail = -1;
	int frm_no;
	for(i = 0; i < NFRAMES; i++)
  	{
		if(frm_tab[i].fr_status == UNMAPPED)
		{
			*avail = i;
			restore(ps);
			return i;
  		}
  	}	
	
	if(page_replace_policy==SC)
	{
		struct cir_queue* temp;
		int frame_num;
		unsigned long a;
		pd_t* pd;
		pt_t* pt;
		unsigned long pd_offset;
		virt_addr_t* virt;
		if(curr==NULL){
			return SYSERR;
		}
	    	while(TRUE)
		{
			frame_num=curr->fr_no;
			a=frm_tab[frame_num].fr_vpno;
			virt = (virt_addr_t*)&a;
			pd_offset = virt->pd_offset;
			pd=(pd_t*)proctab[currpid].pdbr;
			pd = pd + ( a>>10);
			if(pd->pd_pres==0){
				return SYSERR;
			}
			pt=(pt_t*)(pd->pd_base<<12);
			pt = pt +(a%1024)*sizeof(pt_t);
			if(pt->pt_pres==0){
				return SYSERR;
			}
			if((pt->pt_acc)!=0)
			{
				(pt->pt_acc)=0;
			}
			else if((pt->pt_acc)==0)
			{	
				*avail=curr->fr_no;
				temp->next=curr->next;
				free_frm(curr->fr_no);
				freemem(curr, sizeof(struct cir_queue));
				curr=temp->next;
				return OK;
			}
			temp=curr;
			curr=curr->next;
		}
	}
	restore(ps);
	return SYSERR;
}
SYSCALL free_frm(int i) {
	STATWORD ps;
	int temp;
	fr_map_t *frptr = &frm_tab[i];
	disable(ps);
	if (i<0 || i>MAX_BS) {
		restore(ps);
		return SYSERR;
	}
	if(frptr->fr_type == -1){
		restore(ps);
		return SYSERR;
	}

	if(frptr->fr_type == FR_PAGE) {
		int store, pageth;
		if ((bsm_lookup(frptr->fr_pid, frptr->fr_vpno * NBPG, &store, &pageth) == SYSERR) || (store == -1) || (pageth == -1)) {
			write_bs((i + FRAME0) * NBPG, store, pageth);
			return SYSERR;
		}
		pd_t *pd_entry = proctab[frm_tab[i].fr_pid].pdbr + (frptr->fr_vpno / NBPG/4) * 4;
		pt_t *pt = (pd_entry->pd_base) * NBPG + (frptr->fr_vpno % NBPG/4) * 4;
		if (pt->pt_pres == 0) {
			restore(ps);
			return SYSERR;
		}
		pt->pt_dirty = CLEAN;
		pt->pt_mbz = 0;
		pt->pt_global = 0;
		pt->pt_avail = 0;
		pt->pt_base = 0;
		pt->pt_pres = 0;
		pt->pt_write = 1;
		pt->pt_user = 0;
		pt->pt_pwt = 0;
		pt->pt_pcd = 0;
		pt->pt_acc = 0;
	
		frptr->fr_dirty = CLEAN;
	        frptr->fr_pid = -1;
	        frptr->fr_refcnt = 0;
	        frptr->fr_status = UNMAPPED;
	        frptr->fr_type = -1;
	        frptr->fr_vpno = -1;
			frptr->count= 0;
	        
        	frm_tab[pd_entry->pd_base - FRAME0].fr_refcnt--;
                if (frm_tab[pd_entry->pd_base - FRAME0].fr_refcnt <= 0) {
                        free_frm(pd_entry->pd_base - FRAME0);
        	}
		restore(ps);
		return OK;
	}
	else if(frptr->fr_type == FR_TBL) {
		int j;
		for (j = 4; j < 1024; j++) {
			pt_t *pt = (i + FRAME0) * NBPG + j *4;
			if (pt->pt_pres == 1) {
				free_frm(pt->pt_base - FRAME0);
			}
		}
		for (j = 4; j < 1024; j++) {
			pd_t *pd_entry = proctab[frm_tab[i].fr_pid].pdbr + j * 4;
			if (pd_entry->pd_base - FRAME0 == i) {
				pd_entry->pd_pres = 0;
				pd_entry->pd_write = 1;
				pd_entry->pd_user = 0;
				pd_entry->pd_pwt = 0;
				pd_entry->pd_pcd = 0;
				pd_entry->pd_acc = 0;
				pd_entry->pd_mbz = 0;
				pd_entry->pd_fmb = 0;
				pd_entry->pd_global = 0;
				pd_entry->pd_avail = 0;
				pd_entry->pd_base = 0;
			}
		}
				frptr->fr_dirty = CLEAN;
                frptr->fr_pid = -1;
                frptr->fr_refcnt = 0;
                frptr->fr_status = UNMAPPED;
                frptr->fr_type = -1;
                frptr->fr_vpno = -1;
				frptr->count= 0;
		restore(ps);
		return OK;
	}
	
}
int create_PageTable(int pid) {

	int i;
	int frame = 0;
        get_frm(&frame);
	if (frame == -1) {
		return SYSERR;
	}

	frm_tab[frame].fr_refcnt = 0;
	frm_tab[frame].fr_type = FR_TBL;
	frm_tab[frame].fr_dirty = CLEAN;
	frm_tab[frame].fr_status = MAPPED;
	frm_tab[frame].fr_pid = pid;
	frm_tab[frame].fr_vpno = -1;
	frm_tab[frame].count = 0;


	for (i = 0; i < 1024; i++) {
		pt_t *pt = (FRAME0 + frame) * NBPG + i * sizeof(pt_t);
		pt->pt_dirty = CLEAN;
		pt->pt_mbz = 0;
		pt->pt_global = 0;
		pt->pt_avail = 0;
		pt->pt_base = 0;
		pt->pt_pres = 0;
		pt->pt_write = 1;
		pt->pt_user = 0;
		pt->pt_pwt = 0;
		pt->pt_pcd = 0;
		pt->pt_acc = 0;
	}
	return frame;
}

int create_PageDirectory(int pid) {
	int i;
	int frame = 0;
	get_frm(&frame);

	if (frame == -1) {
		return -1;
	}
	frm_tab[frame].fr_dirty = CLEAN;
	frm_tab[frame].fr_pid = pid;
	frm_tab[frame].fr_refcnt = 4;
	frm_tab[frame].fr_status = MAPPED;
	frm_tab[frame].fr_type = FR_DIR;
	frm_tab[frame].fr_vpno = -1;
	frm_tab[frame].count = 0;

	proctab[pid].pdbr = (FRAME0 + frame) * NBPG;
	for (i = 0; i < 1024; i++) {
		pd_t *pd_entry = proctab[pid].pdbr + (i * sizeof(pd_t));
		pd_entry->pd_pcd = 0;
		pd_entry->pd_acc = 0;
		pd_entry->pd_mbz = 0;
		pd_entry->pd_fmb = 0;
		pd_entry->pd_global = 0;
		pd_entry->pd_avail = 0;
		pd_entry->pd_base = 0;
		pd_entry->pd_pres = 0;
		pd_entry->pd_write = 1;
		pd_entry->pd_user = 0;
		pd_entry->pd_pwt = 0;

		if (i < 4) {
			pd_entry->pd_pres = 1;
			pd_entry->pd_write = 1;
			pd_entry->pd_base = global_pt[i];
		}
	}
	return frame;
}
