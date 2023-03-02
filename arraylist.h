#ifndef _ARRAYLIST_H
#define _ARRAYLIST_H

typedef struct {
	unsigned size;
	unsigned capacity;
	char **data;
} list_t;

int  al_init(list_t *list, unsigned capacity);
void al_destroy(list_t *list, unsigned capacity);

unsigned al_length(list_t *list);
int al_insert(list_t *list, unsigned index, char* src);

int al_push(list_t *list, char* str);
int al_pop(char **dest, list_t *list);

#endif
