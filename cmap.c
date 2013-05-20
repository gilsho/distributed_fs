/** 
 * File: cmap.c
 * ---------------
 * Implementation for CMap. This version uses hashtable, collisions
 * handled by chaining in linked list.  Each entry is struct with
 * key, value, and next pointer. jzelenski Fall 2009
 */

#include "cmap.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


    
    
// MAGIC field is not something they had to do, but something I did
// to bulletproof against client errors students are likely to make in assignment 2

#define MAGIC 94305

#define REHASH_LOAD 2

static void Rehash(CMap *cm);

typedef struct link {
    struct link *next;
    char key[]; // students will not know to do this, and we certainly don't expect it, but I couldn't resist :-), just simplfies access to next/key fields
} link;

struct CMapImplementation {
    int magic;
    link **buckets;
    int numBuckets, numEntries, keySize, valueSize;
    CMapKeyCompareFn compareFn;
    CMapCleanupValueFn cleanupFn;
};

// the helpers would be a bit messier if straight-up void * (as opposed to my var-len array)
// for each helper, I put the code to expect from students in comments. With these helpers
// written, rest of CMap implementation is nice & clear -- no other manual dissection
// of blob anywhere!
static link *GetNext(struct link *p)
{
    return p->next;
    // return *(void **)blob;
}

static void SetNext(struct link *p, struct link *val)
{
    p->next = val;
    // *(void **)blob = val;
}

static void *GetKey(const CMap *cm, struct link *p)
{
  return p->key;
    // return (char *)blob + sizeof(void *);
}

static void SetKey(const CMap *cm, struct link *p, const void *key)
{
   memcpy(GetKey(cm, p), key, cm->keySize);
}

static void *GetValue(const CMap *cm, struct link *p)
{
   char *key = GetKey(cm, p);
   return key + cm->keySize;
}

static void SetValue(const CMap *cm, struct link *p, const void *val)
{   
    memcpy(GetValue(cm,p), val, cm->valueSize);
}



/* Function: Hash
 * --------------
 * This function adapted from Eric Roberts' _The Art and Science of C_
 * It takes a string and uses it to derive a "hash code," which
 * is an integer in the range [0..NumBuckets-1]. The hash code is computed
 * using a method called "linear congruence." A similar function using this
 * method is described on page 144 of Kernighan and Ritchie. The choice of
 * the value for the Multiplier can have a significant effort on the
 * performance of the algorithm, but not on its correctness.
 * This hash function is case-sensitive, "ZELENSKI" and "Zelenski" are
 * not guaranteed to hash to same code.
 */
static int Hash(const void *key, int keySize, int numBuckets)
{
   unsigned long MULTIPLIER = 2630849305L;// constant
   unsigned long hashcode = 0;
   char *k = (char *)key;
   for (int i = 0; i < keySize; i++)  
      hashcode = hashcode * MULTIPLIER + k[i];  
   return hashcode % numBuckets;                                  
}

static void VerifyMagic(const CMap *cm, const char *fnName)
{
   if (cm->magic != MAGIC) {
      fprintf(stderr, "%s passed a pointer to a corrupted CMap.\n", fnName);
      assert(cm->magic != MAGIC);
   }
}


CMap *CMapCreate(int keySize, int valueSize, int capacityHint, 
                 CMapKeyCompareFn compareFn,
                 CMapCleanupValueFn cleanupFn)
{
   const int DEFAULT_CAPACITY = 99;
   assert(capacityHint >= 0);
   if (capacityHint == 0) capacityHint = DEFAULT_CAPACITY;
   assert(valueSize > 0);
   CMap *cm = malloc(sizeof(*cm));
   assert(cm != NULL);
   cm->magic = MAGIC;
   cm->numBuckets = capacityHint;
   cm->buckets = calloc(cm->numBuckets, sizeof(link *));
   assert(cm->buckets != NULL);
   cm->numEntries = 0;
   cm->keySize = keySize;
   cm->valueSize = valueSize;
   cm->compareFn = compareFn;
   cm->cleanupFn = cleanupFn;
   return cm;
}

static void CleanupAndFree(CMap *cm, link *cell)
{
	if (cm->cleanupFn != NULL) cm->cleanupFn(GetValue(cm, cell));
    free(cell);
}

