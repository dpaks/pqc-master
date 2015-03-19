#include <sys/mman.h>

#include "invalidation/mmap_store.h"
#include "pool.h"
#include "invalidation/main_headers.h"

/*
 *Writing to File using MMAP
 */
void send_to_mmap(char *str, int numbytes)
{
    int fdin;
    void *src;
    struct stat sbuf;
    char path[PATHLENGTH];

    snprintf(path, sizeof(path), "%s/%s", dir, "mmapfile.txt");

    if ((fdin = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH)) < 0)
    {
        perror("can’t open mmapfile for reading");
        exit(EXIT_FAILURE);
    }

    lseek(fdin, numbytes, SEEK_SET);    /* area used by received data */
    if (write(fdin, "", 1) == -1)
    {
        perror("Writing into mmap file error");
        exit(EXIT_FAILURE);
    }
    lseek(fdin, 0, SEEK_SET);

    if (fstat(fdin, &sbuf) < 0)        /* need size of file area where data can be written to */
    {
        perror("fstat error");
        exit(EXIT_FAILURE);
    }

    if ((src = mmap(0, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fdin, 0)) == MAP_FAILED)
    {
        perror("mmap error for input");
        exit(EXIT_FAILURE);
    }

    memcpy(src, str, numbytes);

    munmap(src, sbuf.st_size);
    close(fdin);
    pool_debug("\tWritten to mmap successfully\n");
}

/*
 *Retrieving from File
 */
char *get_from_mmap(int *numbytes)
{
    FILE *fp;
    char path[PATHLENGTH];

    snprintf(path, sizeof(path), "%s/%s", dir, "mmapfile.txt");

    char *buf = malloc(sizeof(char) * BUFSIZE);
    if(!buf)
    {
        perror("Malloc failed!");
        exit(EXIT_FAILURE);
    }
    int i=0;

    fp = fopen(path,"r");

    if( fp == NULL )
    {
        perror("Error while opening the file.\n");
    }

    while ((buf[i] = fgetc(fp) ) != '\0')
    {
        i++;
    }
    ++i;
    buf[i] = '\0';
    *numbytes = i;

    fclose(fp);
    pool_debug("\tRead from mmap successfully\n");
    return buf;
}


