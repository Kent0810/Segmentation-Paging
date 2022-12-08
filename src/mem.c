
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
					_mem_stat[prev].next = i;
				}
				prev = i;

				/*Add entries to segment table page tables of [proc]
		 	  	to ensure accesses to allocated memory slot is
		  	  	valid.
				*/
				struct page_table_t * page_table = proc->page_table;
				if(page_table->table[0].next_lv == NULL) 
				{
					// if page table is null, set page table size = 0
					page_table->size = 0; 
				}

 				
				addr_t virtual_address = ret_mem + (frame_index << OFFSET_LEN) ;
				addr_t segment_idx = get_first_lv(virtual_address);   //get the first layer index
				addr_t page_idx = get_second_lv(virtual_address); //get the second layer index 
				struct trans_table_t * trans_table = get_trans_table(segment_idx, page_table);
				
				
				
				// if there is not trans_table in seg -> create new trans_table
				if (trans_table == NULL) 
				{
					int idx = page_table->size;
					page_table->table[idx].v_index = segment_idx;
					trans_table = (struct trans_table_t*) malloc(sizeof(struct trans_table_t));
					page_table->table[idx].next_lv = (struct trans_table_t*) malloc(sizeof(struct trans_table_t));
					trans_table = page_table->table[idx].next_lv;
					page_table->size++;
				}

				// update mem_stat
				int idx = trans_table->size++;
				trans_table->table[idx].v_index = page_idx;
				trans_table->table[idx].p_index = i;
				if(frame_index == (num_pages-1))
				{
					// the last element's next field = -1
					_mem_stat[i].next = -1;
					break;
				}
				frame_index++; //update page index
			}
		}
	}
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
		addr_t addr = virtual_address + i * PAGE_SIZE;
		addr_t seg = get_first_lv(addr);
		addr_t page = get_second_lv(addr);
		struct trans_table_t * trans_table = get_trans_table(seg, proc->page_table);
		if (trans_table == NULL)
			continue;
		//remove page
		for (int j = 0; j < trans_table->size; j++)
		{
			if (trans_table->table[j].v_index == page)
			{
				trans_table->size--;
				trans_table->table[j] = trans_table->table[trans_table->size];
				break;
			}
		}
		//remove table
		if (trans_table->size == 0 && proc->page_table != NULL)
		{
			for (int k = 0; k < proc->page_table->size; k++)
			{
				if (proc->page_table->table[k].v_index == seg)
				{
					proc->page_table->size--;
					proc->page_table->table[k] = proc->page_table->table[proc->page_table->size];
					proc->page_table->table[proc->page_table->size].v_index = 0;
					free(proc->page_table->table[proc->page_table->size].next_lv);
				}
			}
		}
	}

	//update breakpoint
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
	pthread_mutex_unlock(&mem_lock);
	return 0;
}



// addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
// 	pthread_mutex_lock(&mem_lock);
// 	addr_t ret_mem = 0;
// 	/* TODO: Allocate [size] byte in the memory for the
// 	 * process [proc] and save the address of the first
// 	 * byte in the allocated memory region to [ret_mem].
// 	 * */
// 	// Number of pages we will use
// 	uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE + 1:
// 		size / PAGE_SIZE;

// 	int mem_avail = 0; // We could allocate new memory region or not?

// 	/* TODO: First we must check if the amount of free memory in
// 	 * Virtual Memory Space (Virtual Memory Engine) and Physical Memory Space 
// 	 * is large enough to represent the amount of required 
// 	 * memory. If so, set 1 to [mem_avail].
// 	 * Hint: check [proc] bit in each page of _mem_stat
// 	 * to know whether this page has been used by a process.
// 	 * For virtual memory space, check bp (break pointer).
// 	 * */

// 	int available_page = 0;
// 	int i;
// 	for (i = 0; i<NUM_PAGES; i++){
// 		// this page has been used or not in PMS
// 		if (_mem_stat[i].proc == 0){
// 			available_page++;
// 		}
// 		/* checks if the address size -  size is greater than or equal to the break
// 		pointer */
// 		if (available_page >= num_pages) {
// 			if ((1 << ADDRESS_SIZE) - size >= proc->bp){
// 				printf("1: %05x\n", 1);
// 				printf("(1 << ADDRESS_SIZE): %05x\n", (1 << ADDRESS_SIZE));
// 				printf("(1 << ADDRESS_SIZE) - size: %05x\n", (1 << ADDRESS_SIZE) - size);
// 				printf("proc->bp: %05x\n", proc->bp);
// 				mem_avail = 1;
// 			}
// 			break;
// 		}
// 	}
	
// 	if (mem_avail) { // large enough
// 		/* We could allocate new memory region to the process */
// 		ret_mem = proc->bp;
// 		printf("ret_mem: %05x\n", ret_mem);
// 		proc->bp += num_pages * PAGE_SIZE;
// 		printf("num_pages: %05x\n", num_pages);
// 		printf("proc->bp: %05x\n", proc->bp);
// 		// printf("ret_mem: %d\n", ret_mem);
		
// 		/* TODO: Update status of physical pages which will be allocated
// 		 * to [proc] in _mem_stat. Tasks to do:
// 		 * 	- Update [proc], [index], and [next] field
// 		 * 	- Add entries to segment table page tables of [proc]
// 		 * 	  to ensure accesses to allocated memory slot is
// 		 * 	  valid. */

// 		// TODO: Update [proc], [index], and [next] field

// 		int number_of_page_left = num_pages;
// 		int prev_mem_stat = -1;
// 		int i;

// 		// Array of physical index of _mem_stat
// 		int list_p_index[num_pages];

