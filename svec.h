// svec.h
// Author: Nat Tuck
// 3650F2017, Challenge01 Hints

#ifndef SVEC_H
#define SVEC_H

typedef struct svec {
    int size;
    int cap;
    char** data;
} svec;

svec* make_svec();
void  free_svec(svec* sv);

char* svec_get(svec* sv, int ii);
void  svec_put(svec* sv, int ii, char* item);

void svec_push_back(svec* sv, char* item);

void svec_sort(svec* sv);

void print_svec(svec* sv);

// Gets the subarray of the given svec,
// starting at the begin_idx (inclusive) and ending at the end_idx (exclusive)
svec* get_sub_svec(svec* sv, int begin_idx, int end_idx);

// Finds the first instance of to_find in the svec and returns its index
// If to_find is not in the svec, then return -1
int svec_find(svec* sv, char* to_find);

#endif
