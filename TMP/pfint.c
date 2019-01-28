/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
extern int page_replace_policy;
struct cir_queue* head;
struct cir_queue* curr;
struct cir_queue* tail;
int global_pt[4];
SYSCALL pfint()
{
	STATWORD ps;
	unsigned long int v_addr;
	virt_addr_t *virt_addr; 
	unsigned int pg_offset;
	unsigned int pt_offset;
	unsigned int pd_offset;

	pd_t *pd_entry;
	pt_t *pt_entry; 
	int new_pt; 
	int new_frame; 
	int bs, x, y; 
	int pg; 
	disable(ps);

	v_addr = read_cr2();
	virt_addr = (virt_addr_t*)&v_addr;
	pg_offset = virt_addr->pg_offset;
	pt_offset = virt_addr->pt_offset;
	pd_offset = virt_addr->pd_offset;
	if(x == -1 || y == -1 || (bsm_lookup(currpid, v_addr, &x, &y) == SYSERR)){
                return kill(currpid);
        }
	pd_entry = proctab[currpid].pdbr + pd_offset*sizeof(pd_t);
	
	if(pd_entry->pd_pres ==0)
	{
		new_pt = create_PageTable(currpid);
		alloc_pdir(pd_entry, new_pt);
	}

	pt_entry = (pt_t*)(pd_entry->pd_base*NBPG+pt_offset*sizeof(pt_t));

	if(pt_entry->pt_pres != 1)
	{
		alloc_pt(pt_entry, pd_entry, new_frame, v_addr);
	}
	if(page_replace_policy == SC){
		if(head->fr_no==-1){
			head->fr_no=new_frame;
			head->next=head;
			head->pid=currpid;
			curr=head;
			tail=head;
		}
		else
		{
			struct cir_queue* new_node=(struct cir_queue*)getmem(sizeof(struct cir_queue));
			new_node->fr_no=new_frame;
			new_node->next=head;
			new_node->pid=currpid;
			tail->next=new_node;
			tail=new_node;
		}
	}
	write_cr3(proctab[currpid].pdbr);
	restore(ps);
  	return OK;
}

void alloc_pdir(pd_t* ptr, int offset){
	
        ptr->pd_pres = 1;
        ptr->pd_write = 1;
        ptr->pd_user = 0;
        ptr->pd_pwt = 0;
        ptr->pd_pcd = 0;
        ptr->pd_acc = 0;
        ptr->pd_mbz = 0;
        ptr->pd_fmb = 0;
        ptr->pd_global = 0;
        ptr->pd_avail = 0;
        ptr->pd_base = offset+FRAME0;

	frm_tab[offset].fr_status = MAPPED;
    	frm_tab[offset].fr_type = FR_TBL;
        frm_tab[offset].fr_pid = currpid;
}

void alloc_pt(pt_t* ptr, pd_t* pdtr, int frm, unsigned long int addr){
	int bs, pg;
	if(get_frm(&frm) == SYSERR){
		kill(currpid);
	}
        ptr->pt_pres = 1;
        ptr->pt_write = 1;
        ptr->pt_base = (FRAME0+frm);

        frm_tab[pdtr->pd_base-FRAME0].fr_refcnt++;

        frm_tab[frm].fr_status = MAPPED;
        frm_tab[frm].fr_type = FR_PAGE;
        frm_tab[frm].fr_pid = currpid;
        frm_tab[frm].fr_vpno = addr/NBPG;
        read_bs((char*)((FRAME0+frm)*NBPG),bs,pg);
}