// 		/* The above code is assigning the physical memory to the process. */
// 		for (i=0; i<NUM_PAGES; i++){
// 			if (_mem_stat[i].proc == 0){
// 				// TODO: Update the [next] of previous _mem_stat to the current _mem_stat
// 				if (prev_mem_stat != -1){
// 					_mem_stat[prev_mem_stat].next = i;
// 				}

// 				_mem_stat[i].proc = proc->pid;
// 				_mem_stat[i].index = num_pages - number_of_page_left;
// 				prev_mem_stat = i;
				
// 				// Update physical index
// 				list_p_index[num_pages - number_of_page_left] = i;
				
// 				number_of_page_left--;

// 				// TODO: Assign -1 to the [next] of tha last _mem_stat
// 				if (number_of_page_left == 0){
// 					_mem_stat[i].next = -1;
// 					break;
// 				}
// 			}
// 		}

// 		// TODO: Add entries to segment table page tables of [proc]
// 		// to ensure accesses to allocated memory slot is
// 		// valid.
		
// 		addr_t current_virtual_address = ret_mem;

// 		for (size_t i=0; i<num_pages; i++){
// 			addr_t first_lv_addr = get_first_lv(current_virtual_addr);
// 			addr_t second_lv_addr = get_second_lv(current_virtual_addr);

// 			int found = 0;
// 			size_t j;
// 			for (j=0; j<proc->page_table->size; j++){
// 				if (proc->page_table->table[j].v_index == first_lv_addr){
// 					found = 1;
// 					break;
// 				}
// 			}

// 			if (found == 0){
// 				proc->page_table->table[j].v_index = first_lv_addr;
// 				proc->page_table->table[j].next_lv = (struct page_table_t *) malloc (sizeof(struct page_table_t));
// 				proc->page_table->table[j].next_lv->size = 0;
// 				proc->page_table->size++;
// 			}

// 			proc->page_table->table[j].next_lv->table[proc->page_table->table[j].next_lv->size].v_index = second_lv_addr;
// 			proc->page_table->table[j].next_lv->table[proc->page_table->table[j].next_lv->size].p_index = list_p_index[i];
// 			// proc->page_table->table[j].->size++;
// 			proc->page_table->table[j].next_lv->size++;

// 			current_virtual_addr += PAGE_SIZE;
// 		}

// 	}
// 	pthread_mutex_unlock(&mem_lock);
// 	return ret_mem;
// }

// int free_mem(addr_t address, struct pcb_t * proc) {
// 	/*TODO: Release memory region allocated by [proc]. The first byte of
// 	 * this region is indicated by [address]. Task to do:
// 	 * 	- Set flag [proc] of physical page use by the memory block
// 	 * 	  back to zero to indicate that it is free.
// 	 * 	- Remove unused entries in segment table and page tables of
// 	 * 	  the process [proc].
// 	 * 	- Remember to use lock to protect the memory from other
// 	 * 	  processes.  */

// 	int found = 0;
// 	int number_of_pages = 0;

// 	for (size_t i=0; i<proc->page_table->size; i++){
// 		if (proc->page_table->table[i].v_index == get_first_lv(address)){
// 			found = 1;
// 		}
// 	}

// 	if (found == 1){
// 		addr_t physical_addr;
// 		translate(address, &physical_addr, proc);
// 		addr_t _p_index = physical_addr >> OFFSET_LEN;
// 		while(1){
// 			pthread_mutex_lock(&mem_lock);
// 			_mem_stat[_p_index].proc = 0;
// 			number_of_pages++;
// 			_p_index = _mem_stat[_p_index].next;
// 			pthread_mutex_unlock(&mem_lock);
// 			if (_p_index == -1){
// 				break;
// 			}
// 		}
// 		addr_t current_virtual_address = address;
// 		for (size_t j=0; j<number_of_pages; j++){
// 			addr_t first_lv_addr = get_first_lv(current_virtual_address);
// 			addr_t second_lv_addr = get_second_lv(current_virtual_address);

// 			size_t seg_table_ind;
// 			for (seg_table_ind=0; seg_table_ind<proc->page_table->size; seg_table_ind++){
// 				if (proc->page_table->table[seg_table_ind].v_index == first_lv_addr){
// 					break;
// 				}
// 			}

// 			size_t page_table_ind;
// 			for (page_table_ind=0; 
// 				page_table_ind<proc->page_table->table[seg_table_ind].next_lv->size; 
// 				page_table_ind++){
// 					if(proc->page_table->table[seg_table_ind].next_lv->table[page_table_ind].v_index
// 					== second_lv_addr){
// 						break;	
// 					}
// 				}

			
// 				for (size_t j=page_table_ind;
// 				j<proc->page_table->table[seg_table_ind].next_lv->size-1;
// 				j++){
// 					proc->page_table->table[seg_table_ind].next_lv->table[j] = 
// 					proc->page_table->table[seg_table_ind].next_lv->table[j+1];
// 				}

// 				proc->page_table->table[seg_table_ind].next_lv->size--;

// 				// TODO: If the page_table is empty, free it
// 				if (proc->page_table->table[seg_table_ind].next_lv->size == 0){
// 					free(proc->page_table->table[seg_table_ind].next_lv);
// 					for (size_t k=seg_table_ind; k<proc->page_table->size-1; k++){
// 						proc->page_table->table[k] = proc->page_table->table[k+1];
// 					}
// 					proc->page_table->size--;
// 				}
// 			current_virtual_address += PAGE_SIZE;
// 		}
// 	}

// 	return 0;
// }

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
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


