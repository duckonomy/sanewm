#ifndef SANEWM_LIST_H
#define SANEWM_LIST_H

struct item
{
	void *data;
	struct item *prev;
	struct item *next;
};

void move_to_head(struct item **, struct item *);
struct item *add_item(struct item **);
void delete_item(struct item **, struct item *);
void free_item(struct item **, int *, struct item *);
void delete_all_items(struct item **, int *);
void list_items(struct item *);

#endif
