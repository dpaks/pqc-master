#include <libmemcached/memcached.h>
#include <time.h>

#include "invalidation/main_headers.h"
#include "invalidation/file_oids.h"
#include "invalidation/md5.h"

void add_table_oid(POOL_CONNECTION *frontend, char tmpkey[33])
{
    char db_name[50];
    char path[1024];

    char *dir = "/tmp/mypqcd";

    if (mkdir(dir, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tCREATION of directory1 failed in file_oids.c ");
            // return;
        }
    }

    char *dir1 = "/tmp/mypqcd/oiddir";

    if (mkdir(dir1, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {

            perror("\tCREATION of directory2 failed in file_oids.c ");
        }
        else
            perror("\tDIRECTORY already exists! ");
    }

    strcpy(db_name, frontend->database);

    snprintf(path, sizeof(path), "%s/%s", dir1, db_name);        // Create database_oid folder
    if (mkdir(path, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tDB directory\n");
        }
    }

    int read_bytes, fd, fd_old;
    char tmpkey_from_file[33], oid_from_file[6], c;
    struct flock fl, fl_new;

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;

    fl_new.l_type   = F_WRLCK;
    fl_new.l_whence = SEEK_SET;
    fl_new.l_start  = 0;
    fl_new.l_len    = 0;

    snprintf(path, sizeof(path), "%s/%s", dir, "ext_info");
    if ((fd_old = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open extracted file in file_oids ");
        return;
    }
    fchmod(fd_old, S_IRWXU|S_IRWXO|S_IRWXG);
    if (fcntl(fd_old, F_SETLKW, &fl) == -1)
    {
        perror("\tProblem locking in file_oids ");
        exit(1);
    }
    else
    {
        pool_debug("\tFile locked in file_oids!\n");
    }

    while (1)
    {
        if ((read_bytes = read(fd_old, &tmpkey_from_file, 32)) != -1)
        {
            if (read_bytes == 0)
            {
                pool_debug("\tFinished reading file!!!\n");
                break;
            }
            else if (read_bytes > 0 && read_bytes < 32)
            {
                pool_debug("\tThis should not be executed! OMG! :\(\n");
                break;
            }
            if (read(fd_old, &c, 1) == -1)      //skipping the first white space after checksum
            {
                perror("\tSkipping the first white space after checksum in file_oids");
                exit(1);
            }
            tmpkey_from_file[32] = '\0';
            pool_debug("\tREAD checksum %s of size %zd from file in file_oids\n",tmpkey_from_file, strlen(tmpkey_from_file));
            if (memcmp(tmpkey, tmpkey_from_file, 33) == 0)          //comparing the last byte Null terminator too
            {
                pool_debug("\t %s of size %d EQUAL TO %s of size %d\n", tmpkey, strlen(tmpkey), tmpkey_from_file, strlen(tmpkey_from_file));
                while (1)
                {
                    pool_debug("\tENTERED while loop in file_oids\n");
                    if (read(fd_old, &oid_from_file, 5) == -1)
                    {
                        perror("\tReading oid from ext info file in file_oids");
                        exit(1);
                    }
                    else
                    {
                        oid_from_file[5] = '\0';
                        pool_debug("\tREAD oid %s of size %zd from file in file_oids\n",oid_from_file, strlen(oid_from_file));
                        snprintf(path, sizeof(path), "%s/%s/%s", dir1, db_name, oid_from_file);      //create oid file
                        if ((fd = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IROTH)) == -1)
                        {
                            perror("\tCreating or opening oid file\n");
                        }
                        else
                        {

                            if (fcntl(fd, F_SETLKW, &fl_new) == -1)
                            {
                                perror("\tProblem locking oid file in file_oids ");
                                exit(1);
                            }
                            else
                            {
                                pool_debug("\tOID file locked in file_oids!\n");
                            }

                            if (write(fd, tmpkey_from_file, strlen(tmpkey_from_file)) == -1)
                            {
                                perror("\tError writing to oid file\n");
                            }

                            fl_new.l_type = F_UNLCK;        //Unlocking the file
                            if (fcntl(fd, F_SETLK, &fl_new) == -1)
                            {
                                perror("\tUnlocking in file_oids ");
                                exit(1);
                            }
                            else
                            {
                                pool_debug("\tFile unlocked in file_oids! ");
                            }
                            close(fd);

                        }
                    }
                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext info file in file_oids");
                        exit(1);
                    }
                    if (c == '\n')
                        break;
                }
                continue;
            }
            else
            {
                pool_debug("\t %s of size %d NOT EQUAL TO %s of size %d\n", tmpkey, strlen(tmpkey), tmpkey_from_file, strlen(tmpkey_from_file));
                while (1)
                {
                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext info file in file_oids");
                        exit(1);
                    }
                    if (c == '\n')
                        break;
                }
                continue;
            }
        }
        else
        {
            perror("\tReading 2 from extracted info file in file_oids");
            exit(1);
        }
        break;
    }       //while loop ends

    fl.l_type = F_UNLCK;        //Unlocking the file
    if (fcntl(fd_old, F_SETLK, &fl) == -1)
    {
        perror("\tUnlocking in file_oids ");
        exit(1);
    }
    else
    {
        pool_debug("\tFile unlocked in file_oids! ");
    }
    close(fd_old);
}

