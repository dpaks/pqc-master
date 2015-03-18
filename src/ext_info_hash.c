#include <ctype.h>

#include "invalidation/ext_info_hash.h"
#include "invalidation/md5.h"
#include "invalidation/main_headers.h"
#include "pool.h"

static void write_meta(char *);

/* Extracting and storing the info received */
void store_extracted_info(char buf[BUFSIZE], int numbytes)
{
    int i, j;                   /* -2-shifting, -6-relid, +1-NULL char */
    char *query_from_buf = malloc(MAXDATASIZE * sizeof(char));
    char oid_from_buf[OIDLENGTH], dbname_from_buf[DBLENGTH], to_hash[MAXDATASIZE];

    char *checksum = malloc(sizeof(char) *MD5KEYSIZE);
    int fd, oid_size;
    char path[PATHLENGTH];
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (buf[0] == 't')
    {
        snprintf(path, sizeof(path), "%s/%s", dir, "ext_info");
    }
    else
    {
        snprintf(path, sizeof(path), "%s/%s", dir, "ext_info_inva");
    }
    if ((fd = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open file in ext_info_hash ");
        exit(EXIT_FAILURE);
    }

    /* shifting buf by two to the left to ignore flag value */
    for (i = 0; i < 2; i++)
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
        dbname_from_buf[i] = buf[i];        /* extracting db name */
        i++;
    }
    dbname_from_buf[i] = '\0';
    ++i;

    while (buf[i] != ';')
    {
        query_from_buf[j] = buf[i];         /* extracting query */
        i++;
        j++;
    }
    query_from_buf[j] = ';';
    query_from_buf[++j] = '\0';

    ++i;
    j=0;

#if TRIGGER_ON_DROP
    if (is_drop_command(query_from_buf))
    {
        char dbname[DBLENGTH];
        get_dbname(dbname, query_from_buf);
        snprintf(path, sizeof(path), "rm -rf %s/%s/%s", dir, "oiddir", dbname);
        system(path);
        return;
    }
#endif // TRIGGER_ON_DROP

    pool_debug("\tEXTRACTED QUERY: %s from DATABASE '%s' of size: %d from BUF",
               query_from_buf, dbname_from_buf, strlen(query_from_buf));

    strcpy(to_hash, dbname_from_buf);
    strncat(to_hash, query_from_buf, (MAXDATASIZE-strlen(dbname_from_buf)-1));
    pg_md5_hash(to_hash, strlen(to_hash), checksum);

    if (fcntl(fd, F_SETLKW, &fl) == -1)     /* locking ext info file */
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    else
    {
        pool_debug("\tFILE locked in ext_info_hash!\n");
    }

    if (write(fd, checksum, strlen(checksum)) == -1)
    {
        perror("\tWrite error for Checksum! ");
        exit(EXIT_FAILURE);
    }

    pool_debug("\tWROTE checksum\n");

    while (buf[i] != '\0')
    {
        oid_size = (OIDLENGTH-1);
        while (oid_size > 0)
        {
            oid_from_buf[j++] = buf[i++];
            oid_size--;
        }
        oid_from_buf[j] = '\0';

        if (atoi(oid_from_buf) < 10000)     /* meta commands are taken care of */
        {
            write_meta(checksum);
            break;
        }

        j = 0;
        pool_debug("\tEXTRACTED OID FROM BUF: %s of size: %d", oid_from_buf,
                   strlen(oid_from_buf));

        if ((write(fd, " ", 1) == -1) || (write(fd, oid_from_buf, (OIDLENGTH-1)) == -1))
        {
            perror("\tWrite error for OID! ");
        }
        else
        {
            pool_debug("\tWROTE oid: %s\n", oid_from_buf);
        }
    }

    if (write(fd, "\n", 1) == -1)
    {
        perror("\tWriting newline char in ext_info_hash ");
    }

    fl.l_type = F_UNLCK;                 /* Unlocking the file */
    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        perror("\tUnlocking in ext_info_hash ");
        close(fd);
        exit(EXIT_FAILURE);
    }
    else
    {
        pool_debug("\tFILE unlocked in ext_info_hash!\n");
    }

    free(checksum);
    free(query_from_buf);
    close(fd);
}

#if TRIGGER_ON_DROP
/*
 *Checking if it is a select query
 */
int is_drop_command(char *query)
{
    if (*query == '\0')
        return 0;

    /* skip spaces */
    while (*query && isspace(*query))
        query++;

    /* DROP? */
    if (strncasecmp("DROP", query, 4))	/* returns 0 if not a match */
        return 0;

    return 1;				/* returns 1 if it is a drop command */
}

/*
 * Extract db name from DROP command
 */
void get_dbname(char dbname[DBLENGTH], char query_from_buf[MAXDATASIZE])
{
    int i = 14, j = 0;
    while (buf[i] != ';')
    {
        dbname[j] = buf[i];         /* extracting dbname */
        i++;
        j++;
    }
    dbname[j] = '\0';
}
#endif // TRIGGER_ON_DROP

void write_meta(char *checksum)
{
    pool_debug("\tMETA command recvd in ext_info_hash\n");

    int fd_new;
    struct flock fl_new;
    char path[512];

    fl_new.l_type = F_WRLCK;
    fl_new.l_whence = SEEK_SET;
    fl_new.l_start = 0;
    fl_new.l_len = 0;

    snprintf(path, sizeof(path), "%s/%s", dir, "ext_info_meta");
    if ((fd_new = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open meta file in ext_info_hash ");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd_new, F_SETLKW, &fl_new) == -1)     /* locking ext info file */
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    else
    {
        pool_debug("\tFILE meta locked in ext_info_hash!\n");
    }

    if ( (write(fd_new, checksum, strlen(checksum)) == -1) || (write(fd_new, "\n", 1) == -1) )
    {
        perror("\tWrite error for Checksum in meta in ext_info_hash! ");
        exit(EXIT_FAILURE);
    }

    fl_new.l_type = F_UNLCK;                 /* Unlocking the file */
    if (fcntl(fd_new, F_SETLK, &fl_new) == -1)
    {
        perror("\tUnlocking meta file in ext_info_hash ");
        close(fd_new);
        exit(EXIT_FAILURE);
    }
    else
    {
        pool_debug("\tFILE meta unlocked in ext_info_hash!\n");
    }

    close(fd_new);
}

