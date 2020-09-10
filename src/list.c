#include <stdio.h>
#include <stdlib.h>

#include "list.h"

void
move_to_head(struct list_item **main_list, struct list_item *item)
{
	// Move element in item to the head of list main_list.
	if (NULL == item || NULL == main_list || NULL == *main_list)
		return;
	/* item is NULL or we're already at head. Do nothing. */
	if (*main_list == item)
		return;
	/* Braid together the list where we are now. */
	if (NULL != item->prev)
		item->prev->next = item->next;

	if (NULL != item->next)
		item->next->prev = item->prev;
	/* Now we'at head, so no one before us. */
	item->prev = NULL;
	/* Old head is our next. */
	item->next = *main_list;
	/* Old head needs to know about us. */
	item->next->prev = item;
	/* Remember the new head. */
	*main_list = item;
}

struct list_item *
add_item(struct list_item **main_list)
{                                   // Create space for a new item and add it to the head of main_list.
				    // Returns item or NULL if out of memory.
	struct list_item *item;

	if (NULL == (item = (struct list_item *)
		     malloc(sizeof (struct list_item))))
		return NULL;
	/* First in the list. */
	if (NULL == *main_list)
		item->prev = item->next = NULL;
	else {
		/* Add to beginning of list. */
		item->next = *main_list;
		item->next->prev = item;
		item->prev = NULL;
	}
	*main_list = item;
	return item;
}

void
delete_item(struct list_item **main_list, struct list_item *item)
{
	struct list_item *ml = *main_list;

	if (NULL == main_list ||
	    NULL == *main_list ||
	    NULL == item)
		return;
	/* First entry was removed. Remember the next one instead. */
	if (item == *main_list) {
		*main_list = ml->next;
		if (item->next!=NULL)
			item->next->prev = NULL;
	}
	else {
		item->prev->next = item->next;
		/* This is not the last item in the list. */
		if (NULL != item->next)
			item->next->prev = item->prev;
	}
	free(item);
}

void
free_item(struct list_item **list, int *stored,struct list_item *item)
{
	if (NULL == list || NULL == *list || NULL == item)
		return;

	if (NULL != item->data) {
		free(item->data);
		item->data = NULL;
	}
	delete_item(list, item);

	if (NULL != stored)
		--(*stored);
}


void
delete_all_items(struct list_item **list, int *stored)
{                                   // Delete all elements in list and free memory resources.
	struct list_item *item;
	struct list_item *next;

	for (item = *list; item != NULL; item = next) {
		next = item->next;
		free(item->data);
		delete_item(list, item);
	}

	if (NULL != stored)
		(*stored) = 0;
}

void
list_items(struct list_item *main_list)
{
	struct list_item *item;
	int i;
	for (item = main_list, i = 1; item != NULL; item = item->next, ++i)
		printf("item #%d (stored at %p).\n", i, (void *)item);
}
