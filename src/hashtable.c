#include "invalidation/hashtable.h"

void insert_into_hash(htable *h,int oid, char *memkey)
{
    hash_insert(h, oid, memkey);
}

htable *init_htable()
{
    int i=0;

    htable *h = (htable *)malloc(sizeof(htable));
    if(!h)
    {
        pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
    }

    h->tot_size = HASHSIZE;
    h->table = (htable_node **)malloc((sizeof(htable_node *)) * (h->tot_size));
    if(!h->table)
    {
        pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
    }

    for (i=0; i < h->tot_size; i++)
    {
        h->table[i] = (htable_node *)malloc((sizeof(htable_node)));
        h->table[i]->next = NULL;
    }
    return h;
}

void hash_insert(htable *h, int oid, char *memkey)
{
    int index;
    index = hash_fn(oid);

    if (hash_search(h, index, memkey))          //returns 1 if data already exists in hash table
    {
        pool_debug("Duplicate insertion detected");
        return;
    }

    htable_ll_node *temp = (htable_ll_node *)malloc(sizeof(htable_ll_node));
    if(!temp)
    {
        pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
    }

    temp->oid = oid;
    temp->memkey = memkey;
    temp->next = h->table[index]->next;        //in chaining, new entries are added at front
    h->table[index]->next = temp;
    pool_debug("\n\t Successfully inserted %d %s at index %d in the Hash Table\n", h->table[index]->next->oid, h->table[index]->next->memkey,index);

}

void hash_delete(htable *h, int oid, char *memkey)
{
    int index;
    htable_ll_node *temp = NULL, *prev = NULL;

    index = hash_fn(oid);

    temp = h->table[index]->next;

    while (temp != NULL)
    {
        if (strcmp(temp->memkey, memkey) == 0)
        {
            prev = temp->next;
            pool_debug("\t Successfully deleted %d %s \n", h->table[index]->next->oid, h->table[index]->next->memkey);
            free(temp);
            break;
        }
        prev = temp;
        temp = temp->next;
    }
}

int hash_search(htable *h,int index, char *memkey)
{
    htable_ll_node *temp = h->table[index]->next;
    while (temp != NULL)
    {
        if (strcmp(temp->memkey, memkey) == 0)
        {
            pool_debug("\n\t Successfully found %d %s at index %d\n\n", temp->oid, temp->memkey, index);
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

void hash_display(htable *h)
{
    int i = 0;
    htable_ll_node *temp = NULL;

    while (i < h->tot_size)
    {
        if (h->table[i]->next != NULL)
        {
            temp = h->table[i]->next;
            while (temp != NULL)
            {
                pool_debug("\t DISPLAYING: Index %d:- oid: %d memkey: %s \n", i, temp->oid, temp->memkey);
                temp = temp->next;
            }
        }
        i++;
    }
}

int hash_fn(int oid)
{
    return oid%13;
}
