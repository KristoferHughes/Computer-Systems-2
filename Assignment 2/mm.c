/*
 * mm-naive.c - The fastest, least memory-efficient malloc package
 * Kristofer Hughes for Computer Systems II, Professor Heart
 * For my memory allocation approach, I primarily utilized the header and footer of each block
 * as the starting point, with an integral global list to track tags and separate mostly segregated lists
 * With this global list utilizing the segregated free list approach to cut down on the amount of code and variable usage
 * From my research there seems to be a variety of techniques for memory allocation
 * but the segregated free list approach absolutely spoke to me and seemed the most straightforward
 * The goal of this all is to basically divide and allocate the memory into different blocks
 * PS: I'm not used to writing high-level concept introductions for my code so I apologize
 * if this is somewhat short!
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#define ALIGNMENT 8 //wall of macros incoming!
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define DSIZE 8
#define ALIGNMENT 8
#define RUNNINGVAR     20   //maximum for the global list that will be implemented further down. 20 seems like a solid number, maybe change to 18 or 16 if needed 
#define SECONDBUFFER  (1<<7)   //buffer if needed
#define FIRSTSIZE (1<<6) //first size for each divided up memory block to be allocated
#define SIZETWO (1<<12) //size for the heap
#define SetBlock(size, alloc) ((size) | (alloc)) //sets up block size and allocation

/*  Read  a  word  at  address  p  */
#define GET(p)	 (*(unsigned int *)(p)) //integral!!!
#define findtag(p)   (GET(p) & 0x2) //grabs tag of block
#define assignTag(p, val)       (*(unsigned int *)(p) = (val) | findtag(p))
#define assignTagOff(p, val) (*(unsigned int *)(p) = (val))
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/*  Read  the  size  field  from  address  p  */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define footerP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)
#define NextBlock(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE)) //find next block address
#define PrevBlock(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE)) //find last block address
#define findAllocation(p) (GET(p) & 0x1)
#define findtagB(p)   (GET(p) |= 0x2) //find tag for block
#define reverseTag(p) (GET(p) &= ~0x2) //reverse tag for block

/*  Given  block  ptr  bp,  compute  address  of  its  header */
#define HDRP(ptr) ((char *)(ptr) - WSIZE)
#define findLastP(ptr) ((char *)(ptr)) //find last pointer
#define Maximum(x,y) ((x) > (y) ? (x) : (y)) //determine the maximum
#define Minimum(x,y) ((x) < (y) ? (x) : (y))  //determine the minimum
#define findFirstP(ptr) ((char *)(ptr) + WSIZE) //finds the first pointer
#define previous(ptr) (*(char **)(ptr)) //find previous ptr
#define following(ptr) (*(char **)(findFirstP(ptr))) //find next ptr

void *globalList[RUNNINGVAR]; //initiate global list to track tags

/*  Helper functions to streamline base functions */
static void insertion(void *ptr, size_t size); //inserts nodes
static void *extension(size_t size); //extends the heap
static void *merge(void *ptr); //merges cases
static void *final(void *ptr, size_t mallocVarA); //final function that determines blocks
static void deletion(void *ptr); //deletes nodes


static void *extension(size_t size) //reminder: MUST be first function to extend the heap
{
    void *ptr;                   
    size_t mallocVarA;
    
    mallocVarA = ALIGN(size);
    
    if ((ptr = mem_sbrk(mallocVarA)) == (void *)-1)
        return NULL;
     
    assignTagOff(HDRP(ptr), SetBlock(mallocVarA, 0));  //header created
    assignTagOff(footerP(ptr), SetBlock(mallocVarA, 0));  //footer created
    assignTagOff(HDRP(NextBlock(ptr)), SetBlock(0, 1)); //next header
    insertion(ptr, mallocVarA);
    return merge(ptr); //initializes merge
}

static void insertion(void *ptr, size_t size) { //maybe switch places of this with merge?
    int insertComparator = 0;
    void *firstPTR = ptr;
    void *secondPTR = NULL;
    
    while ((insertComparator < RUNNINGVAR - 1) && (size > 1)) {
        size >>= 1;
        insertComparator++;
    }
    
    firstPTR = globalList[insertComparator];
    while ((firstPTR != NULL) && (size > GET_SIZE(HDRP(firstPTR)))) {
        secondPTR = firstPTR;
        firstPTR = previous(firstPTR);
    }
    
    if (firstPTR != NULL) { //check if first pointer used to search is null, if so...
        if (secondPTR != NULL) { //then check if second pointer used for the insert is null
            SET_PTR(findLastP(ptr), firstPTR);
            SET_PTR(findFirstP(firstPTR), ptr);
            SET_PTR(findFirstP(ptr), secondPTR);
            SET_PTR(findLastP(secondPTR), ptr);
        } else {
            SET_PTR(findLastP(ptr), firstPTR);
            SET_PTR(findFirstP(firstPTR), ptr);
            SET_PTR(findFirstP(ptr), NULL);
            globalList[insertComparator] = ptr;
        }
    } else {
        if (secondPTR != NULL) { 
            SET_PTR(findLastP(ptr), NULL);
            SET_PTR(findFirstP(ptr), secondPTR);
            SET_PTR(findLastP(secondPTR), ptr);
        } else {
            SET_PTR(findLastP(ptr), NULL);
            SET_PTR(findFirstP(ptr), NULL);
            globalList[insertComparator] = ptr;
        }
    }
    
    return; //need to put in a return statement but no need to return a variable for this
}


