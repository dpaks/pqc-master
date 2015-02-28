
#include "invalidation/ext_info_hash.h"

#define EMAXDATASIZE 100

int store_extracted_info(e_htable **eh, char buf[EMAXDATASIZE], int numbytes)
{
    e_htable *h = *eh;

    pool_debug("\n\nEntered store_extracted_info\n");

    int i, j, query_len = 0;
    time_t cur_time;
    char *query_from_buf = (char *)malloc(numbytes-2-6+1);      //-2-shifting, -6-relid, +1-NULL char
    char oid_from_buf[6];

    //pool_debug("\n\nTESTING0\n");

    //struct extracted_info_nodes *info_node;
    //struct extracted_info_nodes *head = NULL;       //LRU mode
    //pool_debug("\n\nTESTING0.5\n");

    //new_head = head;

    pool_debug("\n\nTESTING1 Address of main head is %u & head is %u\n", (h), (h)->table);

    //printf("\n\nTESTING1: %d from head", (*head)->tableoid_i);

    time(&cur_time);

    for (i = 0; i < 2; i++)                     //shifting buf by two to the left to ignore flag value eg: t;
    {
        for (j = 0; j < numbytes; j++)
        {
            buf[j] = buf[j+1];
        }
        buf[numbytes-i] = '\0';
    }
    numbytes -= 2;
    i = 0;
    j = 0;

    while (buf[i] != ';')
    {
        query_from_buf[i] = buf[i];
        i++;
    }
    query_from_buf[i] = ';';
    query_len = i;
    i++;
    while (buf[i] != '\0')
    {
        oid_from_buf[j++] = buf[i++];
    }
    oid_from_buf[j++] = '\0';

    pool_debug("\n\n\tGOING to HASH INSERT oid_char: %s, oid after atoi: %d, query: %s", oid_from_buf, atoi(oid_from_buf), query_from_buf);
    return e_hash_insert(h, atoi(oid_from_buf) , query_from_buf);
}

e_htable *init_e_htable()
{
    int i=0;

    e_htable *h = (e_htable *)malloc(sizeof(e_htable));
    if(!h) {
        pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
        //puts("Hash table creation failed!!!");
    }

    h->tot_size = 0;
    h->table = (e_htable_node **)malloc((sizeof(e_htable_node *)) * (EHASHSIZE));
    if(!h->table) {
        pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
        //puts("Hash table creation failed!!!");
    }

    for (i=0; i < EHASHSIZE; i++) {
        h->table[i] = (e_htable_node *)malloc((sizeof(e_htable_node)));
        h->table[i]->next = NULL;
    }
    return h;
}

int e_hash_insert(e_htable *h, int oid, char *memkey)
{
    int index;
    index = (e_hash_fn((unsigned char *)memkey))%19;

    if (e_hash_search(h, index, memkey)) {        //returns 1 if data already exists in hash table
        pool_debug("\nDuplicate insertion in EHash Table detected\n");
        return;
    }

    e_htable_ll_node *temp = (e_htable_ll_node *)malloc(sizeof(e_htable_ll_node));
    if(!temp) {
        //pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
    }

    temp->oid = oid;
    temp->memkey = memkey;
    temp->next = h->table[index]->next;        //in chaining, new entries are added at front
    h->table[index]->next = temp;
    ++h->tot_size;
    pool_debug("\n\t Successfully inserted oid: %d & query: %s at index %d in the Hash Table & tot_size is %d\n", h->table[index]->next->oid, h->table[index]->next->memkey,index, h->tot_size);
    return index;
}

void e_hash_delete(e_htable *h, int oid, char *memkey)
{
    int index;
    e_htable_ll_node *temp = NULL, *prev = NULL;

    index = (e_hash_fn((unsigned char *)memkey))%19;

    temp = h->table[index]->next;

    while (temp != NULL){
        if (strcmp(temp->memkey, memkey) == 0) {
            prev = temp->next;
            pool_debug("\n\t Successfully deleted oid: %d & query: %s from index %d\n", h->table[index]->next->oid, h->table[index]->next->memkey, index);
            free(temp);
            break;
        }
        prev = temp;
        temp = temp->next;
    }
}

int e_hash_search(e_htable *h,int index, char *memkey)
{
    e_htable_ll_node *temp = h->table[index]->next;
    while (temp != NULL){
        if (strcmp(temp->memkey, memkey) == 0) {
            printf("\n\t Successfully found oid: %d & query: %s at index %d\n\n", temp->oid, temp->memkey, index);
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

void e_hash_display(e_htable *h)
{
    int i = 0;
    e_htable_ll_node *temp = NULL;

     while (i < EHASHSIZE)
    {
         if (h->table[i]->next != NULL)
        {
             temp = h->table[i]->next;
             while (temp != NULL)
            {
                printf("\t Index %d:- oid: %d memkey: %s \n", i, temp->oid, temp->memkey);
                temp = temp->next;
            }
        }
         i++;
    }
}

unsigned long e_hash_fn(unsigned char *str)                 //djb2 hash function for strings by Dan Bernstein
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int found_in_ehtable(e_htable *h, char *query)
{
    int index;
    index = (e_hash_fn((unsigned char *)query))%19;

    e_htable_ll_node *temp = (h)->table[index]->next;

    while (temp != NULL){
        if (strcmp(temp->memkey, query) == 0) {
            printf("\n\t Successfully found oid: %d & query: %s at index %d\n\n", temp->oid, temp->memkey, index);
            pool_debug("\n\t Successfully found oid: %d & query: %s at index %d\n\n", temp->oid, temp->memkey, index);
            return temp->oid;
        }
        temp = temp->next;
    }
    return 0;
}
