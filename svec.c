#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "svec.h"

/*
Constructs a new empty svec with an initial maximum size of 4 items
An svec is a vector (array list) of strings
 */
svec*
svec_constructor()
{
    svec* new_svec = malloc(sizeof(svec));
    (*new_svec).size = 0;
    (*new_svec).max = 4;
    (*new_svec).items = malloc((*new_svec).max * sizeof(char*));
    return new_svec;
}

/*
Add the given string to the end of the given svec.
 */
void
add_to_end(svec* list, char* to_add)
{
    // If the current size of the list is greater than or equal to the max size,
    // Then double the max size to accomodate, and copy the items over to the new reallocation
    if ((*list).size >= (*list).max) {
        (*list).max *= 2;
        (*list).items = realloc((*list).items, (*list).max * sizeof(char*));
    }

    // Duplicate to_add and add it to the end of the list
    (*list).items[(*list).size] = strdup(to_add);

    // Increase the current size of the list by 1
    (*list).size += 1;
}

/*
Print each string in the given svec on a new line to stdout
 */
void
print_svec(svec* list)
{
    for (int i = 0; i < (*list).size; ++i) {
        printf("%s", (*list).items[i]);
        printf("\n");
    }
}

/*
Free the memory allocated for the given svec
 */
void
free_svec(svec* list)
{
    // Free each string (character array) in the svec
    for (int i = 0; i < (*list).size; ++i) {
        free((*list).items[i]);
    }

    // Free the array of strings in the svec
    free((*list).items);

    // Free the svec itself
    free(list);
}