// svec.c
// Author: Nat Tuck
// 3650F2017, Challenge01 Hints

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "svec.h"

svec*
make_svec()
{
    svec* sv = malloc(sizeof(svec));
    sv->size = 0;
    sv->cap  = 4;
    sv->data = malloc(4 * sizeof(char*));
    memset(sv->data, 0, 4 * sizeof(char*));
    return sv;
}

void
free_svec(svec* sv)
{
    for (int ii = 0; ii < sv->size; ++ii) {
        if (sv->data[ii] != 0) {
            free(sv->data[ii]);
        }
    }
    free(sv->data);
    free(sv);
}

char*
svec_get(svec* sv, int ii)
{
    assert(ii >= 0 && ii < sv->size);
    return sv->data[ii];
}

void
svec_put(svec* sv, int ii, char* item)
{
    assert(ii >= 0 && ii < sv->size);
    sv->data[ii] = strdup(item);

}

void svec_push_back(svec* sv, char* item)
{
    int ii = sv->size;

    if (ii >= sv->cap) {
        sv->cap *= 2;
        sv->data = (char**) realloc(sv->data, sv->cap * sizeof(char*));
    }

    sv->size = ii + 1;
    svec_put(sv, ii, item);
}

void
print_svec(svec* sv) {
  for (int i = 0; i < sv->size; ++i) {
    printf("%s\n", sv->data[i]);
  }
}

svec*
get_sub_svec(svec* sv, int begin_idx, int end_idx) {
     printf("The svec to get sub of : ");
     print_svec(sv);
     printf("begin: %i\n", begin_idx);
     printf("end: %i\n", end_idx);
  //perror("help");

  assert(begin_idx < end_idx && begin_idx >= 0 && end_idx <= sv->size);
  svec* sub_svec = make_svec();
  for (int i = begin_idx; i < end_idx; ++i) {
    svec_push_back(sub_svec, svec_get(sv, i));
  }
  return sub_svec;
}

int
svec_find(svec* sv, char* to_find) {
  for (int i = 0; i < sv->size; ++i) {
    if (strcmp(sv->data[i], to_find) == 0) {
      return i;
    }
  }

  // to_find was not found
  return -1;
}
