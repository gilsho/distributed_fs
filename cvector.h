/** 
 * File: cvector.h
 * ---------------
 * Defines the interface for the CVector type.
 *
 * The CVector manages a linear, indexed collection of homogeneous elements.
 * Think of it as an upgrade from the raw C array, with convenient dynamic
 * memory management (grow/shrink), operations to insert/remove, and
 * sort and search facilities. In order to work for all types of elements,
 * the size of each element is specified when creating the CVector, and
 * all CVector elements must be passed and returned via void* pointers.
 * Given its extensive use of untyped pointers, the CVector is a bit tricky 
 * to use correctly as a client. Be diligent!
 *
 * CS107 jzelenski
 */

#ifndef _cvector_h
#define _cvector_h

#include <stdbool.h>	//  this header defines C99 bool type

/**
 * Type: CVectorCmpElemFn
 * ----------------------
 * CVectorCmpElemFn is the typename for a pointer to a client-supplied 
 * comparator function. Such functions are passed to CVector to sort or 
 * search for elements. The comparator takes two const void* pointers, 
 * each of which points to an element of the type stored in the CVector, 
 * and returns an integer. That integer should indicate the ordering of 
 * the two elements using the same convention as the strcmp library function:
 * 
 *   If element at elemAddr1 < element at elemAddr2, return a negative number
 *   If element at elemAddr1 > element at elemAddr2, return a positive number
 *   If element at elemAddr1 = element at elemAddr2, return zero
 */
typedef int (*CVectorCmpElemFn)(const void *elemAddr1, const void *elemAddr2);


/** 
 * Type: CVectorCleanupElemFn
 * --------------------------
 * CVectorCleanupElemFn is the typename for a pointer to a client-supplied 
 * cleanup function. Such function pointers are passed to CVectorCreate to 
 * apply to an element when it is removed/replaced/disposed. The cleanup 
 * function takes one void* pointer that points to the element being removed.
 */
typedef void (*CVectorCleanupElemFn)(void *elemAddr);


/**
 * Type: CVector
 * -------------
 * Defines the CVector type. The type is "incomplete", i.e. deliberately
 * avoids stating the field names/types for the struct CVectorImplementation. 
 * (That struct is completed in the implementation code not visible to
 * clients). The incomplete type forces the client to respect the privacy
 * of the representation. Client declare variables only of type CVector * 
 * (pointers only, never of the actual struct) and cannot never dereference 
 * a CVector * nor attempt to read/write its internal fields. A CVector 
 * is manipulated solely through the functions listed in this interface
 * (this is analogous to how you use a FILE *).
 */
typedef struct CVectorImplementation CVector;


/**
 * Function: CVectorCreate
 * Usage: CVector *myvec = CVectorCreate(sizeof(int), 10, NULL);
 * -------------------------------------------------------------
 * Creates a new empty CVector and returns a pointer to it. The pointer 
 * is to storage allocated in the heap. When done with the CVector, the
 * client should call CVectorDispose to dispose of it. If allocation fails,
 * an assert is raised.
 *
 * The elemSize parameter specifies the size, in bytes, of the elements
 * that will be stored in the CVector. For example, to store elements of type
 * double, the client passes sizeof(double) for the elemSize. Note that all
 * elements stored in a given CVector must be of the same type. An assert is 
 * raised if elemSize is not greater than zero.
 *
 * The capacityHint parameter allows the client to tune the resizing behavior.
 * The CVector's internal storage will initially be allocated to hold the 
 * number of elements hinted. This capacityHint is not a binding limit. 
 * Whenever the capacity is outgrown, the capacity will double in size.
 * If planning to store many elements, specifying a large capacityHint will
 * result in an appropriately large initial allocation and fewer resizing 
 * operations later. For a small vector, a small capacityHint will result in 
 * several smaller allocations and potentially less waste. If capacityHint 
 * is 0, an internal default value is used. An assert is raised if capacityHint
 * is negative.
 *
 * The cleanupFn is a client callback function that will be called on an 
 * element being removed/replaced (via CVectorRemove/CVectorReplace, respectively) 
 * and on every element in the CVector when it is destroyed (via CVectorDispose). 
 * The client can use this function to do any deallocation/cleanup required 
 * for the element, such as freeing any memory to which the element points (if 
 * the element itself is or contains a pointer). The client can pass NULL for 
 * cleanupFn if elements don't require any special cleanup.
 */
CVector *CVectorCreate(int elemSize, int capacityHint, CVectorCleanupElemFn cleanupFn);


/**
 * Function: CVectorDispose
 * Usage:  CVectorDispose(myvec);
 * ------------------------------
 * Disposes of the CVector. Calls the client's cleanup function on all elements
 * and deallocates memory used for the CVector's storage. Operates in linear-time.
 */
void CVectorDispose(CVector *cv);


/**
 * Function: CVectorCount
 * Usage:  int count = CVectorCount(myvec);
 * ----------------------------------------
 * Returns the number of elements currently stored in the CVector.
 * Operates in constant-time.
 */
int CVectorCount(const CVector *cv);