void invalidate_query_cache(POOL_CONNECTION *frontend, char query[256])         //max query length is assumed to be 256
{
    char path[1024];
    memcached_return rc;

    char *tmpkey = malloc(sizeof(char) *33);      //size of md5 is 32

    pg_md5_hash(query, strlen(query), tmpkey);

    char db_name[50];
    char *dir = "/tmp/mypqcd";

    if (mkdir(dir, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tCREATION of directory1 failed in file_oids.c ");
            // return;
        }
    }

    char *dir1 = "/tmp/mypqcd/oiddir";

    if (mkdir(dir1, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {

            perror("\tCREATION of directory2 failed in file_oids.c ");
        }
        else
        {
            perror("\tDIRECTORY already exists! ");
        }
    }

    strcpy(db_name, frontend->database);

    snprintf(path, sizeof(path), "%s/%s", dir1, db_name);        // Create database_oid folder
    if (mkdir(path, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tDB directory\n");
        }
    }

    int read_bytes, fd, fd_old;
    char tmpkey_from_file[33], oid_from_file[6], c, tmpkey_to_be_inva[33];
    struct flock fl, fl_new;

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;

    fl_new.l_type   = F_WRLCK;
    fl_new.l_whence = SEEK_SET;
    fl_new.l_start  = 0;
    fl_new.l_len    = 0;

    snprintf(path, sizeof(path), "%s/%s", dir, "ext_info_inva");
    if ((fd_old = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open extracted file in file_oids ");
        return;
    }
    fchmod(fd_old, S_IRWXU|S_IRWXO|S_IRWXG);
    if (fcntl(fd_old, F_SETLKW, &fl) == -1)
    {
        perror("\tProblem locking in file_oids ");
        exit(1);
    }
    else
    {
        pool_debug("\tFile locked in file_oids!\n");
    }

    while (1)
    {
        if ((read_bytes = read(fd_old, &tmpkey_from_file, 32)) != -1)
        {
            if (read_bytes == 0)
            {
                pool_debug("\tFinished reading file!!!\n");
                break;
            }
            else if (read_bytes > 0 && read_bytes < 32)
            {
                pool_debug("\tThis should not be executed! OMG! :\(\n");
                break;
            }
            if (read(fd_old, &c, 1) == -1)      //skipping the first white space after checksum
            {
                perror("\tSkipping the first white space after checksum in file_oids");
                exit(1);
            }
            tmpkey_from_file[32] = '\0';
            pool_debug("\tREAD checksum %s of size %zd from file in file_oids\n",tmpkey_from_file, strlen(tmpkey_from_file));
            if (memcmp(tmpkey, tmpkey_from_file, 33) == 0)          //comparing the last byte Null terminator too
            {
                pool_debug("\t %s of size %d EQUAL TO %s of size %d\n", tmpkey, strlen(tmpkey), tmpkey_from_file, strlen(tmpkey_from_file));
                while (1)
                {
                    pool_debug("\tENTERED while loop in file_oids inva part\n");
                    if (read(fd_old, &oid_from_file, 5) == -1)
                    {
                        perror("\tReading oid from ext_info_inva file in file_oids");
                        exit(1);
                    }
                    else
                    {
                        oid_from_file[5] = '\0';
                        pool_debug("\tREAD oid %s of size %zd from file in file_oids in inva part\n",oid_from_file, strlen(oid_from_file));
                        snprintf(path, sizeof(path), "%s/%s/%s", dir1, db_name, oid_from_file);      //create oid file
                        if ((fd = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IROTH)) == -1)
                        {
                            perror("\tCreating or opening oid file\n");

                            /* This may be normal. It is possible that no SELECT has
                            * been issued since the table has been created or since
                            * pgpool-II started up.
                            */

                            /*  if (read(fd_old, &c, 1) == -1)
                              {
                                  perror("\tReading till new line char from ext info file in file_oids");
                                  exit(1);
                              }
                              if (c == '\n')
                              {
                                  break;      //move to next line
                              }*/

                            continue;       //if more oids to be read
                        }
                        else
                        {

                            if (fcntl(fd, F_SETLKW, &fl_new) == -1)
                            {
                                perror("\tProblem locking oid file in file_oids ");
                                exit(1);
                            }
                            else
                            {
                                pool_debug("\tOID file locked in file_oids in inva part!\n");
                            }

                            //retrieve hash n delete them
                            while (1)
                            {
                                pool_debug("\nEntering INNER WHILE LOOP in inva part\t");
                                if ((read_bytes = read(fd, &tmpkey_to_be_inva, 32)) != -1)
                                {
                                    if (read_bytes == 0)
                                    {
                                        pool_debug("\tFinished reading file!!!\n");
                                        break;
                                    }
                                    else if (read_bytes > 0 && read_bytes < 32)
                                    {
                                        pool_debug("\tThis should not be executed! OMG! :\(\n");
                                        break;
                                    }
                                    tmpkey_to_be_inva[32] = '\0';
                                    pool_debug("\tREAD key %s of size %zd from file in file_oids in inva part\n",tmpkey_to_be_inva, strlen(tmpkey_to_be_inva));
                                    memcached_st *memc = get_memc();
                                    pool_debug("\tMEMC in inva part is %d\n",memc);
                                    pool_debug("\tJust before MEMCACHED DELETE\n");
                                    rc= memcached_delete(memc, (char *)tmpkey_to_be_inva, 32, (time_t)0);

                                    if (rc == MEMCACHED_SUCCESS)
                                    {
                                        pool_debug("\tMEMCACHED ENTRY DELETED!!!\n");
                                    }
                                    else if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_BUFFERED)
                                    {
                                        perror("\tFAILED to delete from memcached! ");
                                        //pool_debug("\tMemcached Return Value is %s\n",rc);
                                        continue;
                                    }
                                    pool_debug("\tJust after MEMCACHED DELETE\n");
                                }
                                else
                                {
                                    perror("\tReading ext_info_inva ");
                                    break;
                                }
                            }

                            fl_new.l_type = F_UNLCK;        //Unlocking the file
                            if (fcntl(fd, F_SETLK, &fl_new) == -1)
                            {
                                perror("\tUnlocking in file_oids ");
                                exit(1);
                            }
                            else
                            {
                                pool_debug("\tFile unlocked in file_oids! ");
                            }

                            close(fd);
                        }
                    }
                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext info file in file_oids");
                        exit(1);
                    }
                    if (c == '\n')
                    {
                        break;      //move to next line
                    }
                }
                continue;
            }

            else
            {
                pool_debug("\t %s of size %d NOT EQUAL TO %s of size %d\n", tmpkey, strlen(tmpkey), tmpkey_from_file, strlen(tmpkey_from_file));
                while (1)
                {
                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext info file in file_oids");
                        exit(1);
                    }
                    if (c == '\n')
                        break;
                }
                continue;
            }
        }
        else
        {
            perror("\tReading 2 from extracted info file in file_oids");
            exit(1);
        }
        break;
    }       //while loop ends

    fl.l_type = F_UNLCK;        //Unlocking the file
    if (fcntl(fd_old, F_SETLK, &fl) == -1)
    {
        perror("\tUnlocking in file_oids ");
        exit(1);
    }
    else
    {
        pool_debug("\tFile unlocked in file_oids! ");
    }
    close(fd_old);
}

