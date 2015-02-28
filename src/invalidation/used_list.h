#ifndef USED_LIST_H
#define USED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LISTSIZE 13     //I don't suppose more than 13 queries would be present in memcached at any time

typedef struct usedlist_ll
{
    int oid[5];             //assuming that a query references a maximum of 5 tables
    struct usedlist_ll *next;
}usedlist_ll;

typedef struct usedlist_node
{
    char *memid;
    struct usedlist_ll *next;
}usedlist_node;

typedef struct usedlist
{
    int blocks_used;
    struct usedlist_node **mylist;
}usedlist;

usedlist *init_list();
void insert_into_list(usedlist *,int ,char *);
void list_insert(usedlist *, int ,char *);

#endif // USED_LIST_H
