/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * Implemented using an explicit list 
 * Best fit algorithm used 
 * Realloc is implemented using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * replace the first three fields in the following struct
 * with your own information (you may choose your own
 * "team name").  Leave the last two feilds blank.
 ********************************************************/
team_t team = {
	/* Team name */
	"Jeremiah Crowley",
	/* First member's full name */
	"Jeremiah Crowley",
	/* First member's NYU NetID*/
	"jcc608",
	/* Second member's full name (leave blank if none) */
	"",
	/* Second member's email address (leave blank if none) */
	""
};
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* From Text Page 830 */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */  
#define DSIZE       8       /* Double Word Size (bytes) */
#define CHUNKSIZE  (1<<7)   /* Extend Heap Size By this amount (bytes) */
#define OVERHEAD    8       /* Overhead of header and footer (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(size_t *)(p))
#define PUT(p,val)      (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Give a free block ptr bp, compute the new and previous free block */
#define NEXT_FREEP(bp) (*(void **)(bp + WSIZE))
#define PREV_FREEP(bp) (*(void  **)bp)

/* Next block of prologue pointer */ 
static void *heap_listp;

/* Points to prologue block */
static void *free_listp;    

/* The largest free block on the list */
static int maxBlockSize1 = 0;      

/* The second largest free block on the list */
static int maxBlockSize2 = 0;

static void *extend_heap(size_t words);
static void place(void* bp, size_t asize, int call_delete);
static void *find_fit(size_t asize, int *list_broken);
static void *coalesce(void *bp);
static void *split_block (void *bp, size_t asize);
void insertList(void *bp);
void deleteList(void * bp);
int mm_check(void);
int lastBlock(void * bp);

/* 
 * mm_init - Initialize the memory manager 
 * From Textbook Page 831
*/
 int mm_init(void)
 {
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == NULL)
        return -1;
    PUT(heap_listp, 0);                          /* alignment padding */
    PUT(heap_listp+ (1*WSIZE), PACK(DSIZE, 1));  /* prologue header */ 
    PUT(heap_listp+ (2*WSIZE), PACK(DSIZE, 1));  /* prologue footer */ 
    PUT(heap_listp+ (3*WSIZE), PACK(0, 1));      /* epilogue header */
    heap_listp += DSIZE;
    free_listp = NULL; 
    maxBlockSize1 = 0; // set the max block size as 0
    maxBlockSize2 = 0; // set the second max block size as 0

    return 0;
 }

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t adjustedsize;             /*Adjusted block size*/
    size_t extendedsize;             /*Extend heap by this amount if no room*/
    char *bp;                        /*Block pointer*/

    int list_broken = 0;

    /* If requested size is less than 0 then ignore */
    if (size <= 0)
        return NULL;

    /* Adjust block size to include overhead and alignment requirements */
    adjustedsize = MAX(ALIGN(size) + DSIZE, OVERHEAD);    

    /* Search for a fit */
    if ((bp = find_fit(adjustedsize, &list_broken)) != NULL) 
    {
	/* Fit found, place and resize block if necessary */
        place(bp, adjustedsize, !list_broken);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendedsize = MAX(adjustedsize, CHUNKSIZE);

    /* If unable to extend heap space */
    if ((bp = extend_heap(extendedsize/WSIZE)) == NULL)
        return NULL;

    /* Place block and adjust freed space if necessary */
    place(bp, adjustedsize, 1);
    return bp;

}

/* 
 * extend_heap - Enlarge the heap
 * 
 */
void *extend_heap(size_t words)
{
    void *bp;
    size_t size;
        
    /* Maintain alignment */
    size = ALIGN(words * WSIZE);
    if ((int)(bp = mem_sbrk(size)) == -1) 
    {
        return NULL;
    }

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

    /* Coalesce */
    return coalesce(bp);
}

