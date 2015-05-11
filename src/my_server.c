/*
 * FILE: my_server.c
 * HEADER: invalidation/my_server.h
 *
 * Receives data from PostgreSQL backend and sends it for extraction
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include "invalidation/my_server.h"
#include "invalidation/mmap_store.h"
#include "pqc.h"
#include "invalidation/ext_info_hash.h"
#include "pool.h"
#include "invalidation/main_headers.h"

#define PORT "3490"  /* the port users will be connecting to */
#define BACKLOG 10     /* how many pending connections queue will hold */

static void sigchld_handler(int );
static void *get_in_addr(struct sockaddr *);

char *dir = "/tmp/mypqcd";

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)              /* get socket addr */
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *recv_info(void *arg)
{
    char path[PATHLENGTH], s[INET6_ADDRSTRLEN], buf[BUFSIZE];
    /* listen on sock_fd, new connection on new_fd */
    int yes=1, rv, numbytes, c_status, sockfd, new_fd, fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;     /* connector's address information */
    socklen_t sin_size;
    struct sigaction sa;
    char *new_buf = NULL;

    pthread_mutex_t lock_ll;

    if (mkdir(dir, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {
            perror("\tCREATION of directory1 failed in my_server.c ");
        }
        else
        {
            pool_debug("\tDIRECTORY \"%s\" already exists!\n", path);
        }
    }

    snprintf(path, sizeof(path), "%s/%s", dir, "oiddir");

    if (mkdir(path, S_IRWXU|S_IRWXO|S_IRWXG) == -1)
    {
        if (errno != EEXIST)
        {

            perror("\tCREATION of directory2 failed in my_server.c ");
        }
        else
        {
            pool_debug("\tDIRECTORY \"%s\" already exists!\n", path);
        }
    }

    if (pthread_mutex_init(&lock_ll, NULL) != 0)
    {
        perror("\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;            /* use my IP */

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return (void *)1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return (void *)2;
    }

    freeaddrinfo(servinfo);             /* all done with this structure */

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;            /* reap all dead processes */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    pool_debug("\tserver: waiting for connections...\n");

    while(1)                                        /*concurrent server */
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        pool_debug("\tserver: got connection from %s\n", s);

        if (!fork())
        {
            close(sockfd);                /* child doesn't need the listener */

            if ((numbytes = recv(new_fd, buf, BUFSIZE, 0)) == -1)
            {
                perror("recv");
                exit(1);
            }
            pool_debug("\tSize of %s using strlen is %d\n", buf, strlen(buf));
            buf[numbytes] = '\0';

            /*
             *   t:cacheable, f:invalidateable; >2: in the called fn, we
             *     shift elements of array to left by two positions
             */
            if (numbytes > 2)
            {
                send_to_mmap(buf, numbytes);    /* invoking shared memory */
            }
            close(new_fd);
            exit(0);
        }

        if (wait(&c_status) == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        close(new_fd);              /* parent doesn't need this */

        pthread_mutex_lock(&lock_ll);               /* mutex lock */

        numbytes = 0;
        new_buf = get_from_mmap(&numbytes);

        /* extracting and storing */
        store_extracted_info(new_buf, numbytes);
        pool_debug("\tBUF RETRIEVED FROM MMAP: %s of size %d",
                   new_buf, numbytes);
        free(new_buf);
        pthread_mutex_unlock(&lock_ll);               /* mutex unlock */

    }
    close(fd);
    pthread_mutex_destroy(&lock_ll);

    return NULL;
}
