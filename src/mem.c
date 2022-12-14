
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;
<<<<<<< HEAD
=======
static pthread_mutex_t ram_lock;
>>>>>>> 56651ad (commit all)

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct trans_table_t * get_trans_table(
		addr_t index, 	// Segment level index
		struct page_table_t * page_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [page_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	int i;
	for (i = 0; i < page_table->size; i++) {
		// Enter your code here
		if(page_table->table[i].v_index == index){
			return page_table->table[i].next_lv;
		}
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct trans_table_t * trans_table = NULL;
	trans_table = get_trans_table(first_lv, proc->page_table);
	if (trans_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < trans_table->size; i++) {
		if (trans_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of trans_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			
			*physical_addr = (trans_table->table[i].p_index << OFFSET_LEN) | offset;

			// printf("1st: %05x | %05x = %05x\n", trans_table->table[i].p_index << OFFSET_LEN, offset, *physical_addr);

			return 1;


		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) 
{
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = ((size % PAGE_SIZE) == 0) ? size / PAGE_SIZE :
		size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
	 
	int pages_avail = 0;
	for(int i = 0; i < NUM_PAGES; i++) //Check if ram memory space is avaiable
	{
		if(_mem_stat[i].proc == 0)
			pages_avail++;
		if(pages_avail == num_pages && proc->bp + pages_avail * PAGE_SIZE <= RAM_SIZE)
		{
			mem_avail = 1;
			break;
		}
	}

	if (mem_avail) 
	{
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  isValid. */

		
		int frame_index = 0;
		int prev = -1; //prev index
		for(int i = 0; i < NUM_PAGES; i++)
		{
			//update status pages in physical address
			if(_mem_stat[i].proc == 0)
			{
			 	// add/update [proc], [index], and [next] field
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = frame_index;
				if (prev != -1) { // not initial page, update last page
<<<<<<< HEAD
					_mem_stat[prev].next = i;
=======
					_mem_stat[prev].next = i; // page has been used
>>>>>>> 56651ad (commit all)
				}
				prev = i;

				/*Add entries to segment table page tables of [proc]
		 	  	to ensure accesses to allocated memory slot is
		  	  	valid.
				*/
				struct page_table_t * page_table = proc->page_table;
<<<<<<< HEAD
				if(page_table->table[0].next_lv == NULL) 
				{
					// if page table is null, set page table size = 0
					page_table->size = 0; 
				}

 				
				addr_t virtual_address = ret_mem + (frame_index << OFFSET_LEN);
				addr_t segment_idx = get_first_lv(virtual_address);   //get the first layer index
				addr_t page_idx = get_second_lv(virtual_address); //get the second layer index 
				struct trans_table_t * trans_table = get_trans_table(segment_idx, page_table);
				
				
=======
				// printf("page_table->size: %d\n", page_table->size);
 				
				addr_t virtual_address = ret_mem + (frame_index << OFFSET_LEN);
				addr_t lv1 = get_first_lv(virtual_address);   //get the first layer index -page
				addr_t lv2 = get_second_lv(virtual_address); //get the second layer index - trans
				struct trans_table_t * trans_table = get_trans_table(lv1, page_table);
>>>>>>> 56651ad (commit all)
				
				// if there is not trans_table in seg -> create new trans_table
				if (trans_table == NULL) 
				{
					int idx = page_table->size;
<<<<<<< HEAD
					page_table->table[idx].v_index = segment_idx;
					trans_table = (struct trans_table_t*) malloc(sizeof(struct trans_table_t));
					page_table->table[idx].next_lv = (struct trans_table_t*) malloc(sizeof(struct trans_table_t));
					trans_table = page_table->table[idx].next_lv;
=======
					page_table->table[idx].v_index = lv1;
					// printf("page_table->table[idx].v_index: %05x\n", page_table->table[idx].v_index);
					trans_table = (struct trans_table_t*) malloc(sizeof(struct trans_table_t));
					page_table->table[idx].next_lv = trans_table;
>>>>>>> 56651ad (commit all)
					page_table->size++;
				}

				// update mem_stat
				int idx = trans_table->size++;
<<<<<<< HEAD
				trans_table->table[idx].v_index = page_idx;
=======
				trans_table->table[idx].v_index = lv2; //mark the trans table as valid
>>>>>>> 56651ad (commit all)
				trans_table->table[idx].p_index = i;
				if(frame_index == (num_pages-1))
				{
					// the last element's next field = -1
					_mem_stat[i].next = -1;
					break;
				}
<<<<<<< HEAD
				frame_index++; //update page index
			}
		}
	}
=======
				frame_index++; //update frame index
			}
		}
	}
	// puts("--------------------------------");
	// dump();
>>>>>>> 56651ad (commit all)
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}


int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);
	addr_t virtual_address = address;	// virtual address to free in process
	addr_t physical_addr = 0;		// physical address to free in memory
	// printf("translate(virtual_address, &physical_addr, proc):%d \n", translate(virtual_address, &physical_addr, proc));
	if (translate(virtual_address, &physical_addr, proc) == 0)
	{
		pthread_mutex_unlock(&mem_lock);	
		return 1;	
	}


	// clear physical page in mem
	int num_pages = 0;
	int i = 0;

	// get physical index used by memory block
	for (i = physical_addr >> OFFSET_LEN; i != -1; i = _mem_stat[i].next)
	{
		num_pages++;

		//Set flag [proc] of physical page use by the memory block
	  	//back to zero to indicate that it is free.
		_mem_stat[i].proc = 0; 
	}
	for (i = 0; i < num_pages; i++)
	{
<<<<<<< HEAD
		addr_t addr = virtual_address + i * PAGE_SIZE;
=======
		addr_t addr = address + i * PAGE_SIZE; //virtual addr
>>>>>>> 56651ad (commit all)
		addr_t first_lv = get_first_lv(addr);
		addr_t second_lv = get_second_lv(addr);
		struct trans_table_t * trans_table = get_trans_table(first_lv, proc->page_table);
		if (trans_table == NULL)
			continue;
<<<<<<< HEAD
		//remove 
=======
		//remove trans
>>>>>>> 56651ad (commit all)
		for (int j = 0; j < trans_table->size; j++)
		{
			if (trans_table->table[j].v_index == second_lv)
			{
				trans_table->size--;
				trans_table->table[j] = trans_table->table[trans_table->size];
				break;
			}
<<<<<<< HEAD
		}
		//remove table
		if (trans_table->size == 0 && proc->page_table != NULL)
=======
			
		}
		//remove page
		if (trans_table->size == 0)
