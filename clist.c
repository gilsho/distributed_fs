
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stddef.h>
#include "clist.h"

struct node *
node_from_data(void * data)
{
	return (struct node *) 
					((char *) data) - offsetof(struct node, data);
}

void 
CListInit(CList *cl)
{
	cl = malloc(sizeof(struct list));
	cl->count = 0;
	cl->head = NULL;
}

void
CListInsertSort(CList *cl, CListCmpElemFn compareFn, 
							  void *data, size_t n) 
{
	struct node *nd = malloc(sizeof(struct node));
	memcpy(&nd->data,data,n);
	nd->sz = n;

	/* empy list */
	if (cl->head == NULL) {
		cl->head = nd;
		cl->tail = nd;
		return;
	}

	/* insert at middle */
	for (struct node *cur=cl->head; cl != NULL; cur=cur->next) {
		if (compareFn(nd,cur) > 0) {
			nd->next = cur;
			nd->prev = cur->prev;
			nd->next->prev = nd;
			if (nd->prev)				/* insert at head */
				nd->prev->next = nd;
			else
				cl->head = nd;
			return;
		}
	}

	/* insert at tail */
	nd->prev = cl->tail;
	nd->next = NULL;
	nd->prev->next = cl->tail;
	cl->tail = nd;
}



/*data is unavailable after this operation*/
void CListRemove(CList *cl, void *data)
{
	struct node *nd = node_from_data(data);
	if (nd->prev)
		nd->prev->next = nd->next;
	else
		cl->head = nd->next;

	if (nd->next)
		nd->next->prev = nd->prev;
	else
		cl->tail = nd->prev;

	free(nd);
}

void CListDestory(CList *cl)
{
	for (struct node *cur=cl->head; cur != NULL; ) {
		struct node *old = cur;
		cur=cur->next;
		free(old);
	}
	free(cl);
}


void *CListFirst(CList *cl)
{
	if (!cl->head) return NULL;
	return cl->head->data;
}

void *CListNext(CList *cl, void *data)
{
	if (!data) return NULL;
	struct node *nd = node_from_data(data);
	if (!nd->next)	return NULL;
	return nd->next->data;
}
