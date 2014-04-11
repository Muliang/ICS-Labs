/*
 * mm.c
 * 
 * Name: Yichao Xue
 * Andrew ID: yichaox
 * 
 * v2: seglist + first fit
 * poor heapchecker
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/*difine constant*/
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 9) 

/* define constant */
#define MIN_FREE_SIZE	16
#define MIN_ALLOC_SIZE	12
#define LIST_NUM 10

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/*define Macros*/
#define MAX(x, y) ((x) > (y) ? (x) : (y))
//put size, prev allocated bit and allocated bit together
#define PACK(size, prev_alloc, alloc)  ((size) | (prev_alloc) | (alloc)) 
//put and get value at ptr p 
#define GET(p)  (*(unsigned int *)(p)) 
#define PUT(p, val)  (*(unsigned int *)(p) = (val)) 

/*read size and allocated bit from ptr p*/
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)  (GET(p) & 0x1)

/* get head and footer ptr for a given block ptr*/
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* for given block ptr bp, compute ptr of previous and next block
 * ptr point to start addr of effective payload
 */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* get prev block allocated bit */
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)
/* set prev block to allocated*/ 
#define SET_PREV_ALLOC(p) (PUT(p, (GET(p) | 0x2)))
/* set prev block to free */ 
#define SET_PREV_FREE(p)  (PUT(p, (GET(p) & (~0x2))))

/* the offset of prev and next free block */
#define PREV_FREE_OFS(bp)	((char *)(bp) + WSIZE)
#define NEXT_FREE_OFS(bp)	(bp)
/* prev and next free block ptr*/
#define NEXT_FREE_BLKP(bp)	(base_ptr + GET(NEXT_FREE_OFS(bp)))
#define PREV_FREE_BLKP(bp)	(base_ptr + GET(PREV_FREE_OFS(bp)))

/*global variables*/
static char *heap_list = 0;
static char *base_ptr = 0;

/*declaration of helper functions*/
static void *extend_heap(size_t words);		//extend heap by n words
static void *coalesce(void *bp);			//coalesce free blocks
static void *find_fit(size_t asize);		//first fit: find a free block
static void place(void * bp, size_t asize);

/* functions operate the free list */
static void insert(void *bp, size_t size);
static void delete(void *bp);
static size_t get_list_index(size_t size); 

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	//initial size of the heap
	size_t initial_szie = LIST_NUM * DSIZE + 4 * WSIZE;
	size_t prologue_size = LIST_NUM * DSIZE + 2 * WSIZE;

	if((heap_list = mem_sbrk(initial_szie)) == (void *)-1)
	{  
 		return -1;	//sbrk fail 
	}
	base_ptr = heap_list;

	PUT(heap_list, 0);		//put first word of heap of 0
	//set the prologue's header
	PUT(heap_list + (WSIZE), PACK(prologue_size, 2, 1));
	//move ptr
	heap_list += (2 * WSIZE); 		
	//initialize the start block of the free list
	for(int i = 0; i < LIST_NUM; i++){
		PUT(heap_list + (i * DSIZE), ((i + 1) * DSIZE));
		PUT(heap_list + (i * DSIZE + WSIZE), ((i + 1) * DSIZE));
	}
	//set the prologue's footer
	PUT(FTRP(heap_list), PACK(prologue_size, 2, 1));
	//set the epilogue
	PUT(FTRP(heap_list) + WSIZE, PACK(0, 2, 1));

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
    	return -1;
    }
    return 0;
}

/*
 * malloc: allocate size size free momery
 */
void *malloc (size_t size) {
	size_t asize;
	size_t extendsize;
	char *bp;

	if(size <= 0){
		return NULL;
	}

	// if request size < MIN_ALLOC_SIZE
	if(size <= MIN_ALLOC_SIZE){
		asize = MIN_FREE_SIZE;
	}else{
		asize = ALIGN(size) + DSIZE;	// align the size to 8
	}

	if((bp = find_fit(asize)) != NULL){
		place(bp, asize);
		return bp;
	}

	//find_fit return null, no free block found, request more memory
	extendsize = MAX(asize, CHUNKSIZE);
	if((bp = extend_heap(extendsize / WSIZE)) == NULL ){	
		return NULL;
	}
	place(bp, asize);

    return bp;
}

/*
 * free: free an allocated block 
 */
