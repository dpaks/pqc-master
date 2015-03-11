#include "invalidation/used_list.h"

void insert_into_list(usedlist *l, int oid ,char *tmpkey)
{
    list_insert(l, oid, tmpkey);
}

/***********************************************Initialization****************************************************/

usedlist *init_list()
{
    int i=0;

    usedlist *l = (usedlist *)malloc(sizeof(usedlist));
    if(!l)
    {
        //pool_error("List creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
        puts("List creation failed!!!");
    }

    l->blocks_used = 0;
    l->mylist = (usedlist_node **)malloc((sizeof(usedlist_node)) * (LISTSIZE));
    if(!l->mylist)
    {
        //pool_error("List creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
        puts("List creation failed!!!");
    }

    for (i=0; i < LISTSIZE; i++)
    {
        l->mylist[i] = (usedlist_node *)malloc((sizeof(usedlist_node)));
        l->mylist[i]->next = NULL;
        l->mylist[i]->memid = NULL;
    }
    return l;
}

/***********************************************Insertion****************************************************/

void list_insert(usedlist *l, int oid, char *memid)
{
    int i = -1,ret_val = 0, ind = 0;

    if ((ret_val = list_search(l, oid, memid, &ind)) == 1)          //returns 1 if data already exists in hash table
    {
        pool_debug("\tDUPLICATE insertion in list memid: %s and oid: %d into list at index: %d\n", memid, oid, ind);
        return;
    }
    else if (ret_val == 2)
    {
        //pool_error("List overflow!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
        puts("List overflow!!!");
        exit(0);
    }

    /*l->mylist[i]->memid = malloc(sizeof(char) * (strlen(memid)+1));
    if(!(l->mylist[i]->memid))
    {
        //pool_error("List creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
    }*/
    usedlist_ll *temp = (usedlist_ll *)malloc(sizeof(usedlist_ll));
    if(!temp)
    {
        //pool_error("List creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
        //return 0;
    }
    temp->oid = oid;
    temp->next = NULL;
    if (ind != 0)                               //already memid present in list
    {
        pool_debug("\tDUPLICATE memid: %s exists in list at index: %d but oid: %d absent\n", l->mylist[ind]->memid, ind, oid);
        i = ind;
        temp->next = l->mylist[i]->next;        //insertion at front
    }
    else
    {
        while (l->mylist[++i] != NULL);
    }
    l->mylist[i]->next = temp;
    strcpy(l->mylist[i]->memid, memid);         //already allocated space in init()
    pool_debug("\tINSERTED memid: %s and oid: %d into list at index: %d\n", l->mylist[i]->memid, l->mylist[i]->next->oid, i);
}

/***********************************************Searching****************************************************/

int list_search(usedlist *l, int oid, char *memid, int *ind)
{
    int i = 0;
    while ((l->mylist[i] != NULL) && (i < LISTSIZE))
    {
        if (strcmp(l->mylist[i]->memid, memid) == 0)
        {
            *ind = i;
            usedlist *temp = l->mylist[i]->next;
            while (temp != NULL)
            {
                if (temp->oid == oid)
                {
                    return 1;
                }
                temp = temp->next;
            }
            return 0;
        }
        i++;
    }
    if (i == LISTSIZE)
    {
        return 2;
    }
}

/***********************************************Displaying****************************************************/

void list_display(usedlist *l)
{
    int i = 0;
    while ((l->mylist[i] != NULL) && (i < LISTSIZE))
    {
        pool_debug("\tDISPLAYING memid: %s, oid: %d in Used List at index: %d\n", l->mylist[i]->memid, l->mylist[i]->next->oid, i);
        i++;
    }

}





