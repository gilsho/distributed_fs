/** 
 * File: cmap.h
 * ------------
 * Defines the interface for the CMap type.
 *
 * The CMap manages a collection of key-value pairs or "entries". The keys 
 * are always of type char *, but the values can be of any desired type. The
 * main operations are to associate a value with a key and to retrieve 
 * the value associated with a key.  In order to work for all types of values,
 * all CMap values must be passed and returned via (void *) pointers.
 * Given its extensive use of untyped pointers, the CMap is a bit tricky 
 * to use correctly as a client. Be diligent!
 *
 * CS107 jzelenski
 */

#ifndef _cmap_h
#define _cmap_h


/**
 * Type: CMapCleanupValueFn
 * ------------------------
 * CMapCleanupValueFn is the typename for a pointer to a client-supplied
 * cleanup function. Such function pointers are passed to CMapCreate to apply
 * to a value when it is removed/disposed. The cleanup function takes one 
 * void* pointer that points to the value being removed.
 */
typedef void (*CMapCleanupValueFn)(void *valueAddr);


/**
 * Type: CMapKeyCompareFn
 * ------------------------
 * CMapKeyCompareFn is the typename for a pointer to a client-supplied
 * compare function. This function is supplied by the user when the map is 
 * created and is used to find the matching element to a specified key matching elements

 Such function pointers are passed to CMapCreate to apply
 * to a value when it is removed/disposed. The cleanup function takes one 
 * void* pointer that points to the value being removed.
 */
typedef int (*CMapKeyCompareFn)(const void *valueAddr1, const void *valueAddr2);


/**
 * Type: CMap
 * ----------
 * Defines the CMap type. The type is "incomplete", i.e. deliberately
 * avoids stating the field names/types for the struct CMapImplementation. 
 * (That struct is completed in the implementation code not visible to
 * clients). The incomplete type forces the client to respect the privacy
 * of the representation. Client declare variables only of type CMap * 
 * (pointers only, never of the actual struct) and cannot never dereference 
 * a CMap * nor attempt to read/write its internal fields. A CMap 
 * is manipulated solely through the functions listed in this interface
 * (this is analogous to how you use a FILE *).
 */
typedef struct CMapImplementation CMap;


/** 
 * Function: CMapCreate
 * Usage: CMap *mymap = CMapCreate(sizeof(int), 10, NULL);
 * -------------------------------------------------------
 * Creates a new empty CMap and returns a pointer to it. The pointer 
 * is to storage allocated in the heap. When done with the CMap, the
 * client must call CMapDispose to dispose of it. If allocation fails, an
 * assert is raised.
 *
 * The valueSize parameter specifies the size, in bytes, of the
 * values that will be stored in the CMap. For example, to store values of
 * type double, the client passes sizeof(double) for the valueSize. Note that 
 * all values stored in a given CMap must be of the same type. An assert is 
 * raised if valueSize is not greater than zero.
 *
 * The capacityHint parameter allows the client to tune the resizing behavior.
 * The CMap's internal storage will initially be optimized to hold
 * the number ov entries hinted. This capacityHint is not a binding limit. 
 * Whenever the capacity is outgrown, the capacity will double in size. 
 * If planning to store many entries, specifying a large capacityHint will
 * result in an appropriately large initial allocation and fewer resizing
 * operations later.  For a small map, a small capacityHint will result in
 * several smaller allocations and potentially less waste.  If capacityHint 
 * is 0, an internal default value is used. An assert is raised if capacityHint
 * is negative.
 *
 * The cleanupFn is a client callback that will be called on a value being
 * removed/replaced (via CMapRemove/CMapPut, respectively) and on every value 
 * in the CMap when it is destroyed (via CMapDispose). The client can use this
 * function to do any deallocation/cleanup required for the value, such as
 * freeing memory to which the value points (if the value is or contains a
 * pointer). The client can pass NULL for cleanupFn if values don't
 * require any special cleanup.
 */
CMap *CMapCreate(int keySize, int valueSize, int capacityHint, 
                 CMapKeyCompareFn compareFn,
                 CMapCleanupValueFn cleanupFn);
/**
 * Function: CMapDispose
 * Usage:  CMapDispose(mymap);
 * ---------------------------
 * Disposes of the CMap. Calls the client's cleanup function on all values
 * and deallocates memory used for the CMap's storage, including the keys
 * that were copied. Operates in linear-time.
 */
void CMapDispose(CMap *cm);


/**
 * Function: CMapCount
 * Usage:  int count = CMapCount(mymap);
 * -------------------------------------
 * Returns the number of entries currently stored in the CMap. Operates in
 * constant-time.
 */
int CMapCount(const CMap *cm);


/**
 * Function: CMapPut
 * Usage:  CMapPut(mymap, "CS107", &val);
 * --------------------------------------
 * Associates the given key with a new value in the CMap. If there is an
 * existing value for the key, it is replaced with the new value. Before 
 * being overwritten, the client's cleanupFn is called on the old value.
 * valueAddr is expected to be a valid pointer and the new value is copied
 * from the memory pointed to by valueAddr. A copy of key string is stored
 * internally by the CMap. Note that keys are compared case-sensitively, 
 * e.g. "binky" is not the same key as "BinKy". The capacity is enlarged 
 * if necessary, an assert is raised on allocation failure. Operates in
 * constant-time (amortized).
 */
void CMapPut(CMap *cm, const void *key, const void *valueAddr);


/**
 * Function: CMapGet
 * Usage:  int val = *(int *)CMapGet(mymap, "CS107");
 * --------------------------------------------------
 * Searches the CMap for an entry with the given key and if found, returns
 * a pointer to its associated value.  If key is not found, then NULL is 
 * returned as a sentinel.  Note this function returns a pointer into the 
 * CMap's storage, so the pointer should be used with care. In particular, 
 * the pointer returned by CMapGet can become invalid after a call that adds 
 * or removes entries within the CMap.  Note that keys are compared
 * case-sensitively,  e.g. "binky" is not the same key as "BinKy". 
 * Operates in constant-time.
 */
void *CMapGet(const CMap *cm, const void * key);


/**
 * Function: CMapRemove
 * Usage:  CMapRemove(mymap, "CS107");
 * -----------------------------------
 * Searches the CMap for an entry with the given key and if found, removes that
 * key and its associated value.  If key is not found, no changes are made. 
 * The client's cleanupFn is called on the removed value and the copy of the
 * key string is deallocated. Note that keys are compared case-sensitively, 
 * e.g. "binky" is not the same key as "BinKy". Operates in constant-time.
 */
void CMapRemove(CMap *cm, const void * key);


/**
 * Functions: CMapFirst, CMapNext
 * Usage: for (char *key = CMapFirst(cm); key != NULL; key = CMapNext(cm, key))
 * ----------------------------------------------------------------------------
 * These functions provide iteration over the CMap keys. The client
 * starts an iteration with a call to CMapFirst which returns the first
 * key of the CMap or NULL if the CMap is empty. The client loop calls 
 * CMapNext passing the previous key and receives the next key in the iteration
 * or NULL if there are no more keys. Keys are iterated in arbitrary order.
 * The argument to CMapNext is expected to be a valid pointer to a key string
 * as returned by a previous call to CMapFirst/CMapNext. The CMap supports
 * multiple iterations at same time without interference, however, the client
 * should not add/remove/rearrange CMap entries in the midst of iterating.
 */
void *CMapFirst(CMap *cm);
void *CMapNext(CMap *cm, void *prevKey);
     
#endif