void free (void *ptr) {

	if(ptr == 0)
		return;
	//get current block size
	size_t size = GET_SIZE(HDRP(ptr));
	//get prev allocated bit
	size_t is_prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
	//set header and footer of the freed block
	PUT(HDRP(ptr), PACK(size, is_prev_alloc, 0));
	PUT(FTRP(ptr), PACK(size, 0, 0));

    coalesce(ptr);
}

/*
 * realloc: change the size of an allocated block
 * using the naive method in the text book
 */
void *realloc(void *oldptr, size_t size) {
	size_t	oldsize; 
	void *newptr;

	if(oldptr == NULL){
		return malloc(size);
	}

	if(size == 0){
		free(oldptr);
		return 0;
	}

	newptr = malloc(size);
  	/* If realloc() fails the original block is left untouched  */
	if(!newptr) {
		return 0;
	}

	oldsize = GET_SIZE(HDRP(oldptr));
	//new allocated size < oldsize
	if(size < oldsize)
		oldsize = size;

	memcpy(newptr, oldptr, oldsize); //copy old payload

	free(oldptr);	//free old allocated block

	return newptr;
}

/*
 * calloc: set a new allocated area to 0
 * using the naive method in the text book
 */
void *calloc (size_t nmemb, size_t size) {

	if (nmemb <= 0 || size <= 0)
	{
		return NULL;
	}
	
	size_t bytes = nmemb * size;
  	void *newptr;

	newptr = malloc(bytes);

	//set the memory to zero
 	memset(newptr, 0, bytes);

  	return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}


//extend heap by words
static inline void *extend_heap(size_t words){
	char *bp;
	size_t size, is_prev_alloc;

	//align the request size
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	/*extend heap*/
	if((long)(bp = mem_sbrk(size)) == -1){
		printf("Error: mem_sbrk failed\n");
		return NULL;
	}
	//get prev allocated bit 
	is_prev_alloc = GET_PREV_ALLOC(HDRP(bp));
	//set header and footer 
	PUT(HDRP(bp), PACK(size, is_prev_alloc, 0));
	PUT(FTRP(bp), PACK(size, 0, 0));

	//restore epilogue
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 2, 1));

	return coalesce(bp);
}

//coalesce free blocks
static inline void *coalesce(void *bp){
	size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

/* case 1: next and prev block are both allocated
 * No need to coalesce
 */		
	if(prev_alloc && next_alloc){
		//add a free block to free list
		insert(bp, size);
		//set prev allocated bit of next block of current block to free
		SET_PREV_FREE(HDRP(NEXT_BLKP(bp)));
		return bp;
	}

	//case 2: prev block is allocated, coalesce next blk
	else if(prev_alloc && !next_alloc){
		//first, delete the next block from the free list
		delete(NEXT_BLKP(bp));
		//get new size
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

		PUT(HDRP(bp), PACK(size, 2, 0));
		PUT(FTRP(bp), PACK(size, 0, 0));
		//then, insert the new free block back to free list
		insert(bp, size);
		return bp;
	}

	//case 3: next block is allocated, coalesce prev blk
	else if(!prev_alloc && next_alloc){
		//remove prev free block
		delete(PREV_BLKP(bp));
		//set prev allocated bit of next block of current block to free
		SET_PREV_FREE(HDRP(NEXT_BLKP(bp)));
		//new size
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));

		//move bp to prev blk
		bp = PREV_BLKP(bp);

		size_t is_prev_alloc = GET_PREV_ALLOC(HDRP(bp));

		PUT(HDRP(bp), PACK(size, is_prev_alloc, 0));
		PUT(FTRP(bp), PACK(size, 0, 0));
		//insert new free blk to list
		insert(bp, size);	
		return bp;
	}

	//case 4: next and prev blocks are both free
	else {
		//get new size
		size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));

		size_t is_prev_alloc = GET_PREV_ALLOC(HDRP(PREV_BLKP(bp)));
		//delete prev and next blocks from free list
		delete(NEXT_BLKP(bp));
		delete(PREV_BLKP(bp));
		//move ptr to prev blk
		bp = PREV_BLKP(bp);

		PUT(HDRP(bp), PACK(size, is_prev_alloc, 0));
		PUT(FTRP(bp), PACK(size, 0, 0));
		//insert to free list
		insert(bp, size);
		return bp;
	}
}

/* find_fit: find a free block in the list
 * Uisng first fit right now
 */

