#ifndef FILE_OIDS_H
#define FILE_OIDS_H

#include "pool.h"
#include <libmemcached/memcached.h>

void add_table_oid(POOL_CONNECTION *, char []);
memcached_st *get_memc();

#endif // FILE_OIDS_H
