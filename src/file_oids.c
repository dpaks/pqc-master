    /*
 * FILE: file_oids.c
 * HEADER: invalidation/file_oids.h
 *
 * File I/O is done here. For each received query at pqcd, a match is checked
 * in the corresponding file where the extracted info is stored. Matching MD5
 * checksums of queries are written to necessary OID file(s) for cacheable
 * queries and matching MD5 checksums of invalidateable queries are read from
 * proper OID file(s) for invalidation.
 *
 * Written by Deepak S
 *
 * Copyright (c) 2015-Today	Deepak S (in.live.in@live.in)
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of the
 * author not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The author makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 */

#include <libmemcached/memcached.h>
#include <time.h>
#include <ctype.h>

#include "invalidation/main_headers.h"
#include "invalidation/file_oids.h"
#include "invalidation/md5.h"

/*
 * OFFSET_VAL: The offset relative to the end of file from where the
 *             checksum comparison has to begin in files
 */

#define OFFSET_VAL -150

static char *skip_comment_space(char *);

/*
 * To write query checksum to its dependent table oid(s) file(s)
 */
void add_table_oid(POOL_CONNECTION *frontend, char tmpkey[MD5KEYSIZE])
{
    char db_name[DBLENGTH], path[PATHLENGTH];

    if (mkdir(dir, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tCREATION of directory1 failed in add_table_oid ");
            exit(EXIT_FAILURE);
        }
    }

    snprintf(path, sizeof(path), "%s/%s", dir, "oiddir");

    if (mkdir(path, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {

            perror("\tCREATION of directory2 failed in add_table_oid ");
        }
        else
        {
            perror("\tDIRECTORY already exists! ");
        }
    }

    strcpy(db_name, frontend->database);

    snprintf(path, sizeof(path), "%s/%s/%s", dir, "oiddir", db_name);
    /* Create database_oid folder */
    if (mkdir(path, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tDB directory\n");
        }
    }

    int read_bytes, fd_old;
    char tmpkey_from_file[MD5KEYSIZE], oid_from_file[OIDLENGTH], c;
    struct flock fl;

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;

    snprintf(path, sizeof(path), "%s/%s", dir, "ext_info");
    if ((fd_old = open(path, O_CREAT|O_RDWR|O_APPEND,
                       S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open extracted file in add_table_oid ");
        return;
    }

    if (fcntl(fd_old, F_SETLKW, &fl) == -1)
    {
        perror("\tProblem locking in add_table_oid ");
        exit(1);
    }
    else
    {
        pool_debug("\tFile locked in add_table_oid!\n");
    }

    /*
     * To minimize the resource consumption in reading and comparing all
     * the checksums in a file. This workaround will force the read and
     * comparison to start only from OFFSET_VAL characters before the
     * end of the file
     */
    if (lseek(fd_old, OFFSET_VAL, SEEK_END) == -1)
    {
        perror("\tlseek in add_table_oid ");
    }
    else
    {
        while (1)
        {
            if ((read_bytes = read(fd_old, &c, 1)) == -1)
            {
                perror("\tSkipping till newline after lseek in add_table_oid");
                exit(1);
            }
            else if (read_bytes == 0)
            {
                break;
            }

            if (c == '\n')
            {
                break;
            }
        }
    }

    while (1)
    {
        /* reading the checksum */
        if ((read_bytes = read(fd_old, &tmpkey_from_file, (MD5KEYSIZE-1))) != -1)
        {

            if (read_bytes == 0)
            {
                pool_debug("\tFinished reading file!!!\n");
                break;
            }
            else if (read_bytes > 0 && read_bytes < (MD5KEYSIZE-1))
            {
                pool_debug("\tThis should not be executed! OMG! :\(\n");
                break;
            }

            /* skipping the first white space after checksum */
            if (read(fd_old, &c, 1) == -1)
            {
                perror("\tSkipping the first white space after \
                checksum in add_table_oid");
                exit(1);
            }

            if (c == '\n')      /* for meta, checksum is followed by newline */
            {
                pool_debug("\tIt is a meta command. Ignore in add_file_oids!\n");
                continue;
            }

            tmpkey_from_file[(MD5KEYSIZE-1)] = '\0';

            pool_debug("\tREAD checksum %s of size %zd from file in add_table_oid\n"
                       ,tmpkey_from_file, strlen(tmpkey_from_file));

            if (memcmp(tmpkey, tmpkey_from_file, MD5KEYSIZE) == 0)
            {
                pool_debug("\t %s of size %d EQUAL TO %s of size %d\n", tmpkey,
                           strlen(tmpkey), tmpkey_from_file,
                           strlen(tmpkey_from_file));

                while (1)       /* reading oid and opening its file */
                {
                    if (read(fd_old, &oid_from_file, 5) == -1)
                    {
                        perror("\tReading oid from ext info file in add_table_oid");
                        exit(1);
                    }
                    else
                    {
                        oid_from_file[5] = '\0';
                        pool_debug("\tREAD oid %s of size %zd from oidfile in \
                        add_table_oid\n",oid_from_file, strlen(oid_from_file));

                        /* writing to OID file */
                        write_to_oid_file(db_name, oid_from_file, tmpkey_from_file);
                    }

                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext \
                        info file in add_table_oid");
                        exit(1);
                    }
                    if (c == '\n')
                        break;      /* required checksum found, now exit from while */

                }       /* end of inner while */

                break;      /* required checksum found, now exit from while */
            }       /* end of inner if */
            else
            {
                pool_debug("\t %s of size %d NOT EQUAL TO %s of size %d\n",
                           tmpkey, strlen(tmpkey), tmpkey_from_file,
                           strlen(tmpkey_from_file));

                /*skip that line if checksum do not match */
                while (1)
                {
                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext info \
                        file in add_table_oid");
                        exit(1);
                    }
                    if (c == '\n')
                        break;
                }

                continue;       /*read more checksums if no match occurred */
            }       /* end of inner else */

        }       /* end of outermost if */
        else
        {
            perror("\tReading 2 from extracted info file in add_table_oid");
            exit(1);
        }       /* end of outermost else */

        break;
    }       /* while loop ends */

    fl.l_type = F_UNLCK;        /* Unlocking the file */
    if (fcntl(fd_old, F_SETLK, &fl) == -1)
    {
        perror("\tUnlocking in add_table_oid ");
        exit(1);
    }
    else
    {
        pool_debug("\tFile unlocked in add_table_oid! ");
    }
    close(fd_old);
}

/*
 * Writing matched received checksum from extracted info storage to oid file
 */
void write_to_oid_file(char db_name[DBLENGTH], char oid_from_file[OIDLENGTH],
                       char tmpkey_from_file[MD5KEYSIZE])
{
    int flag = 0, fd;
    //int read_bytes;
    //char tmpkey_from_oid_file[MD5KEYSIZE];
    char path[PATHLENGTH];
    struct flock fl_new;

    fl_new.l_type   = F_WRLCK;
    fl_new.l_whence = SEEK_SET;
    fl_new.l_start  = 0;
    fl_new.l_len    = 0;

    /* create or open oid file to write checksum */
    snprintf(path, sizeof(path), "%s/%s/%s/%s", dir, "oiddir",
             db_name, oid_from_file);
    if ((fd = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IROTH)) == -1)
    {
        perror("\tCreating or opening oid file\n");
    }
    else
    {
        if (fcntl(fd, F_SETLKW, &fl_new) == -1)
        {
            perror("\tProblem locking oid file in add_table_oid ");
            exit(EXIT_FAILURE);
        }
        else
        {
            pool_debug("\tOID file locked in add_table_oid!\n");
        }

        /* if already an entry exists in oid file, then do not write again */
        #if OID_FILE_ENTRY_DUPLICATION
        while (1)
        {
            if ((read_bytes = read(fd, &tmpkey_from_oid_file,
                                   (MD5KEYSIZE-1))) != -1)
            {

                if (read_bytes == 0)
                {
                    pool_debug("\tFinished reading file!!!\n");
                    break;
                }
                tmpkey_from_oid_file[(MD5KEYSIZE-1)] = '\0';

                if (memcmp(tmpkey_from_file, tmpkey_from_oid_file,
                           MD5KEYSIZE) == 0)
                {
                    flag = 1;
                    lseek(fd, 0, SEEK_END);
                    break;
                }
            }
            else
            {
                perror("\tOID file in add_table_oid for match checking ");
                break;
            }
        }       /* end of inner while loop */
        #endif // OID_FILE_ENTRY_DUPLICATION


        if (flag == 0)
        {
            if (write(fd, tmpkey_from_file, strlen(tmpkey_from_file)) == -1)
            {
                perror("\tError writing to oid file\n");
            }
        }

        fl_new.l_type = F_UNLCK;        /* Unlocking the file */
        if (fcntl(fd, F_SETLK, &fl_new) == -1)
        {
            perror("\tUnlocking in add_table_oid ");
            exit(1);
        }
        else
        {
            pool_debug("\tFile unlocked in add_table_oid! ");
        }
        close(fd);
    }       /* end of outermost else */
}

