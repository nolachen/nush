// Define a svec struct (similar to arraylist of strings)
// size - the current size of the svec
// max - the maximum capacity of the svec
// items - the strings in the svec
typedef struct svec {
    int size;
    int max;
    char** items;
} svec;

svec* svec_constructor();

void add_to_end(svec* list, char* to_add);

void print_svec(svec* list);

void free_svec(svec* list);