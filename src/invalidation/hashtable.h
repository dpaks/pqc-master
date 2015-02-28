#ifndef HASHTABLE_H
#define HASHTABLE_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define HASHSIZE 13     //I don't suppose more than 13 queries would be present in memcached at any time

typedef struct htable_ll_node       //linked list node
{
    int oid;
    char *memkey;
    struct htable_ll_node *next;
}htable_ll_node;

typedef struct htable_node      //hash table entries
{
    htable_ll_node *next;
}htable_node;

typedef struct htable
{
    int tot_size;       //tot_size = HASHSIZE
    htable_node **table;
}htable;

htable *init_htable();
void insert_into_hash(htable *,int , char *);
void hash_insert(htable *, int , char *);
int hash_search(htable *,int , char *);     //0 for no match, 1 for a match
void hash_delete(htable *,int , char *);
void hash_display(htable *);
int hash_fn(int );

#endif