/* 
 * mm_free - Free block of memory 
 * From Textbook page 832
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp)); 

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr == NULL)
        return (mm_malloc(size));

    if(size == 0){
        mm_free(ptr);
        return ptr;
    }
      
    // get the proper alligned size
    size_t asize,currentSize;

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);


    currentSize = GET_SIZE(HDRP(ptr));


    if (asize < currentSize)
    {
        size_t splitBlockSize = currentSize-asize;
	
	/* If the split block is larger than the size of a block */ 
        if(splitBlockSize >= DSIZE )
        {
            /* Split the blocks */ 
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(currentSize-asize, 1));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(currentSize-asize, 1));

	    /* Free smaller block to be used */ 
            mm_free(NEXT_BLKP(ptr));
            return ptr;
        }
        else
        {
            return ptr;
        }
       
    }

    else if(asize > currentSize)
    {

        if(lastBlock(ptr) == 1)
        {
	    /* If current block is the last in the heap, then just enlarge */
            size_t payLoadSize = (asize- currentSize) + WSIZE;

            /* Enlarge heap */ 
            mem_sbrk(payLoadSize);
            size_t newSize = currentSize + payLoadSize;
            PUT(HDRP(ptr), PACK(newSize-WSIZE, 1));       // Reset the new header size
            PUT(FTRP(ptr), PACK(newSize-WSIZE, 1));       // Reset the new footer size
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));        // New epilogue header

            return ptr;
        }
        else
        {
            void* newptr = mm_malloc(size);
            memcpy(newptr, ptr, currentSize);
            mm_free(ptr);
            return newptr;
        }
    }
    else
    {
        return ptr;
    }
      
}

/*
 * place - Mark the block as allocated
*/
void place(void* bp, size_t asize, int call_delete)
{
  /* Get the current block size */
  size_t size = GET_SIZE(HDRP(bp));  

  if (call_delete == 1) 
  {
     deleteList(bp);
  }      
  
  PUT(HDRP(bp), PACK(size, 1));
  PUT(FTRP(bp), PACK(size, 1));
}

/*
 * find_fit - Traverse through heap to find block of size asize 
*/
void * find_fit(size_t asize, int *list_broken)
{
    //if(mm_check()){ 

	    if(asize>maxBlockSize1)
		return NULL;
	    size_t bestfit = mem_heapsize(); 
	    size_t curfit;

	    void *bestfitptr = NULL;
	    void *free_bp;
	    int sizethreshold = 2*DSIZE;

	    /* Traverse the heap looking for big enough block */
	    for (free_bp = free_listp; free_bp != NULL; free_bp = *((void **)free_bp))
	    {
		if (asize <= GET_SIZE(HDRP(free_bp)))
		{
		    curfit = GET_SIZE(HDRP(free_bp)) - asize;
		    if (curfit < 4*sizethreshold) {
		        bestfitptr = free_bp;
		        break;
		    }
		    if (curfit < bestfit) {
		        bestfit = curfit;
		        bestfitptr = free_bp;
		    }
		}
	    }

	    /* Block close to size not found, return closest size and split it */ 
	    if ((bestfitptr != NULL) && (GET_SIZE(HDRP(bestfitptr)) - asize >= sizethreshold)) {
		bestfitptr = split_block (bestfitptr, asize);
		*list_broken = 1;
	    }
	    return bestfitptr;
	//}
}

