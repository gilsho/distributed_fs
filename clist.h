
#ifndef __LIST_H__
#define __LIST_H__

typedef int (*CListCmpElemFn)(const void *elemAddr1, 
															const void *elemAddr2);

typedef void (*CListCleanupElemFn)(void *elemAddr);

struct node {
	struct node *next;
	struct node *prev;
	void *data;
};

struct list {
	struct node *head;
	struct node *tail;
	CListCleanupElemFn cleanupFn;
	int count;
};

typedef struct list CList;

CList * CListInit(CListCleanupElemFn cleanupFn);

void
CListInsertSort(CList *cl, CListCmpElemFn compareFn, 
							  void *data);


/*data is unavailable after this operation*/
void CListRemove(CList *cl, CListCmpElemFn compareFn, void *data);

void CListDestory(CList *cl);

void *CListFirst(CList *cl);

void *CListNext(CList *cl, void *data);

#endif