>>>>>>> 56651ad (commit all)
		{
			for (int k = 0; k < proc->page_table->size; k++)
			{
				if (proc->page_table->table[k].v_index == first_lv)
				{
					proc->page_table->size--;
					proc->page_table->table[k] = proc->page_table->table[proc->page_table->size];
					proc->page_table->table[proc->page_table->size].v_index = 0;
					free(proc->page_table->table[proc->page_table->size].next_lv);
				}
			}
		}
	}
<<<<<<< HEAD

	// update breakpoint
	// if (virtual_address + num_pages * PAGE_SIZE == proc->bp) 
	// {
	// 	while (proc->bp >= PAGE_SIZE) {
	// 		addr_t last_addr = proc->bp - PAGE_SIZE;
	// 		addr_t last_segment = get_first_lv(last_addr);
	// 		addr_t last_page = get_second_lv(last_addr);
	// 		struct trans_table_t * trans_table = get_trans_table(last_segment, proc->page_table);
	// 		if (trans_table == NULL) break;
	// 		while (last_page >= 0) {
	// 			int i;
	// 			for (i = 0; i < trans_table->size; i++) {
	// 				if (trans_table->table[i].v_index == last_page) {
	// 					proc->bp -= PAGE_SIZE;
	// 					last_page--;
	// 					break;
	// 				}
	// 			}
	// 			if (i == trans_table->size) break;
	// 		}
	// 		if (last_page >= 0) break;
	// 	}
	// }
=======
	// puts("-----------------?----------------------");
	// dump();
>>>>>>> 56651ad (commit all)
	pthread_mutex_unlock(&mem_lock);
	return 0;
}



int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
<<<<<<< HEAD
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
=======
	pthread_mutex_lock(&ram_lock);
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		pthread_mutex_unlock(&ram_lock);
>>>>>>> 56651ad (commit all)
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
<<<<<<< HEAD
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
=======
	pthread_mutex_lock(&ram_lock);
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		pthread_mutex_unlock(&ram_lock);

>>>>>>> 56651ad (commit all)
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