/**
 * Method: CVectorNth
 * Usage:  int num = *(int *)CVectorNth(myvec, 0);
 * -----------------------------------------------
 * Returns a pointer to the element stored at a given index in the CVector. 
 * Valid indexes are 0 to count-1. An assert is raised if atIndex is out
 * of bounds. This function returns a pointer into the CVector's storage
 * so it must be used with care. In particular, a pointer returned by 
 * CVectorNth can become invalid after a call that adds, removes, or
 * rearranges elements within the CVector. The CVector could have been 
 * designed without this direct access, but it is useful and efficient to offer 
 * it, despite its potential pitfalls. Operates in constant-time.
 */ 
void *CVectorNth(const CVector *v, int atIndex);


/**
 * Function: CVectorInsert
 * Usage:  CVectorInsert(myvec, &elem, 0);
 * ---------------------------------------
 * Inserts a new element into the CVector, placing it at the given index
 * and shifting up other elements to make room. An assert is raised if atIndex 
 * is less than 0 or greater than the count. elemAddr is expected to be
 * a valid pointer and the new element is copied from the memory pointed to
 * by elemAddr. The capacity is enlarged if necessary, an assert is raised
 * on allocation failure. Operates in linear-time.
 */
void CVectorInsert(CVector *v, const void *elemAddr, int atIndex);


/**
 * Function: CVectorAppend
 * Usage:  CVectorAppend(myvec, &elem);
 * ------------------------------------
 * Appends a new element to the end of the CVector. elemAddr is expected to 
 * be a valid pointer and the new element is copied from the memory pointed 
 * to by elemAddr. The capacity is enlarged if necessary, an assert is raised 
 * on allocation failure. Operates in constant-time (amortized).
 */
void CVectorAppend(CVector *v, const void *elemAddr);
  
  
/**
 * Function: CVectorReplace
 * Usage:  CVectorReplace(myvec, &elem, 0);
 * ----------------------------------------
 * Overwrites the element at the given index with a new element. Before 
 * being overwritten, the client's cleanupFn is called on the old
 * element. elemAddr is expected to be a valid pointer and the new element 
 * is copied from the memory pointed to by elemAddr and replaces the 
 * old element at index. An assert is raised if atIndex is out of bounds. 
 * Operates in constant-time.
 */
void CVectorReplace(CVector *v, const void *elemAddr, int atIndex);


/**
 * Function: CVectorRemove
 * Usage:  CVectorRemove(myvec, 0);
 * --------------------------------
 * Removes the element at the given index from the CVector and shifts
 * other elements down to close the gap. The client's cleanupFn is called
 * on the element being removed. An assert is raised if atIndex is out of 
 * bounds. Operates in linear-time.
 */
void CVectorRemove(CVector *v, int atIndex);
  
  
/**
 * Function: CVectorSearch
 * Usage:  int found = CVectorSearch(myvec, &key, MyCmpElem, 0, false);
 * --------------------------------------------------------------------
 * Searches the CVector for an element matching the element to which the
 * given key points. It uses the provided comparefn callback to compare
 * elements. The search considers all elements from startIndex to end. 
 * To search the entire CVector, specify a startIndex of 0. 
 * The isSorted parameter allows the client to specify that elements
 * are currently stored in sorted order, in which case CVectorSearch uses a
 * faster binary search. If isSorted is false, linear search is used instead.
 * If a match is found, the index of the matching element is returned;
 * else -1 is returned. This function does not re-arrange/modify elements
 * within the CVector or modify the key. Operates in linear-time or
 * logarithmic-time (sorted).
 * 
 * An assert is raised if startIndex is less than 0 or greater than the count
 * (although searching from count will never find anything, allowing this case 
 * means client can search an empty CVector from 0 without getting an assert). 
 */  
int CVectorSearch(const CVector *v, const void *key, CVectorCmpElemFn comparefn, int startIndex, bool isSorted);


/**
 * Function: CVectorSort
 * Usage:  CVectorSort(myvec, MyCmpElem);
 * --------------------------------------
 * Rearranges elements in the CVector into ascending order according to the 
 * provided comparator function. Operates in NlgN-time.
 */
void CVectorSort(CVector *v, CVectorCmpElemFn comparefn);


/**
 * Functions: CVectorFirst, CVectorNext
 * Usage: for (void *cur = CVectorFirst(v); cur != NULL; cur = CVectorNext(v, cur))
 * --------------------------------------------------------------------------------
 * These functions provide iteration over the CVector elements. The client
 * starts an iteration with a call to CVectorFirst which returns a pointer
 * to the first (0th) element of the CVector or NULL if the CVector is empty.
 * The client loop calls CVectorNext passing the pointer to the previous
 * element and receives a pointer to the next element in the iteration
 * or NULL if there are no more elements. Elements are iterated in order
 * of increasing index (from 0 to N-1).
 * The argument to CVectorNext is expected to be a valid pointer to an element 
 * as returned by a previous call to CVectorFirst/CVectorNext. CVector supports
 * multiple iterations at same time without interference, however, the client
 * should not add/remove/rearrange CVector elements in the midst of iterating.
 */
void *CVectorFirst(CVector *cv);
void *CVectorNext(CVector *cv, void *prev);

#endif
