#ifndef EXT_INFO_HASH_H
#define EXT_INFO_HASH_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <time.h>
#include "pool.h"
#include<error.h>

#define EHASHSIZE 19     //I don't suppose more than 19 queries would be shot by enduser to server at any time

typedef struct e_htable_ll_node       //linked list node
{
    int oid;
    char *memkey;
    struct e_htable_ll_node *next;
}e_htable_ll_node;

typedef struct e_htable_node      //hash table entries
{
    e_htable_ll_node *next;
}e_htable_node;

typedef struct e_htable
{
    int tot_size;       //no of elements present
    e_htable_node **table;
}e_htable;

void init_e_htable(e_htable **);
int store_extracted_info(e_htable **, char [], int, char *);
int e_hash_insert(e_htable **, int , char []);
int e_hash_search(e_htable *,int , int, char *);     //0 for no match, 1 for a match
void e_hash_delete(e_htable *,int , char *);
void e_hash_display(e_htable *);
e_htable **get_ehtable_head();
unsigned long e_hash_fn(unsigned char *);
int *found_in_ehtable(e_htable *, char *);     //return 0 if not found else oid

#endif
