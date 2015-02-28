#include "invalidation/my_server.h"
//#include "invalidation/pqcd_inva.h"
#include <pthread.h>
#include "invalidation/ext_info_hash.h"
#include "pool.h"


pthread_mutex_t lock_ll;
e_htable *eh;

e_htable **get_ehtable_head()
        {
           return &eh;
        }

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *recv_info(void *arg)
{
        pool_debug("\n\nEntered recv_info\n");

        eh = init_e_htable();

    if (pthread_mutex_init(&lock_ll, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        //return 1;
    }

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1, index;
    char s[INET6_ADDRSTRLEN];
    //int rv;
    int rv, numbytes, c_status;
    char buf[MAXDATASIZE];

    pool_debug("\n\nEntered recv_info\n");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("\n\nserver: waiting for connections...\n");

    while(1) {  // main accept() loop
           // pool_debug("\n\nTESTING WHILE LOOP START Address of main head is %u & allocated head is %u", (my_head_of_ll), (my_head_of_ll)->head);



        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("\n\nserver: got connection from %s\n", s);

        //pthread_mutex_lock(&lock_ll);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

    if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';

    if (buf[0] == 't' && numbytes > 2)      //t: cacheable query ; >2: in the called fn, we shift elements of array to left by two positions
    {
        printf("\n\nserver: received '%s' of size %d\n",buf, numbytes);

        pool_debug("\n\nInvoking store_extracted_info\n");
        pool_debug("\n\nTESTING Before CS Address of main head is %u, allocated head is %u & size is %d", (eh), (eh)->table, eh->tot_size);

        index = store_extracted_info(&eh, buf, numbytes);

       // pool_debug("\n\nTESTING After CS Address of main head is %u & allocated head is %u", (my_head_of_ll), (my_head_of_ll)->head);
   //  pool_debug("\n\nTESTING After CS Address of e_htable is %u, allocated block oid is %u & size is %d", (eh)->table, eh->table[index]->next->oid, eh->tot_size);

  //  if(eh->table[index]->next->next != NULL)
  //  pool_debug("\n\nTESTING After CS After second input allocated block oid 1 is %u & 2 is %u & size is %d", eh->table[index]->next->oid, eh->table[index]->next->next->oid, eh->tot_size);
    }
        pool_debug("\n\nTESTING WHILE LOOP END Address of e_htable is %u & allocated block oid is %u", (eh)->table, eh->table[index]->next->oid);

        close(new_fd);
        exit(0);
    }

            if (wait(&c_status) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }

    //pthread_mutex_unlock(&lock_ll);

    close(new_fd);  // parent doesn't need this
    pool_debug("\n\n\t111111111111111111111111111XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
    }
    pthread_mutex_destroy(&lock_ll);
    pool_debug("\n\n\t2222222222222222222222222XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");

    return NULL;
}