/*
 * Coalesce - Merge free blocks
*/
void *coalesce(void *bp)
{
    size_t next_alloc = GET_ALLOC((void *)(FTRP(bp)) + WSIZE);
    size_t prev_alloc = GET_ALLOC((void *)(bp) - 2*WSIZE);
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {              /* Case 1 */
    insertList(bp);              
        return bp;
    }

    else if (prev_alloc && !next_alloc) {       /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        deleteList(NEXT_BLKP(bp));              
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {       /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        deleteList(PREV_BLKP(bp));               
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {            			         /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        deleteList(PREV_BLKP(bp));               
        deleteList(NEXT_BLKP(bp));               
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }

	insertList(bp);                         // Insert onto free list
	//mm_check();				// Check
        return (bp);
}


/*
 * insertList - Insert a block onto the free list
*/
void insertList(void *bp)
{
    int blockSize = GET_SIZE(HDRP(bp));

    /* Current block is larger than max block size, reset maxBlockSize1 to current block size */
    if(blockSize > maxBlockSize1)
    {
        maxBlockSize1 = blockSize;
    }

    /* If current block is greater than second largest block size, but less than largest block
       size, then the current block is now the second largest block size */
    if(blockSize > maxBlockSize2 && blockSize < maxBlockSize1)
    {
        maxBlockSize2 = blockSize;
    }

    /* Set pointers */
    *((void **)bp) = free_listp;
    *((void **)((char *)bp + WSIZE)) = NULL;
    if (free_listp != NULL) {
        *((void **)((char *)free_listp + WSIZE)) = bp;
    }
    free_listp = bp; 
}

/* 
 * deleteList - Remove block pointer bp from the free list 
*/
void deleteList(void * bp)
{
    int blockSize = GET_SIZE(HDRP(bp));

    if(blockSize == maxBlockSize1)
    {
        maxBlockSize1 = maxBlockSize2;
    }

    void *nextFreeBlock, *prevFreeBlock;
    
    nextFreeBlock = *((void **)bp); 
    prevFreeBlock = *((void **)((char *)bp + WSIZE));
 
    // Block is in the middle
    if (nextFreeBlock != NULL && prevFreeBlock != NULL) {
        *((void **)prevFreeBlock) = nextFreeBlock;
        *((void **)((char *)nextFreeBlock + WSIZE)) = prevFreeBlock;
    }
    // Block is the first block
    if (nextFreeBlock == NULL && prevFreeBlock != NULL){
        *((void **)prevFreeBlock) = nextFreeBlock;
    }
    // Block is the last block
    if (nextFreeBlock != NULL && prevFreeBlock == NULL) {
        
        *((void **)((char *)nextFreeBlock + WSIZE)) = NULL;
        free_listp = nextFreeBlock;
    }
    // Block is the only block left of the free list
    if ((nextFreeBlock == NULL) && (prevFreeBlock == NULL)) {
        free_listp = NULL;
    }
}

/*
 * split_block - Split the block and return lower one 
*/
void *split_block (void *bp, size_t asize) {
    if (bp == NULL) {
        return NULL;
    }
    void *end_of_block, *new_bp;
    size_t old_size = GET_SIZE(HDRP(bp));
    end_of_block = (char *)bp + old_size - WSIZE;
    new_bp = (char *)end_of_block - asize + WSIZE;
    PUT(HDRP(new_bp), PACK(asize, 0));
    PUT(FTRP(new_bp), PACK(asize, 0));

    PUT(HDRP(bp), PACK(old_size-asize, 0));
    PUT(FTRP(bp), PACK(old_size-asize, 0));

    return new_bp;
}

/*
 * lastBlock - Check if epilogue block
*/
int lastBlock(void * bp)
{
    void * next = NEXT_BLKP(bp);
    if(next == NULL)
        return 0;

    if(GET_SIZE(HDRP(next))==0 && GET_ALLOC(HDRP(next)) == 0x1)
        return 1;
    else
        return 0;
}


/*
 * mm_check - Heap Consistency Checker
 * 
*/
int mm_check(void){

    void *traverse = NEXT_BLKP(heap_listp); //Traverse heap

    int heap_size_counter = 2*WSIZE;
    
    while (GET_SIZE(HDRP(traverse)) != 0) {
       
	// Are there any contiguous free blocks that somehow escaped coalescing? 
        if (GET_ALLOC(HDRP(traverse)) == 0 && GET_ALLOC(HDRP(NEXT_BLKP(traverse))) == 0) {
            printf("Blocks not coalesced\n");
            return 0;
        }

        heap_size_counter += GET_SIZE(HDRP(traverse));
        traverse = NEXT_BLKP(traverse);
    }

    // Is every block in the free list marked as free? 
    traverse = free_listp;
    while (traverse != NULL){
        if (GET_ALLOC(HDRP(traverse)) != 0 || GET_ALLOC(FTRP(traverse)) != 0) {
            printf ("Free block not marked as free\n");
            return 0;
        }
        traverse = NEXT_FREEP(traverse);
    }

    return 1;
} 


