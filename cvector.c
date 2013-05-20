/** 
 * File: cvector.c
 * ---------------
 * Implementation for CVector. I wrote it again from scratch just to remind
 * myself of what's it's like :-)   jzelenski Fall 2009
 *
 * CVector is backed by a dynamically (re)allocated array. Contiguous region,
 * 0-indexed, just like a regular C array, but with convenience of memory
 * management, insert/remove, track length, search, sort, etc.
 */

#include "cvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

struct CVectorImplementation {
    int magic;
    void *elems;
    int elemSize;
    int numAllocated, numUsed;
    CVectorCleanupElemFn cleanupFn;
};



// This is not something we expected them to do, I added it as extra check
// to help them debug their client use
#define MAGIC 107

// helper to do the manual array elem addressing, used everywhere!
static void *NthAddress(const CVector *cv, int n)
{
  return (char *)cv->elems + (n)*cv->elemSize;
}  
    

static void VerifyMagic(const CVector *cv, const char *fnName)
{
   if (cv->magic != MAGIC) {
      fprintf(stderr, "%s called on pointer to corrupted CVector!\n", fnName);
      assert(cv->magic == MAGIC);
   }
}

CVector *CVectorCreate(int elemSize, int capacityHint, CVectorCleanupElemFn cleanupFn)
{
	int CAPACITY_DEFAULT = 10; // constant used when user doesn't specify
   assert(elemSize > 0 && capacityHint >= 0);
   if (capacityHint == 0) capacityHint = CAPACITY_DEFAULT;
   CVector *cv = malloc(sizeof(*cv));
   assert(cv != NULL);
   cv->magic = MAGIC;
   cv->elemSize = elemSize;
   cv->cleanupFn = cleanupFn;
   cv->numUsed = 0;
   cv->numAllocated = capacityHint; // initial size
   cv->elems = malloc(elemSize*cv->numAllocated);
   assert(cv->elems != NULL);
   return cv;
}

void CVectorDispose(CVector *cv)
{
   VerifyMagic(cv, __FUNCTION__);
   cv->magic = 0; // write to magic field so we will reject use of freed CVec
   if (cv->cleanupFn != NULL)
      for (int i = 0; i < cv->numUsed; i++)
	 	cv->cleanupFn(NthAddress(cv, i));
   free(cv->elems);
   free(cv);
}

int CVectorCount(const CVector *cv)
{
   VerifyMagic(cv, __FUNCTION__);
   return cv->numUsed; 
}

void *CVectorNth(const CVector *cv, int atIndex)
{
   VerifyMagic(cv, __FUNCTION__);
   assert(atIndex >= 0 && atIndex < cv->numUsed);
   return NthAddress(cv, atIndex);
}

void CVectorReplace(CVector *cv, const void *elemAddr, int atIndex)
{
   VerifyMagic(cv, __FUNCTION__);
   assert(atIndex >= 0 && atIndex < cv->numUsed);
   if (cv->cleanupFn != NULL) cv->cleanupFn(NthAddress(cv, atIndex));
   memcpy(NthAddress(cv, atIndex), elemAddr, cv->elemSize);
}

void CVectorInsert(CVector *cv, const void *elemAddr, int atIndex)
{
   VerifyMagic(cv, __FUNCTION__);
   assert(atIndex >= 0 && atIndex <= cv->numUsed);
   if (cv->numUsed == cv->numAllocated) {
      cv->numAllocated *= 2;
      cv->elems = realloc(cv->elems, cv->elemSize*cv->numAllocated);
      assert(cv->elems != NULL);
   }
   memmove(NthAddress(cv, atIndex+1), NthAddress(cv, atIndex), cv->elemSize*(cv->numUsed-atIndex));
   memcpy(NthAddress(cv, atIndex), elemAddr, cv->elemSize);
   cv->numUsed++;
}

void CVectorAppend(CVector *cv, const void *elemAddr)
{
   VerifyMagic(cv, __FUNCTION__);
   CVectorInsert(cv, elemAddr, cv->numUsed);
}

void CVectorRemove(CVector *cv, int atIndex)
{
   VerifyMagic(cv, __FUNCTION__);
   assert(atIndex >= 0 && atIndex < cv->numUsed);
   if (cv->cleanupFn != NULL) cv->cleanupFn(NthAddress(cv, atIndex));
   memmove(NthAddress(cv, atIndex), NthAddress(cv, atIndex+1), cv->elemSize*(cv->numUsed-atIndex-1));
   cv->numUsed--;
}

void CVectorSort(CVector *cv, CVectorCmpElemFn cmpfn)
{
   VerifyMagic(cv, __FUNCTION__);
   qsort(cv->elems, cv->numUsed, cv->elemSize, cmpfn);
}


int CVectorSearch(const CVector *cv, const void *key, CVectorCmpElemFn cmpfn, int startIndex, bool isSorted)
{
   VerifyMagic(cv, __FUNCTION__);
   assert(startIndex >= 0 && startIndex <= cv->numUsed);
   size_t nelems = cv->numUsed-startIndex;
   void *found;
   if (isSorted)
      found = bsearch(key, NthAddress(cv, startIndex), nelems, cv->elemSize, cmpfn);
   else
      found = lfind(key, NthAddress(cv, startIndex), &nelems, cv->elemSize, cmpfn);
   return (found == NULL ?  -1 : ((char *)found - (char *)cv->elems)/cv->elemSize);
}

void *CVectorFirst(CVector *cv)
{
    VerifyMagic(cv, __FUNCTION__);
    return (cv->numUsed == 0? NULL : cv->elems);
}

void *CVectorNext(CVector *cv, void *prev)
{
    VerifyMagic(cv, __FUNCTION__);
    int index = ((char *)prev - (char *)cv->elems)/cv->elemSize;
    return (index == cv->numUsed - 1? NULL: (char *)prev + cv->elemSize);
}
    
