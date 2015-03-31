#ifndef EXT_INFO_HASH_H
#define EXT_INFO_HASH_H

/* extract the info received from backend */
extern void store_extracted_info(char [], int);

#if TRIGGER_ON_DROP
extern int is_drop_command(char *);      /* ret 1 if it's a drop command */

extern void get_dbname(char [], char []);
#endif // TRIGGER_ON_DROP

#endif
