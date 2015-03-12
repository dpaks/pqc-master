#include "invalidation/pqcd_inva.h"
#include "pool.h"

/***********************************************Writing to File using MMAP****************************************************/

void send_to_mmap(char *str, int numbytes)
{
    int fdin;
    void *src;
    struct stat sbuf;

    if ((fdin = open("/tmp/mypqcd/mmapfile.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH)) < 0)
        perror("can’t open mmapfile for reading");

    lseek(fdin, 100, SEEK_SET);            //Assuming that maximum query size is 100 characters
    if (write(fdin, "", 1) == -1)
    {
        perror("Writing into mmap file error");
    }
    lseek(fdin, 0, SEEK_SET);

    if (fstat(fdin, &sbuf) < 0) /* need size of input file */
        perror("fstat error");

    if ((src = mmap(0, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fdin, 0)) == MAP_FAILED)
        perror("mmap error for input");

    memcpy(src, str, numbytes);

    close(fdin);
}

/***********************************************Retrieving from File****************************************************/

char *get_from_mmap(int *numbytes)
{
    FILE *fp;
    char *buf = malloc(sizeof(char) * 100);          //Assuming that maximum query size is 100 characters
    if(!buf)
    {
        perror("Malloc failed!");
    }
    int i=0;

    fp = fopen("/tmp/mypqcd/mmapfile.txt","r");

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

    /*lseek(fdin, 0, SEEK_SET);
    if (write(fdin, "", 1) == -1)
    {
        perror("Writing into mmap file error");
    }*/
    fclose(fp);
    return buf;

}