//to be modified
static inline void *find_fit(size_t asize){
	void *bp;
	void *temp;
	size_t index = get_list_index(asize);

	char *cur_list = (char *)heap_list + index * DSIZE;

	for(temp = cur_list; temp != ((char *)heap_list + LIST_NUM * DSIZE) ; temp = (char *)temp + DSIZE){
		for(bp = NEXT_FREE_BLKP(temp); bp != temp; bp = NEXT_FREE_BLKP(bp)){
			if((GET_SIZE(HDRP(bp)) >= asize) && !GET_ALLOC(HDRP(bp))){
				return bp;
			}
		}
	}
	return NULL;		//cant find a valid block
}

/* place: place the allocated block to free block */
static inline void place(void * bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));		//current block size
	size_t is_prev_alloc = GET_PREV_ALLOC(HDRP(bp));

	//if current size - request size > MIN_FREE_LIST
	//then split the free blk
	if((csize - asize) >= MIN_FREE_SIZE){
		delete(bp);
		//set the header of new allocated blk
		PUT(HDRP(bp), PACK(asize, is_prev_alloc, 1));

		/* ptr to new free block after place*/
		bp = NEXT_BLKP(bp);

		PUT(HDRP(bp), PACK(csize - asize, 2, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0, 0));

		insert(bp, (csize - asize));
	} 
	//do not split the free block
	else{
		delete(bp);
		PUT(HDRP(bp), PACK(csize, 2, 1));
		SET_PREV_ALLOC(HDRP(NEXT_BLKP(bp)));
	}
}

//insert a free block, always insert it to the start of the list
static inline void insert(void *bp, size_t size){
	size_t index = get_list_index(size);
	//set the current blk's next free blk's offset
	PUT(NEXT_FREE_OFS(bp), GET((char *)heap_list + (index * DSIZE)));
	//set the current blk's prev free blk's offset
	PUT(PREV_FREE_OFS(bp), GET(PREV_FREE_OFS(NEXT_FREE_BLKP(bp))));
	//set the prev blk's next free blk's offset
	PUT(NEXT_FREE_OFS((char *)heap_list + index * DSIZE),
	 (long)bp - (long)base_ptr);
	//set the next blk's prev free blk's offset
	PUT(PREV_FREE_OFS(NEXT_FREE_BLKP(bp)), (long)bp - (long)base_ptr);
}

//remove the free blk from the list
static inline void delete(void *bp){
	//put prev free blk offset to next free blk
	PUT(NEXT_FREE_OFS(PREV_FREE_BLKP(bp)), GET(NEXT_FREE_OFS(bp)));
	//put next free blk offset to prev free blk
	PUT(PREV_FREE_OFS(NEXT_FREE_BLKP(bp)), GET(PREV_FREE_OFS(bp)));
}

/* get the list index upon the size of the free blk */
static inline size_t get_list_index(size_t size){
	if(size <= 16)
		return 0;
	else if(size <= 32)
		return 1;
	else if(size <= 64)
		return 2;
	else if(size <= 128)
		return 3;
	else if(size <= 256)
		return 4;
	else if(size <= 512)
		return 5;
	else if(size <= 1024)
		return 6;
	else if(size <= 2048)
		return 7;
	else if(size <= 4096)
		return 8;
	else
		return 9;
}

/*******************************
 	   	check heap
 ******************************/

/*
 * mm_checkheap
 */
void mm_checkheap(int verbose) {

	/*check the szie of the heap*/
	if(mem_heapsize() > 4294967296){
		printf("Heap size: [%d] bytes, violate heap size limitation\n", (int)mem_heapsize());
	}
	/*check first word of the heap, it should be 0 padding*/
	if(GET(mem_heap_lo()) != 0){
		printf("%s\n","Error: first word should be 0 padding");
	}

	char *bp = heap_list;
	/*check prologue*/
	if((GET_SIZE(HDRP(bp)) != DSIZE) || (GET_ALLOC(HDRP(bp)) != 1)){
		printf("%s\n", "Error: invalid prologue");
	}

	for(bp = heap_list; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){

		if(verbose){
			printf("pointer is at [%p] \n", bp);
		}
		//check alignment
		if(!aligned(bp)){
			printf("block @ [%p] is not aligned\n",bp );
		}
		if(!in_heap(bp)){
			printf("block @ [%p] is out of boundry\n",bp );
		}
	}

}

