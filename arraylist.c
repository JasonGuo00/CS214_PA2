#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "arraylist.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#ifdef SAFE
#define SAFETY_CHECK \
if (list->data == NULL) { \
	fputs("Use of unallocated or destroyed arraylist\n", stderr); \
	exit(EXIT_FAILURE); \
}
#else
#define SAFETY_CHECK
#endif

/* Initialize an empty array list with the specified capacity.
 *
 * Returns 1 on success, or 0 if unable to allocate storage
 *
 * capacity must be greater than 0
 */
int al_init(list_t *list, unsigned capacity)
{
	if (DEBUG > 1) fprintf(stderr, "Initializing %p with %u\n", list, capacity);
	assert(capacity > 0);
	list->capacity = capacity;
	list->size = 0;
	list->data = malloc(sizeof(char*) * capacity);
	
	return list->data != NULL;
}

void al_destroy(list_t *list)
{
	if (DEBUG > 1) fprintf(stderr, "Destroying %p\n", list);
	SAFETY_CHECK
	for(int i = 0; i < list->size; i++) {
		free(list->data[i]);
	}
	free(list->data);

#ifdef SAFE
	list->data = NULL;
#endif
}

unsigned al_length(list_t *list)
{
	return list->size;
}


/* Writes the given integer to the specified index in the list
 *
 * Returns 1 on success, or 0 if the index is out of range.
 * Does not extend the length of the list.
 */

int al_insert(list_t *list, unsigned index, char* str)
{
	SAFETY_CHECK
	
	if (index >= list->size) return 0;
	
	list->data[index] = str;
	
	return 1;
}

/* Appends integer to the end of the list
 *
 * Returns 1 on success, or 0 if memory allocation failed.
 */
int al_push(list_t *list, char* str)
{
	if (DEBUG > 1) fprintf(stderr, "push %p: %s\n", list, str);
	SAFETY_CHECK

	if (list->size == list->capacity) {
		int newcap = list->capacity * 2;
		char **new = realloc(list->data, sizeof(char*) * newcap);
		if (DEBUG) fprintf(stderr, "increase capacity of %p to %d\n", list, newcap);
		if (!new) return 0;

		// NOTE no changes are made to the list until we know allocation succeded
		list->data = new;
		list->capacity = newcap;
	}
	
	list->data[list->size] = str;
	++list->size;
	
	return 1;
}


/* Copies last entry in list to destination and reduces size of list by 1.
 *
 * Returns 1 on success, or 0 if the list is empty.
 */
int al_pop(char **dest, list_t *list)
{
	if (DEBUG > 1) fprintf(stderr, "pop %p\n", list);
	SAFETY_CHECK
	
	if (list->size == 0) return 0;
	
	--list->size;
	*dest = list->data[list->size];
	
	return 1;
}
