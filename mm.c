/*
 * mm.c -- implement your malloc functions here. See mm-naive.c for an example implementation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

typedef struct {
	size_t size_n_status;   //size of payload, excluding headers
	size_t padding;  //8 byte padding
} header;


typedef struct free_hdr {
	header common_header;
	struct free_hdr *next;
	struct free_hdr *prev;
} free_hdr;


free_hdr * coalesce (free_hdr * me, free_hdr * other);
bool inspect_chunk_prev (header * hdr);
bool inspect_chunk_next (header * hdr);
header * get_next_header(header * p);
header * get_prev_header(header * p);
header * mem_extend_occupied (header * me, free_hdr * other);

//Doubly linked list head pointer for free chunks
free_hdr * freelist;

MIN_OS_ALLOC_SZ = sizeof(header) + 17*sizeof(free_hdr);


//============================================================
//Helper Functions for Header
/*
Helper functions removed for public hosting since is a common
CS lab in many schools to prevent plagiarism. 

Contact me to request the full source code.
*/

//============================================================
//Explicit List Implimentation



//Initiate free chunk of size sz, status false, prev and next NULL
void init_free_chunk(free_hdr * hdr, size_t sz){

	header * free_common_hdr = &(hdr->common_header);

	set_chunk_size_status(free_common_hdr, sz, false);
	hdr->prev = NULL;
	hdr->next = NULL;
	header * free_common_ftr = get_footer_from_header(free_common_hdr);

	set_chunk_size_status(free_common_ftr, sz, false);

}

//Get mem from OS and initiate new free chunk of size sz
free_hdr * get_block_from_OS(size_t sz) {
	free_hdr * hdr = mem_sbrk(sz);

	//out of memory
	if ((long) hdr == -1){
		return NULL;
	}

	init_free_chunk(hdr, sz); //initiate header and footer for this new free chunk
	return hdr;
}




//Insert at front of linked list
void insert (free_hdr ** head, free_hdr *node) {
	if (*head)
		(*head)->prev = node;
	node->next = *head;
	node->prev = NULL;
	*head = node;
}


void delete (free_hdr ** head, free_hdr *node) {

	assert(node); //make sure node is not null

	free_hdr * prevn = node->prev;
	free_hdr * nextn = node->next;


	if (prevn) {  //not deleting the head of the list

		prevn->next = nextn;

		if (nextn){
			
			nextn->prev = prevn;
		}

	}
	else {  //delete frist node in list
		*head = nextn;
		if (nextn){
			nextn->prev = NULL;
		}
	}
	
}


//Find first fit in linked lits
free_hdr * first_fit(size_t sz) {
	free_hdr *n = freelist;
	size_t cur_chunk_sz;
	free_hdr * wanted = NULL;
	while (n!=NULL) {

		cur_chunk_sz = get_chunk_size(&(n->common_header));

		if (cur_chunk_sz >= sz){
			delete(&freelist, n);
		
			wanted = n;
			break;
		}

		n = n->next;
	}

	return wanted;

}




//Split free block, return one to be used, one to be added to free list
free_hdr* split(free_hdr* n, size_t csz){
	//csz is the size of the utilised chunk counting counting headers.

	size_t total_size = get_chunk_size(&n->common_header);

	if (total_size - csz < MIN_OS_ALLOC_SZ) {

		return NULL;
	}

	//reset occupied chunk header sizes
	set_chunk_size_status(n, csz, true);
	set_chunk_size_status(get_footer_from_header(n), csz, true);


	//set free chunk headers
	free_hdr * new_free = (free_hdr*)((char *)n + csz);
	init_free_chunk(new_free, total_size - csz);

	insert(&freelist, new_free);

	return n;

}




/* 
 * mm_init initializes the malloc package.
 */
int mm_init(void)
{
	freelist = get_block_from_OS(MIN_OS_ALLOC_SZ);
	return 0;
}

/* 
 * mm_malloc allocates a memory block of size bytes
 */
void *mm_malloc(size_t s)
{
	size_t csz = align16(s) + 2*sizeof(header); // min chunk size

	free_hdr * n = first_fit(csz);
	if (!n) {
		n = get_block_from_OS(csz>MIN_OS_ALLOC_SZ?csz:MIN_OS_ALLOC_SZ);

		//OS out of memory
		assert(n!=NULL);

	}
	
	//set occupied status of occupied chunk in spit function
	split(n, csz);
	
	set_chunk_status(n, true);
	set_chunk_status(get_footer_from_header(n), true);

	return (void*)((char*)n + sizeof(header));

}




/*
 * mm_free frees the previously allocated memory block
 */
void mm_free(void *ptr)
{

	header * hdr = payload_to_header(ptr);

	init_free_chunk((free_hdr*)hdr, get_chunk_size(hdr));
	
	header *next = get_next_header(hdr);
	header *prev = get_prev_header(hdr);

	bool coalesce_next = inspect_chunk_next(hdr);
	bool coalesce_prev = inspect_chunk_prev(hdr);
		
	if (coalesce_next) {
		
		hdr = coalesce ((free_hdr *)hdr, (free_hdr *)next);

	}


	if (coalesce_prev) {

		hdr = coalesce ((free_hdr *)hdr, (free_hdr *)prev);
	}


	


	insert(&freelist, (free_hdr *)hdr);


}	




