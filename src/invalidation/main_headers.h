#ifndef MAIN_HEADERS_H
#define MAIN_HEADERS_H

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#define MAXDATASIZE 251     /* max query size */
#define MD5KEYSIZE 33
#define PATHLENGTH 150
#define DBLENGTH 20
#define OIDLENGTH 6
#define BUFSIZE 1000    /* size of buffer that accepts data from client */

extern char *dir;       /* path where files have to be created & is
                                defined in my_server.c */

#endif
