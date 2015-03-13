#include "invalidation/ext_info_hash.h"
#include "invalidation/md5.h"
#include "invalidation/main_headers.h"
#include "pool.h"

#define EMAXDATASIZE 100
#define MAX_KEY 256

/***********************************************Extracting and Storing****************************************************/

void store_extracted_info(char buf[EMAXDATASIZE], int numbytes)
{
    int i, j;
    char *query_from_buf = (char *)malloc((numbytes-2-5) * sizeof(char));      //-2-shifting, -6-relid, +1-NULL char
    char oid_from_buf[6];

    char *checksum = malloc(sizeof(char) *MAX_KEY);
    int fd, oid_size;
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

    pg_md5_hash(query_from_buf, strlen(query_from_buf), checksum);

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

    pool_debug("\tWROTE checksum\n");

    while (buf[i] != '\0')
    {
        oid_size = 5;
        while (oid_size > 0)
        {
            oid_from_buf[j++] = buf[i++];
            oid_size--;
        }
        oid_from_buf[j] = '\0';
        j = 0;
        pool_debug("\tEXTRACTED OID FROM BUF: %s of size: %d", oid_from_buf, strlen(oid_from_buf));

        if ((write(fd, " ", 1) == -1) | (write(fd, oid_from_buf, 5) == -1))
        {
            perror("\tWrite error for OID! ");
        }
        else
        {
            pool_debug("\tWROTE oid: %s\n", oid_from_buf);
        }
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

    close(fd);
}

