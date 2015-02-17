
#include "invalidation/pqcd_inva.h"

#define MAXDATASIZE 100

/*********************************Extracting and storing info in a CDLL*************************************************/

void store_extracted_info(char buf[MAXDATASIZE], int numbytes)
{
    int i, j, query_len = 0;
    time_t cur_time;
    char *query_from_buf = (char *)malloc(numbytes-2-6+1);
    char oid_from_buf[6];   //-2-shifting, -6-relid, +1-NULL char

    struct extracted_info_nodes *info_node = (struct extracted_info_nodes *)malloc(sizeof(struct extracted_info_nodes *));
    struct extracted_info_nodes *head = NULL;       //LRU mode

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
        query_from_buf[i] = buf[i++];
    }
    query_from_buf[i] = ';';
    query_len = i;

    while (buf[i] != '\0')
    {
        oid_from_buf[j++] = buf[i++];
    }
    oid_from_buf[j++] = '\0';

    info_node = initialize_ll(info_node, query_len);

    if (head == NULL)
    {
        info_node->time_of_entry = cur_time;
        strcpy(info_node->query_i, query_from_buf);
        info_node->tableoid_i = atoi(oid_from_buf);
        head = info_node;
        head->next = head;
        head->prev = head;
        //get_head(head);

    }
    else
    {
        info_node->time_of_entry = cur_time;
        strcpy(info_node->query_i, query_from_buf);
        info_node->tableoid_i = atoi(oid_from_buf);
        info_node->prev = head->prev;
        info_node->prev->next = info_node;
        info_node->next = head;
        head->prev = info_node;
        if(head->next == head)
        {
            head->next = info_node;
        }
    }
}

/****************************************************Initializing the CDLL***************************************************/

struct extracted_info_nodes *initialize_ll(struct extracted_info_nodes *info_node, int query_len)
{
    info_node->query_i = (char *)malloc(sizeof(char *)*query_len);
    info_node->next = NULL;
    info_node->prev = NULL;
    return info_node;
}

/*********************************Fetching from CDLL and populating Used Tables and Tables Hash************************************/


