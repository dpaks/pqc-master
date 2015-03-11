#include "invalidation/main_headers.h"
#include "invalidation/file_oids.h"

void pool_add_table_oid_map(POOL_CONNECTION *frontend, char tmpkey[33])
{
    char db_name[50];
    char path[1024];

    char *dir = "/tmp/mypqcd";

    if (mkdir(dir, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tCREATION of directory1 failed in my_server.c ");
            // return;
        }
    }

    char *dir1 = "/tmp/mypqcd/oiddir";

    if (mkdir(dir1, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {

            perror("\tCREATION of directory2 failed in my_server.c ");
        }
        else
            perror("\tDIRECTORY already exists! ");
    }

    snprintf(path, sizeof(path), "/tmp/mypqcd/oiddir");

    strcpy(db_name, frontend->database);

    snprintf(path, sizeof(path), "%s/%s", dir1, db_name);        // Create database_oid folder
    if (mkdir(path, S_IRWXU|S_IRWXO) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tDB directory\n");
        }
    }

    int read_bytes, fd, fd_old;
    char tmpkey_from_file[33], oid_from_file[6], c;
    struct flock fl;

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;

    snprintf(path, sizeof(path), "%s/%s", dir1, "ext_info");
    if ((fd_old = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IRWXO|S_IRWXG)) == -1)
    {
        perror("\tFailed to open extracted file in file_oids ");
        return;
    }
    fchmod(fd_old, S_IRWXU|S_IRWXO|S_IRWXG);
    if (fcntl(fd_old, F_SETLKW, &fl) == -1)
    {
        perror("\tProblem locking in pqc.c ");
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
            if (read(fd_old, &c, 1) == -1)      //skipping the first white space after checksum
            {
                perror("\tSkipping the first white space after checksum in pqc.c\n");
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
                        perror("\tReading oid from ext info file in pqc.c\n");
                        exit(1);
                    }
                    else
                    {
                        oid_from_file[5] = '\0';
                        pool_debug("\tREAD oid %s of size %zd from file in file_oids\n",oid_from_file, strlen(oid_from_file));
                        snprintf(path, sizeof(path), "%s/%s/%s", dir1, db_name, oid_from_file);      //create oid file
                        if ((fd = open(path, O_CREAT|O_RDWR|O_APPEND, S_IRWXU|S_IROTH)) == -1)
                        {
                            perror("\tCreating oid file\n");
                        }
                        else
                        {
                            if (write(fd, tmpkey_from_file, strlen(tmpkey_from_file)) == -1)
                            {
                                perror("\tError writing to oid file\n");
                            }
                        }
                        close(fd);
                    }
                    if (read(fd_old, &c, 1) == -1)
                    {
                        perror("\tReading till new line char from ext info file in pqc.c\n");
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
                        perror("\tReading till new line char from ext info file in pqc.c\n");
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
            if (read_bytes == -1)
            {
                perror("\tReading 2 from extracted info file in pqc.c\n");
                exit(1);
            }
            else if (read_bytes > 0)
            {
                pool_debug("\tThis should not be executed! OMG! :\(\n");
            }
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