void CMapDispose(CMap *cm)
{
   VerifyMagic(cm, __FUNCTION__);
   cm->magic = 0;
   for (int i = 0; i < cm->numBuckets; i++) {
       for (link *cur = cm->buckets[i]; cur != NULL; ) {
		   link *toDispose = cur;
           cur = GetNext(cur); // save next field before freeing cell
		   CleanupAndFree(cm, toDispose);
      }
   }
   free(cm->buckets);
   free(cm);
}

int CMapCount(const CMap *cm)
{ 
   VerifyMagic(cm, __FUNCTION__);
   return cm->numEntries;
}

static link *FindKey(const CMap *cm, link *head, const void *key, link **prev)
{
   for (link *cur = head; cur != NULL; cur = GetNext(cur)) {
      if (cm->compareFn(GetKey(cm, cur), key) == 0) return cur;
      if (prev) *prev = cur; // drag a previous pointer if needed by client
   }
   return NULL;
}

   
void *CMapGet(const CMap *cm, const void *key)
{ 
   VerifyMagic(cm, __FUNCTION__);
   int bucket = Hash(key, cm->numBuckets, cm->keySize);
   link *match = FindKey(cm, cm->buckets[bucket], key, NULL);
   return (match ? GetValue(cm, match): NULL);
}

static void *CreateNewCell(const CMap *cm, const void *key, 
                           const void *valueAddr, link *next)
{
    link *cell = malloc(sizeof(*cell) + cm->valueSize + cm->keySize);
    assert(cell != NULL);
    SetKey(cm, cell, key);
    SetValue(cm, cell, valueAddr);
    SetNext(cell, next);
    return cell;
}

      
void CMapPut(CMap *cm, const void *key, const void *valueAddr)
{
   VerifyMagic(cm, __FUNCTION__);
   int bucket = Hash(key, cm->numBuckets, cm->keySize);
   link *match = FindKey(cm, cm->buckets[bucket], key, NULL);
   if (match) {
      if (cm->cleanupFn) cm->cleanupFn(GetValue(cm, match));
       SetValue(cm, match, valueAddr); //overwrite existing value
   } else {
       cm->buckets[bucket] = CreateNewCell(cm, key, valueAddr, cm->buckets[bucket]);
       cm->numEntries++;
   }
   if (cm->numEntries/cm->numBuckets >= REHASH_LOAD) Rehash(cm);
}


void CMapRemove(CMap *cm, const void *key)
{ 
   VerifyMagic(cm, __FUNCTION__);
   int bucket = Hash(key, cm->numBuckets, cm->keySize);
	// this assign to prev works because next field is at base of cell (yes, I am lazy)
   link *prev = (link *)&cm->buckets[bucket];
   link *match = FindKey(cm, cm->buckets[bucket], key, &prev);
   if (match) {
      SetNext(prev, GetNext(match));
	  CleanupAndFree(cm, match);
      cm->numEntries--;
   }
}

static void Rehash(CMap *cm)
{
    int oldNBuckets = cm->numBuckets;
    cm->numBuckets *= 2;
    struct link **newBuckets = calloc(cm->numBuckets, sizeof(struct link *));

   for (int i = 0; i < oldNBuckets; i++) {
      link *next;
      for (link *cur = cm->buckets[i]; cur != NULL; cur = next) {
          next = GetNext(cur); // save this, before we overwrite it
          int newcode = Hash(GetKey(cm, cur), cm->numBuckets, cm->keySize);
          SetNext(cur, newBuckets[newcode]);
          newBuckets[newcode] = cur;
      }
   }
   free(cm->buckets);
   cm->buckets = newBuckets;
}

static void *FindNextKey(CMap *cm, int startBucket)
{
    for (int i = startBucket; i < cm->numBuckets; i++)
        if (cm->buckets[i]) return GetKey(cm, cm->buckets[i]);
    return NULL;
}

void *CMapFirst(CMap *cm)
{
    VerifyMagic(cm, __FUNCTION__);
    return FindNextKey(cm, 0);
}

void *CMapNext(CMap *cm, void *prevKey)
{
    VerifyMagic(cm, __FUNCTION__);
    void *cell = prevKey - sizeof(void *); // sketchy?!
    void *next = GetNext(cell);
    if (next)
        return GetKey(cm, next);
    else
        return FindNextKey(cm, 1 + Hash(prevKey, cm->numBuckets, cm->keySize));
}