/*
 * Skipping comments of a query
 */
char *skip_comment_space(char *query)
{
    if (strncmp(query, "/*", 2) == 0)
    {
        query += 2;
        while (query)
        {
            if (strncmp(query, "*/", 2) == 0)
            {
                query += 2;
                break;
            }
            query++;
        }
    }

    /* skip spaces */
    while (isspace(*query))
        query++;
    return (char *)query;
}

/*
 * Read checksum from oid file and invalidate memcached entry
 */
void invalidate_query_cache(POOL_CONNECTION *frontend, char query1[MAXDATASIZE])
{
    char *tmpkey = malloc(sizeof(char) *MD5KEYSIZE), *query2;
    char db_name[DBLENGTH], to_hash[MAXDATASIZE], path[PATHLENGTH];
    char *query = malloc(sizeof(char) * MAXDATASIZE);

    query2 = query;
    strncpy(query, query1, MAXDATASIZE);
    query[MAXDATASIZE-1] = '\0';

    query = skip_comment_space(query);
    if (strlen(query) == (MAXDATASIZE-1))
    {
        query[MAXDATASIZE-2] = ';';
    }

    strcpy(to_hash, frontend->database);
    strncat(to_hash, query, (MAXDATASIZE-strlen(frontend->database)-1) );
    pg_md5_hash(to_hash, strlen(to_hash), tmpkey);

    if (mkdir(dir, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tCREATION of directory1 failed in invalidate_query_cache ");
            exit(EXIT_FAILURE);
        }
    }

    snprintf(path, sizeof(path), "%s/%s", dir, "oiddir");

    if (mkdir(path, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tCREATION of directory2 failed in invalidate_query_cache ");
            exit(EXIT_FAILURE);
        }
        else
        {
            perror("\tDIRECTORY already exists! ");
        }
    }

    strcpy(db_name, frontend->database);
    /* Create database_oid folder */
    snprintf(path, sizeof(path), "%s/%s/%s", dir, "oiddir", db_name);
    if (mkdir(path, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tDB directory\n");
            exit(EXIT_FAILURE);
        }
    }

    int read_bytes, fd_old;
    char tmpkey_from_file[MD5KEYSIZE], oid_from_file[OIDLENGTH], c;
    struct flock fl;

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;

    snprintf(path, sizeof(path), "%s/%s", dir, "ext_info_inva");
    if ((fd_old = open(path, O_CREAT|O_RDWR|O_APPEND,
                       S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open extracted file in invalidate_query_cache ");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd_old, F_SETLKW, &fl) == -1)
    {
        perror("\tProblem locking in invalidate_query_cache ");
        exit(EXIT_FAILURE);
    }
    else
    {
        pool_debug("\tFile locked in invalidate_query_cache!\n");
    }

    /*
     * To minimize the resource consumption in reading and comparing all
     * the checksums in a file. This workaround will force the read and
     * comparison to start only from OFFSET_VAL characters before the
     * end of the file
     */
    if (lseek(fd_old, OFFSET_VAL, SEEK_END) == -1)
    {
        perror("\tlseek in invalidate_query_cache ");
    }
    else
    {
        while (1)
        {
            if ((read_bytes = read(fd_old, &c, 1)) == -1)
            {
                perror("\tSkipping till newline after lseek in invalidate_query_cache");
                exit(1);
            }
            else if (read_bytes == 0)
            {
                break;
            }

            if (c == '\n')
            {
                break;
            }
        }
    }

    while (1)
    {
        if ((read_bytes = read(fd_old, &tmpkey_from_file,
                               (MD5KEYSIZE-1))) != -1)
        {

            if (read_bytes == 0)
            {
                pool_debug("\tFinished reading file!!!\n");
                break;
            }
            else if (read_bytes > 0 && read_bytes < (MD5KEYSIZE-1))
            {
                pool_debug("\tThis should not be executed! OMG! :\(\n");
                break;
            }           /*skipping the first white space after checksum */

            if (read(fd_old, &c, 1) == -1)
            {
                perror("\tSkipping the first white space after \
                checksum in invalidate_query_cache");
                exit(EXIT_FAILURE);
            }
            tmpkey_from_file[(MD5KEYSIZE-1)] = '\0';
            pool_debug("\tREAD checksum %s of size %zd from file in \
                    invalidate_query_cache\n",tmpkey_from_file, strlen(tmpkey_from_file));

            if (memcmp(tmpkey, tmpkey_from_file, MD5KEYSIZE) == 0)
            {
                pool_debug("\t %s of size %d EQUAL TO %s of size %d\n", tmpkey,
                           strlen(tmpkey), tmpkey_from_file,
                           strlen(tmpkey_from_file));

                while (1)
                {
                    if ((read_bytes = read(fd_old, &oid_from_file, 5)) == -1)
                    {
                        perror("\tReading oid from ext_info_inva \
                        file in invalidate_query_cache");
                        exit(EXIT_FAILURE);
                    }
                    else if (read_bytes < 5)
                    {
                        break;
                    }
                    else
                    {
                        oid_from_file[5] = '\0';
                        pool_debug("\tREAD oid %s of size %zd from file in \
                        invalidate_query_cache in inva part\n",oid_from_file,
                                   strlen(oid_from_file));

                        /* 0 on failure */
                        if (!read_from_oid_file(db_name, oid_from_file))
                        {
                            continue;
                        }
                    }

                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext \
                        info file in invalidate_query_cache");
                        exit(EXIT_FAILURE);
                    }

                    if (c == '\n')
                    {
                        break;
                    }

                }       /* inner while ends */

                break;
            }       /* inner if ends */
            else
            {
                pool_debug("\t %s of size %d NOT EQUAL TO %s of size %d\n",
                           tmpkey, strlen(tmpkey), tmpkey_from_file,
                           strlen(tmpkey_from_file));
                while (1)
                {
                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext \
                        info file in invalidate_query_cache");
                        exit(1);
                    }
                    if (c == '\n')
                        break;
                }
                continue;

            }       /* inner else ends */

        }       /* outermost if ends */
        else
        {
            perror("\tReading 2 from extracted info file in invalidate_query_cache");
            exit(EXIT_FAILURE);
        }       /* outermost else ends */

        break;
    }       /* while loop ends */

    fl.l_type = F_UNLCK;        /* Unlocking the file */
    if (fcntl(fd_old, F_SETLK, &fl) == -1)
    {
        perror("\tUnlocking in file_oids ");
        exit(1);
    }
    else
    {
        pool_debug("\tFile ext_info_inva unlocked in invalidate_query_cache! ");
    }
    close(fd_old);
    free(tmpkey);
    free(query2);
}

/*
 * Read checksum from oid file
 */
int read_from_oid_file(char db_name[DBLENGTH], char oid_from_file[OIDLENGTH])
{
    int fd, read_bytes;
    char path[PATHLENGTH], tmpkey_to_be_inva[MD5KEYSIZE];
    //char command[COMLENGTH];
    struct flock fl_new;
    memcached_return rc;

    fl_new.l_type   = F_WRLCK;
    fl_new.l_whence = SEEK_SET;
    fl_new.l_start  = 0;
    fl_new.l_len    = 0;

    /* create or open oid file */
    snprintf(path, sizeof(path), "%s/%s/%s/%s", dir, "oiddir", db_name, oid_from_file);
#if ENABLE_COMMAND
    snprintf(command, sizeof(command), "rm -rf %s",path);
#endif // ENABLE_COMMAND

    if ((fd = open(path, O_CREAT|O_RDWR|O_APPEND|O_TRUNC, S_IRWXU|S_IROTH)) == -1)
    {
        perror("\tCreating or opening oid file\n");
        /*
         * This may be normal. It is possible that no SELECT has
         * been issued since the program has been started.
         */

        /*
        if (system(command) == -1)
        {
            perror("\tSystem command ");
            exit(EXIT_FAILURE);
        }
        */
        return 0;       /* if more oid files to be read */
    }
    else
    {

        if (fcntl(fd, F_SETLKW, &fl_new) == -1)
        {
            perror("\tProblem locking oid file in invalidate_query_cache ");
            exit(1);
        }
        else
        {
            pool_debug("\tOID file %s locked for invalidation in \
            inva part!\n", oid_from_file);
        }

        /* fetch hash key from oid file and delete it */
        while (1)
        {
            if ((read_bytes = read(fd, &tmpkey_to_be_inva, (MD5KEYSIZE-1))) != -1)
            {
                if (read_bytes == 0)
                {
                    pool_debug("\tFinished reading file!!!\n");
                    break;
                }
                else if (read_bytes > 0 && read_bytes < (MD5KEYSIZE-1))
                {
                    pool_debug("\tThis should not be executed! OMG! :\(\n");
                    break;
                }
                tmpkey_to_be_inva[(MD5KEYSIZE-1)] = '\0';

                pool_debug("\tREAD key %s of size %zd from file in invalidate_query_cache \
                in inva part\n",tmpkey_to_be_inva, strlen(tmpkey_to_be_inva));

                memcached_st *memc = get_memc();
                rc= memcached_delete(memc, (char *)tmpkey_to_be_inva,
                                     (MD5KEYSIZE-1), (time_t)0);

                if (rc == MEMCACHED_SUCCESS)
                {
                    pool_debug("\tMEMCACHED ENTRY DELETED!!!\n");
                }
                else if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED)
                {
                    perror("\tFAILED to delete from memcached! ");
                }

            }       /* inner if ends */
            else
            {
                perror("\tReading ext_info_inva ");
                break;
            }

        }       /* while ends */

        /*
        if (system(command) == -1)
        {
            perror("\tSystem command ");
            exit(EXIT_FAILURE);
        }
        */

        /* flushing the contents of the OID file after invalidation */
        if (write(fd, "", 0) == -1 )   /* TRUNC mode */
        {
            perror("\tFailed in TRUNCating invalidated OID file contents ");
        }
        pool_debug("\tFlushed invalidated OID file contents. ");

        fl_new.l_type = F_UNLCK;        //Unlocking the file
        if (fcntl(fd, F_SETLK, &fl_new) == -1)
        {
            perror("\tUnlocking in invalidate_query_cache ");
            exit(1);
        }
        else
        {
            pool_debug("\tFile oid unlocked in invalidate_query_cache! ");
        }

        close(fd);
    }           /* outermost else ends */

    return 1;
}

