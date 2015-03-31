/*
 * FILE: mmap_store.c
 * HEADER: invalidation/mmap_store.h
 *
 * Stores and retrieves data received from PostgreSQL backend into MMAP
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

#include <sys/mman.h>

#include "invalidation/mmap_store.h"
#include "pool.h"
#include "invalidation/main_headers.h"

/*
 * Writing to MMAP
 */
void send_to_mmap(char *str, int numbytes)
{
    int fdin;
    void *src;
    struct stat sbuf;
    char path[PATHLENGTH];

    snprintf(path, sizeof(path), "%s/%s", dir, "mmapfile.txt");

    if ((fdin = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR |
                     S_IROTH | S_IWOTH)) < 0)
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

    /* need size of file area where data can be written to */
    if (fstat(fdin, &sbuf) < 0)
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
 * Retrieving from MMAP
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


