#ifndef FILE_OIDS_H
#define FILE_OIDS_H

#include "pool.h"
#include <libmemcached/memcached.h>

extern void add_table_oid(POOL_CONNECTION *, char []);

extern void invalidate_query_cache(POOL_CONNECTION *, char []);

/* gets memcached pointer from pqc.c */
extern memcached_st *get_memc();

extern void write_to_oid_file(char [], char [], char []);

extern int read_from_oid_file(char [], char []);

extern int check_if_meta(char []);

#endif // FILE_OIDS_H
