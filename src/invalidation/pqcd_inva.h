#ifndef PQCD_INVA_H
#define PQCD_INVA_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void store_extracted_info(char [], int);

struct extracted_info_nodes *initialize_ll(struct extracted_info_nodes *, int);

struct extracted_info_nodes
{
    char *query_i;
    int tableoid_i;
    time_t time_of_entry;
    struct extracted_info_nodes *next, *prev;
};

#endif // PQCD_INVA_H
