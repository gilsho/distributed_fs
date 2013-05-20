
#ifndef __LIST_H__
#define __LIST_H__

struct node {
	struct node *next;
	struct node *prev;
	size_t sz;
	void *data;
};

struct list {
	struct node *head;
	struct node *tail;
	int count;
};

typedef struct list CList;

typedef int (*CListCmpElemFn)(const void *elemAddr1, 
															const void *elemAddr2);

typedef void (*CListCleanupElemFn)(void *elemAddr);

void CListInit(CList *cl);

void
CListInsertSort(CList *cl, CListCmpElemFn compareFn, 
							  void *data, size_t n);


/*data is unavailable after this operation*/
void CListRemove(CList *cl, void *data);

void CListDestory(CList *cl);

void *CListFirst(CList *cl);

void *CListNext(CList *cl, void *data);

#endif



