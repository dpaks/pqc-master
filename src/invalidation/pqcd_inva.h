#ifndef PQCD_INVA_H
#define PQCD_INVA_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

//void populate_inva_strucs(htable *, usedlist *, int , char *);
void send_to_mmap(char *, int);

char *get_from_mmap(int *);

//void populate_inva_strucs(htable *, int , char *);

#endif // PQCD_INVA_H
