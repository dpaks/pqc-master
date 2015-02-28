
#include "invalidation/pqcd_inva.h"
#include "pool.h"



/***********************************************Populate invalidation structures****************************************************/

//void populate_inva_strucs(htable *h, usedlist *l, int oid, char *tempkey)
void populate_inva_strucs(htable *h, int oid, char *tempkey)
{
    insert_into_hash(h, oid, tempkey);
    //insert_into_list(l, oid, tempkey);
}

