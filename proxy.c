/*
 * proxy.c - M1522.000800 System Programming: Web proxy
 *
 * Student ID: 2012-13276	
 *         Name: Yeli Kim
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
#define DEBUG

/*
 * Functions to define
 */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    int port, listenfd;
    FILE *log_file;
    
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }
    
    port = atoi(argv[1]);
    
    if ((listenfd=open_listenfd(port))==-1){
        unix_error("Error in listening");
    }
    
    log_file = fopen(PROXY_LOG, "a");
    
    exit(0);
}


/*
 * open_clientdf_ts - Thread safe wrapper for Open_clientfd
 */
int open_clientfd_ts(char *hostname, int port)
{
    int result;
    printf("Hostname: %s, port: %d", hostname, port);
    P(&sem);
    result = Open_clientfd(hostname, port);
    V(&sem);
    return result;
}

/*-----------Wrappers for robust I/O routines rewritten---------*/

ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
    printf("Rio_readn error");
    return n;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
    printf("Rio_readlineb error");
    return rc;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
    printf("Rio_writen error");
}