static void deletion(void *ptr) {
    int deleteCompar = 0;
    size_t size = GET_SIZE(HDRP(ptr));
    
    while ((deleteCompar < RUNNINGVAR - 1) && (size > 1)) {
        size >>= 1;
        deleteCompar++;
    }
    
    if (previous(ptr) != NULL) {
        if (following(ptr) != NULL) {
            SET_PTR(findFirstP(previous(ptr)), following(ptr));
            SET_PTR(findLastP(following(ptr)), previous(ptr));
        } else {
            SET_PTR(findFirstP(previous(ptr)), NULL);
            globalList[deleteCompar] = previous(ptr);
        }
    } else {
        if (following(ptr) != NULL) {
            SET_PTR(findLastP(following(ptr)), NULL);
        } else {
            globalList[deleteCompar] = NULL;
        }
    }
    
    return; //need to put in a return statement but no need to return a variable for this
}


static void *merge(void *ptr) //merging cases
{
    size_t mergeAllocA = findAllocation(HDRP(PrevBlock(ptr)));
    size_t mergeAllocB = findAllocation(HDRP(NextBlock(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if (findtag(HDRP(PrevBlock(ptr))))
        mergeAllocA = 1; //maybe switch places with if (mergeAllocA && mergeAllocB) statement?

    if (mergeAllocA && mergeAllocB) {                    
        return ptr;
    }
    else if (mergeAllocA && !mergeAllocB) {            
        deletion(ptr); //delete the ptr
        deletion(NextBlock(ptr)); //delete the next block's ptr
        size += GET_SIZE(HDRP(NextBlock(ptr)));
        assignTag(HDRP(ptr), SetBlock(size, 0));
        assignTag(footerP(ptr), SetBlock(size, 0));
    } else if (!mergeAllocA && mergeAllocB) {         
        deletion(ptr); //delete the ptr
        deletion(PrevBlock(ptr)); //delete the previous block's ptr
        size += GET_SIZE(HDRP(PrevBlock(ptr)));
        assignTag(footerP(ptr), SetBlock(size, 0)); //assign a new tag, footer ptr, and set the block size
        assignTag(HDRP(PrevBlock(ptr)), SetBlock(size, 0)); //assign a new tag, header ptr, and set the block size
        ptr = PrevBlock(ptr);
    } else {                                 
        deletion(ptr); //delete the ptr
        deletion(PrevBlock(ptr)); //delete the previous block's ptr
        deletion(NextBlock(ptr)); //delete the next block's ptr
        size += GET_SIZE(HDRP(PrevBlock(ptr))) + GET_SIZE(HDRP(NextBlock(ptr)));
        assignTag(HDRP(PrevBlock(ptr)), SetBlock(size, 0));
        assignTag(footerP(NextBlock(ptr)), SetBlock(size, 0));
        ptr = PrevBlock(ptr);
    }
    
    insertion(ptr, size); //inserting now that the merge has been complete
    return ptr; //return the pointer to make sure it works
}

static void *final(void *ptr, size_t mallocVarA) //reminder: MUST be last helper function
{
    size_t ptr_size = GET_SIZE(HDRP(ptr)); //read the size field of the header
    size_t newBuffer = ptr_size - mallocVarA; //attribute from malloc function, reused for simplicity/recognition
    deletion(ptr); //delete the ptr
     
    if (newBuffer <= DSIZE * 2) {
        assignTag(HDRP(ptr), SetBlock(ptr_size, 1)); 
        assignTag(footerP(ptr), SetBlock(ptr_size, 1)); 
    }
    
    else if (mallocVarA >= 100) {
        assignTag(HDRP(ptr), SetBlock(newBuffer, 0));
        assignTag(footerP(ptr), SetBlock(newBuffer, 0));
        assignTagOff(HDRP(NextBlock(ptr)), SetBlock(mallocVarA, 1));
        assignTagOff(footerP(NextBlock(ptr)), SetBlock(mallocVarA, 1));
        insertion(ptr, newBuffer);
        return NextBlock(ptr);
        
    }
    
    else {
        assignTag(HDRP(ptr), SetBlock(mallocVarA, 1)); 
        assignTag(footerP(ptr), SetBlock(mallocVarA, 1)); 
        assignTagOff(HDRP(NextBlock(ptr)), SetBlock(newBuffer, 0)); 
        assignTagOff(footerP(NextBlock(ptr)), SetBlock(newBuffer, 0)); 
        insertion(NextBlock(ptr), newBuffer);
    }
    return ptr;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int initVar;         
    char *initialChar; // Pointer to beginning of heap
    
    for (initVar = 0; initVar < RUNNINGVAR; initVar++) {
        globalList[initVar] = NULL; //setting global list to null
    }
  
    if ((long)(initialChar = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    assignTagOff(initialChar, 0);  //integral         
    assignTagOff(initialChar + (1 * WSIZE), SetBlock(DSIZE, 1)); //header
    assignTagOff(initialChar + (2 * WSIZE), SetBlock(DSIZE, 1)); 
    assignTagOff(initialChar + (3 * WSIZE), SetBlock(0, 1));     
    
    if (extension(FIRSTSIZE) == NULL)
        return -1;
    
    return 0; //return 0 for the initialization
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t mallocVarA;  
    size_t mallocVarB;
    void *ptr = NULL;  //null pointer
    
    if (size == 0)
        return NULL; //remove this code section if there isn't any 0 size cases in Makefile?
    
    if (size <= DSIZE) {
        mallocVarA = 2 * DSIZE;
    } else {
        mallocVarA = ALIGN(size+DSIZE);
    }
    
    int mallocInt = 0; 
    size_t newsize = mallocVarA; //changing variable included in initial function version in handout.tar to previously used variable
    while (mallocInt < RUNNINGVAR) { //allocating the block
        if ((mallocInt == RUNNINGVAR - 1) || ((newsize <= 1) && (globalList[mallocInt] != NULL))) {
            ptr = globalList[mallocInt];
            while ((ptr != NULL) && ((mallocVarA > GET_SIZE(HDRP(ptr))) || (findtag(HDRP(ptr)))))
            {
                ptr = previous(ptr);
            }
            if (ptr != NULL)
                break;
        }
        
        newsize >>= 1; //NOTE TO SELF: after redoing the final section this probably isn't doing anything for now, delete?
        mallocInt++; //^^^ditto
    }
    
    if (ptr == NULL) {
        mallocVarB = Maximum(mallocVarA, SIZETWO);
        
        if ((ptr = extension(mallocVarB)) == NULL)
            return NULL;
    }
    
    ptr = final(ptr, mallocVarA);
    return ptr; //return ptr to show and finalize block allocation
}

/*
 * mm_free - Currently, freeing a block does nothing.
 * 	You must revise this function so that it frees the block.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
 
    reverseTag(HDRP(NextBlock(ptr)));
    assignTag(HDRP(ptr), SetBlock(size, 0));
    assignTag(footerP(ptr), SetBlock(size, 0));
    
    insertion(ptr, size); //insert first, then merge
    merge(ptr); //cases are now merged!
    
    return; //block should be freed by time of the return statement if helper functions work
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 *
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;    //reusing variable from placeholder function in handout
    size_t copySize = size;
    int newBuffer; //buffer to compare against normal buffer
    int mallocVarB;  //reusing second var from malloc function for simplicity/recognition, not sure if this carries over from mm_malloc at the moment though?
    int bufferSIZE; 
    
    if (size == 0)
        return NULL; //remove this code section if there isn't any 0 size cases in Makefile?
    
    if (copySize <= DSIZE) {
        copySize = 2 * DSIZE;
    } else {
        copySize = ALIGN(size+DSIZE);
    }
    
    copySize += SECONDBUFFER;
    bufferSIZE = GET_SIZE(HDRP(ptr)) - copySize;
    
    if (bufferSIZE < 0) {
        if (!findAllocation(HDRP(NextBlock(ptr))) || !GET_SIZE(HDRP(NextBlock(ptr)))) {
            newBuffer = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NextBlock(ptr))) - copySize;
            if (newBuffer < 0) {
                mallocVarB = Maximum(-newBuffer, SIZETWO);
                if (extension(mallocVarB) == NULL)
                    return NULL;
                newBuffer += mallocVarB;
            }
                
            deletion(NextBlock(ptr));
            assignTagOff(HDRP(ptr), SetBlock(copySize + newBuffer, 1)); 
            assignTagOff(footerP(ptr), SetBlock(copySize + newBuffer, 1)); 
        } else {
            oldptr = mm_malloc(copySize - DSIZE);
            memcpy(oldptr, ptr, Minimum(size, copySize));
            mm_free(ptr);
        }
        bufferSIZE = GET_SIZE(HDRP(oldptr)) - copySize; //change buffer size
    }
    
    if (bufferSIZE < 2 * SECONDBUFFER) //compare buffer size to buffer
        findtagB(HDRP(NextBlock(oldptr)));
    
    return oldptr; //time to return the initial variable and be done!
}










