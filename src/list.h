#ifndef SANEWM_LIST_H
#define SANEWM_LIST_H

struct list_item
{
	void *data;
	struct list_item *prev;
	struct list_item *next;
};

void move_to_head(struct list_item **, struct list_item *);
struct list_item *add_item(struct list_item **);
void delete_item(struct list_item **, struct list_item *);
void free_item(struct list_item **, int *, struct list_item *);
void delete_all_items(struct list_item **, int *);
void list_items(struct list_item *);

#endif