//#################### COALESCE HELPERS ####################


header * get_next_header(header * p){
	return (header *)((char *)p + get_chunk_size(p));
}


header * get_prev_header(header * p){

	header * prev_footer = (header *)((char *)p - sizeof(header));

	header * prev_header = get_header_from_footer(prev_footer);


	return prev_header;
}

bool inspect_chunk_prev (header * hdr){

	if (hdr <= mem_heap_lo()) return false;

	header * footer_prev = (header *)((char*)hdr - sizeof(header));
	header * header_prev = get_prev_header(hdr);

	//makes sure it is within heap region
	if (header_prev >= mem_heap_lo() && footer_prev <= mem_heap_hi() - sizeof(header)){
		if (get_chunk_status(footer_prev) == 0)
			return true;  // check if prev is free

	}


	return false;
}

bool inspect_chunk_next (header * hdr){
	
	header * footer_cur = get_footer_from_header(hdr);
	if (footer_cur >= ((char*) mem_heap_hi() - sizeof(header)))
		return false; // current chunk is at heap highest point

	header * header_next = get_next_header(hdr);
	header * footer_next = get_footer_from_header(header_next);

	//makes sure it is within heap region
	if (header_next >= mem_heap_lo() && footer_next <= mem_heap_hi() - sizeof(header)){

		if (get_chunk_status(header_next) == 0)
			return true;  // check if prev is free
	}
	return false;
}

//#################### END OF COALESCE HELPERS ####################


free_hdr * coalesce (free_hdr * me, free_hdr * other) {

	header * me_header = &(me->common_header);
	header * other_header = &(other->common_header);

	size_t sum = get_chunk_size(me_header) + get_chunk_size(other_header);

	delete (&freelist, other);

	//pick the lower chunk as new head
	free_hdr *h;
	if (me>other){
		h = other;

	}
	else {
		h = me;

	}

	init_free_chunk(h, sum);

	return h;

	
}




/*
 * mm_realloc changes the size of the memory block pointed to by ptr to size bytes.  
 * The contents will be unchanged in the range from the start of the region up to the minimum of   
 * the  old  and  new sizes.  If the new size is larger than the old size, the added memory will   
 * not be initialized.  If ptr is NULL, then the call is equivalent  to  malloc(size).
 * if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
 */

void *mm_realloc(void *ptr, size_t size)
{
	
	//mm_checkheap(2);
	if (size == 0 && ptr != NULL){
		mm_free(ptr);
		return NULL;
	}

	if (ptr == NULL){
		return (void *)mm_malloc(size);
	}

	if (ptr != NULL) {

		header * hdr = (header *)((char*)ptr - sizeof(header));

		size_t cur_payload_size = get_payload_size(hdr);
		size_t size_aligned = align16(size);// + 2*sizeof(header);


		//cur size greater than size needed
		if (cur_payload_size > size_aligned){

			
			split((free_hdr *)hdr, size_aligned);
			return (void*)header_to_payload(hdr);
		}

		if (cur_payload_size == size_aligned){

			return ptr;
		}
		
		else { 


			void * new = mm_malloc(size_aligned);	//returns the start of payload
			memcpy(new, ptr, size);
			mm_free(ptr);

			return new;
			
		}
	}	

	return NULL;
}



/*
 * mm_checkheap checks the integrity of the heap and helps with debugging
 */

void mm_checkheap(int verbose_level) 
{

	size_t hc_total_allocated = 0,hc_total_free = 0;
	size_t hc_total_allocated_sz = 0, hc_total_free_sz = 0;
	header * h;

	h = mem_heap_lo();

	if (verbose_level > 0)
		printf("# Current heap low : %p ; heap hi: %p\n", mem_heap_lo(), mem_heap_hi());

	while((void*) h < mem_heap_hi()){

		if (verbose_level > 1)
			printf("# Chunk at %p, size %d, status %d\n", h, get_chunk_size(h), get_chunk_status(h));
		if (get_chunk_status(h)) {
			hc_total_allocated ++;
			hc_total_allocated_sz += get_chunk_size(h);

		}

		else {
			hc_total_free_sz += get_chunk_size(h);
			hc_total_free ++;
		}
		if (get_chunk_size(h)== 0) {
			printf("!! Zero sized chunk found!\n");
			break;
		}
		assert(get_chunk_size(h) != 0);
		h = get_next_chunk(h);
	}

	if (verbose_level > 0)
		printf("# Heap Check: %d allocated chunks, %d free chunks; allocated size = %d, free size = %d.\n", hc_total_allocated, hc_total_free, hc_total_allocated_sz, hc_total_free_sz);
	

	return;
}
