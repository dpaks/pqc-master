#ifndef PQCD_INVA_H
#define PQCD_INVA_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "invalidation/hashtable.h"
//#include "invalidation/ext_info_hash.h"
//#include "invalidation/used_list.h"

/*typedef struct extracted_info_nodes
{
    char *query_i;
    int tableoid_i;
    time_t time_of_entry;
    struct extracted_info_nodes *next, *prev;
}extracted_info_nodes;

typedef struct extracted_info_nodes_ll
{
    int count_of_nodes;
    struct extracted_info_nodes *head;
}extracted_info_nodes_ll;

extracted_info_nodes_ll **get_ll_head();*/

//void store_extracted_info(extracted_info_nodes_ll **, char [], int);
//void store_extracted_info(e_htable **, char [], int);


//struct extracted_info_nodes *initialize_ll(struct extracted_info_nodes *, int);

//int *found_in_ll(extracted_info_nodes_ll **, char *);     //return 0 if not found else oid

//void populate_inva_strucs(htable *, usedlist *, int , char *);
void *send_to_mmap(char *, int);

char *get_from_mmap(int *);

void populate_inva_strucs(htable *, int , char *);

#endif // PQCD_INVA_H
