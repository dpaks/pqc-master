#include "invalidation/ext_info_hash.h"
#include "invalidation/md5.h"
#include "invalidation/main_headers.h"

#define EMAXDATASIZE 100
#define MAX_KEY 256

/***********************************************Extracting and Storing****************************************************/

int store_extracted_info(e_htable **eh, char buf[EMAXDATASIZE], int numbytes, char *prev_query)
{
    //e_htable *h = *eh;
    int i, j;
    time_t cur_time;
    char *query_from_buf = (char *)malloc((numbytes-2-5) * sizeof(char));      //-2-shifting, -6-relid, +1-NULL char
    char oid_from_buf[6];
    time(&cur_time);

    char *dir = "/tmp/mypqcd/oiddir";
    char path[100];
    char *checksum = malloc(sizeof(char) *MAX_KEY);
    int fd, k=0;
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    //fl.l_pid = 0;

    snprintf(path, sizeof(path), "%s/%s", dir, "ext_info");
    if ((fd = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open file in ext_info_hash ");
        exit(1);
    }
    fchmod(fd, S_IRWXU|S_IRWXO|S_IRWXG);

    pool_debug("Numbytes in store_extracted_info is %d", numbytes);
    for (i = 0; i < 2; i++)                     //shifting buf by two to the left to ignore flag value eg: t;
    {
        for (j = 0; j < numbytes; j++)
        {
            buf[j] = buf[j+1];
        }
        --numbytes;
    }

    i = 0;
    j = 0;

    while (buf[i] != ';')
    {
        query_from_buf[i] = buf[i];
        i++;
    }
    query_from_buf[i] = ';';
    query_from_buf[++i] = '\0';
    pool_debug("\tEXTRACTED QUERY FROM BUF: %s of size: %d", query_from_buf, strlen(query_from_buf));

    if (strcmp(prev_query, " ") == 0)
    {
        strcpy(prev_query, query_from_buf);
    }

    if (strcmp(query_from_buf, prev_query) != 0)                //writing to file
    {
        char char_oid[6];
        int *arr_of_oid = found_in_ehtable(*eh, prev_query);
        pool_debug("\tPrev_query before MD5 is %s", prev_query);

        pg_md5_hash(prev_query, strlen(prev_query), checksum);

        if (fcntl(fd, F_SETLKW, &fl) == -1)     //locking ext info file
        {
            perror("fcntl");
            exit(1);
        }
        else
        {
            pool_debug("\tFILE locked in ext_info_hash!\n");
        }

        if (write(fd, checksum, strlen(checksum)) == -1)
        {
            perror("\tWrite error for Checksum! ");
        }
        //len_arr_of_oid = sizeof(arr_of_oid)/arr_of_oid[0];
        //while (len_arr_of_oid > 0)
        pool_debug("\tWROTE checksum\n");
        while (arr_of_oid[k] != -1)             //array terminator is -1
        {
            snprintf(char_oid, 6, "%d", arr_of_oid[k]);
            if ((write(fd, " ", 1) == -1) | (write(fd, char_oid, 5) == -1))
            {
                perror("\tWrite error for OID! ");
            }
            else
            {
                pool_debug("\tWROTE oid: %s\n", char_oid);
            }
            k++;
        }
        if (write(fd, "\n", 1) == -1)
            perror("\tWriting newline char in ext_info_hash ");

        fl.l_type = F_UNLCK;        //Unlocking the file
        if (fcntl(fd, F_SETLK, &fl) == -1)
        {
            perror("\tUnlocking in ext_info_hash ");
            close(fd);
            exit(1);
        }
        else
        {
            pool_debug("\tFILE unlocked in ext_info_hash!\n");
        }
    }
    close(fd);
    strcpy(prev_query, query_from_buf);     //can be put inside the above if loop

    while (buf[i] != '\0')
    {
        oid_from_buf[j++] = buf[i++];
    }
    oid_from_buf[j] = '\0';
    pool_debug("\tEXTRACTED OID FROM BUF: %s of size: %d", oid_from_buf, strlen(oid_from_buf));

    pool_debug("\tGOING to HASH INSERT oid: %s, query: %s\n", oid_from_buf, query_from_buf);
    return e_hash_insert(eh, atoi(oid_from_buf) , query_from_buf);
}

/***********************************************Initializing Hash Table****************************************************/

void init_e_htable(e_htable **h)
{
    int i=0;
    *h = (e_htable *)malloc(sizeof(e_htable));
    if(!(*h))
    {
        pool_error("Hash table creation failed!!!\n");        //in the calling function make sure that you do treat the returned '0'
    }

    (*h)->tot_size = 0;
    (*h)->table = (e_htable_node **)malloc((sizeof(e_htable_node)) * (EHASHSIZE));
    if(!(*h)->table)
    {
        pool_error("Hash table creation failed!!!\n");        //in the calling function make sure that you do treat the returned '0'
    }

    for (i=0; i < EHASHSIZE; i++)
    {
        (*h)->table[i] = (e_htable_node *)malloc((sizeof(e_htable_node)));
        (*h)->table[i]->next = NULL;
    }
}

/***********************************************Insertion****************************************************/

int e_hash_insert(e_htable **h, int oid, char memkey[EMAXDATASIZE])       //indexing done on memkey
{
    int index;
    index = (e_hash_fn((unsigned char *)memkey))%19;
    pool_debug("\tMEMKEY THAT IS TO BE INSERTED: %s", memkey);
    if (e_hash_search((*h), index, oid, memkey))          //returns 1 if data already exists in hash table
    {
        pool_debug("\tDuplicate insertion in EHash Table detected\n");
        exit(1);
    }

    e_htable_ll_node *temp = (e_htable_ll_node *)malloc(sizeof(e_htable_ll_node));
    if(!temp)
    {
        pool_error("Hash table creation failed!!!");        //in the calling function make sure that you do treat the returned '0'
    }

    temp->oid = oid;
    temp->memkey = malloc((strlen(memkey)+1) * sizeof(char));
    strcpy(temp->memkey, memkey);
    temp->next = (*h)->table[index]->next;        //in chaining, new entries are added at front
    (*h)->table[index]->next = temp;
    ++((*h)->tot_size);
    pool_debug("\tSuccessfully inserted oid: %d & query: %s at index %d in the Hash Table & tot_size is %d\n", (*h)->table[index]->next->oid, (*h)->table[index]->next->memkey,index, (*h)->tot_size);
    return index;
}

/***********************************************Deletion****************************************************/

void e_hash_delete(e_htable *h, int oid, char *memkey)
{
    int index;
    e_htable_ll_node *temp = NULL, *prev = NULL;

    index = (e_hash_fn((unsigned char *)memkey))%19;

    temp = h->table[index]->next;

    while (temp != NULL)
    {
        if (strcmp(temp->memkey, memkey) == 0)
        {
            prev = temp->next;
            pool_debug("\tSuccessfully deleted oid: %d & query: %s from index %d\n", h->table[index]->next->oid, h->table[index]->next->memkey, index);
            free(temp);
            temp = NULL;
            break;
        }
        prev = temp;
        temp = temp->next;
    }
}

/***********************************************Searching****************************************************/

int e_hash_search(e_htable *h,int index, int oid, char *memkey)
{
    e_htable_ll_node *temp = h->table[index]->next;
    while (temp != NULL)
    {
        if ((temp->oid == oid) && (strcmp(temp->memkey,memkey) == 0))
        {
            pool_debug("\tSuccessfully found oid: %d & query: %s at index %d\n", temp->oid, temp->memkey, index);
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

/***********************************************Displaying****************************************************/

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
                pool_debug("\tDISPLAYING: Index %d:- oid: %d memkey: %s at addr %u\n", i, temp->oid, temp->memkey,temp);
                temp = temp->next;
            }
        }
        i++;
    }
}

/***********************************************HASH Function****************************************************/

unsigned long e_hash_fn(unsigned char *str)                 //djb2 hash function for strings by Dan Bernstein
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/***********************************************Retrieving and Deleting****************************************************/

int *found_in_ehtable(e_htable *h, char *query)
{
    int index, i=0;
    int *arr_of_oid = malloc(sizeof(int) * 10);         //assuming a query refers a max of 10 tables only
    if (!arr_of_oid)
    {
        perror("\tarr_of_oid creation failed! ");
    }
    index = (e_hash_fn((unsigned char *)query))%19;

    e_htable_ll_node *temp = (h)->table[index]->next;

    while (temp != NULL)
    {
        if (strcmp(temp->memkey, query) == 0)
        {
            pool_debug("\tSuccessfully found oid: %d & query: %s at index %d\n", temp->oid, temp->memkey, index);
            arr_of_oid[i++] = temp->oid;
            //e_hash_delete(h, temp->oid, temp->memkey);
        }
        temp = temp->next;
    }
    arr_of_oid[i] = -1;
    if (i>0)
    {
        return arr_of_oid;
    }
    return 0;
}