/*
 * Check if the query is meta by probing meta query file storage
 */
int check_if_meta(char tmpkey[MD5KEYSIZE])
{
    pool_debug("\tEntered check_if_meta\n");
    int fd_new, read_bytes, flag = 0;
    struct flock fl_new;
    char tmpkey_from_file[MD5KEYSIZE], c, path[PATHLENGTH];

    fl_new.l_type = F_WRLCK;
    fl_new.l_whence = SEEK_SET;
    fl_new.l_start = 0;
    fl_new.l_len = 0;

    snprintf(path, sizeof(path), "%s/%s", dir, "ext_info_meta");
    if ((fd_new = open(path, O_CREAT|O_RDWR|O_APPEND,
                       S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open file ext_info_meta ");
        exit(EXIT_FAILURE);
    }

    /* locking ext info file */
    if (fcntl(fd_new, F_SETLKW, &fl_new) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    else
    {
        pool_debug("\tFILE locked in ext_info_meta!\n");
    }

    /*
     * To minimize the resource consumption in reading and comparing all
     * the checksums in a file. This workaround will force the read and
     * comparison to start only from OFFSET_VAL characters before the
     * end of the file
     */
    if (lseek(fd_new, OFFSET_VAL, SEEK_END) == -1)
    {
        perror("\tlseek in check_if_meta ");
    }
    else
    {
        while (1)
        {
            if ((read_bytes = read(fd_new, &c, 1)) == -1)
            {
                perror("\tSkipping till newline after lseek in invalidate_query_cache");
                exit(1);
            }
            else if (read_bytes == 0)
            {
                break;
            }

            if (c == '\n')
            {
                break;
            }
        }
    }

    while (1)
    {
        if ((read_bytes = read(fd_new, &tmpkey_from_file,
                               (MD5KEYSIZE-1))) != -1)
        {

            if (read_bytes == 0)
            {
                pool_debug("\tFinished reading file!!!\n");
                break;
            }
            else if (read_bytes > 0 && read_bytes < (MD5KEYSIZE-1))
            {
                pool_debug("\tThis should not be executed! OMG! :\(\n");
                break;
            }
            tmpkey_from_file[(MD5KEYSIZE-1)] = '\0';
            pool_debug("\tREAD checksum %s of size %zd from meta file in \
                    invalidate_query_cache\n",tmpkey_from_file, strlen(tmpkey_from_file));

            if (memcmp(tmpkey, tmpkey_from_file, MD5KEYSIZE) == 0)
            {
                pool_debug("\t %s of size %d EQUAL TO %s of size %d\n",
                           tmpkey, strlen(tmpkey), tmpkey_from_file,
                           strlen(tmpkey_from_file));

                flag = 1;   /* match found */
                break;
            }       /* inner if ends */
            else
            {
                pool_debug("\t %s of size %d NOT EQUAL TO %s of size %d\n",
                           tmpkey, strlen(tmpkey), tmpkey_from_file,
                           strlen(tmpkey_from_file));

                /*skipping the new line after checksum */
                if (read(fd_new, &c, 1) == -1)
                {
                    perror("\tSkipping the new line after checksum in \
                    file_oids");
                    exit(EXIT_FAILURE);
                }

                continue;

            }       /* inner else ends */

        }       /* outermost if ends */
        else
        {
            perror("\tReading 2 from extracted info file in invalidate_query_cache");
            exit(EXIT_FAILURE);
        }       /* outermost else ends */

    }       /* while loop ends */

    fl_new.l_type = F_UNLCK;                 /* Unlocking the file */
    if (fcntl(fd_new, F_SETLK, &fl_new) == -1)
    {
        perror("\tUnlocking in ext_info_hash ");
        close(fd_new);
        exit(EXIT_FAILURE);
    }
    else
    {
        pool_debug("\tFILE unlocked in ext_info_hash!\n");
    }

    close(fd_new);
    return flag;
}

