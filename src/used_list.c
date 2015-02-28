
#include "invalidation/used_list.h"

void insert_into_list(usedlist *l, int oid ,char *tmpkey)
{
    list_insert(l, oid, tmpkey);
}

usedlist *init_list()
{
    int i=0;

    usedlist *l = (usedlist *)malloc(sizeof(usedlist));
    if(!l) {
        //pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
        puts("List creation failed!!!");
    }

    l->blocks_used = 0;
    l->mylist = (usedlist_node **)malloc((sizeof(usedlist_node *)) * (LISTSIZE));
    if(!l->mylist) {
        //pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
        puts("List creation failed!!!");
    }

    for (i=0; i < LISTSIZE; i++) {
        l->mylist[i] = (usedlist_node *)malloc((sizeof(usedlist_node)));
        l->mylist[i]->next = NULL;
        l->mylist[i]->memid = NULL;
    }
    return l;
}

void list_insert(usedlist *l, int oid, char *memid)
{
    int i = 0;
    while (l->mylist[i] != NULL)
    {
        i++;
        if (i > LISTSIZE)
        {
            //pool_error("List overflow!!!");        //in the calling function make sure that you do treat the returned '0'
            //return 0;
            puts("List overflow!!!");
        }
    }

    strcpy(l->mylist[i]->memid, memid);

    usedlist_ll *temp = (usedlist_ll *)malloc(sizeof(usedlist_ll));
    if(!temp) {
        //pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
    }

    temp->oid = oid;
    temp->memkey = memkey;
    temp->next = h->table[index]->next;        //in chaining, new entries are added at front
    h->table[index]->next = temp;
    printf("\t Successfully inserted %d %s at index %d \n", h->table[index]->next->oid, h->table[index]->next->memkey,index);

}
