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
#include <string.h>
#include <pthread.h>

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
#define DEBUG

/* Global Variable for Semaphore */
sem_t sem, sem_log;

/* Functions to define */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

/* Helper Function */
int parse_uri(char *uri, char *hostname, char *pathname, int *port);
void log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void print_log(struct sockaddr_in *sockaddr, char *uri, int size);

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
 * open_clientdf_ts - wrapper function for Open_clientfd
 * Use semaphore and cell Open_clientfd,
 * to make it thread safe
 */
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp)
{
    int result;
    printf("Hostname: %s, port: %d", hostname, port);
    P(&sem);
    result = Open_clientfd(hostname, port);
    V(&sem);
    return result;
}

/*--------------------------------------------------------------------*/
/*
 * Rio_readn_w - wrapper function for rio_readn
 * Call rio_readn and if there is an error,
 * do not terminate, but print error msg and return 0.
 */
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0){
    printf("rio_readn error");
    n=0;
    }
    return n;
}

/*
 * Rio_readlineb_w - wrapper function for rio_readlineb
 * Call rio_readlineb and if there is an error,
 * do not terminate, but print error msg and return 0.
 */
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0){
    printf("rio_readlineb error");
    rc = 0;
    }
    return rc;
}

/*
 * Rio_writen_w - wrapper function for rio_writen
 * Call rio_writen and if there is an error,
 * do not terminate, but print error msg and return 0.
 */
void Rio_writen_w(int fd, void *usrbuf, size_t n)
{
    ssize_t n;
    
    if ((n = rio_writen(fd, usrbuf, n)) < 0){
        printf("rio_writen error");
        n=0;
    }
    return n;
}

/*---------------------Helper Function--------------------------*/
/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')
	*port = atoi(hostend + 1);

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	pathname[0] = '\0';
    }
    else {
	pathbegin++;
	strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * log_entry - Set the log file entry as directed
 * "date: clientIP clientPort size echostring"
 * 1. Get the date info in correct form
 * 2. Get clientIP and Convert it to dotted decimal manually,
 *    because inet ntoa is a thread-unsafe function
 * 3. Get clientport, size, echostring
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get the date info */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* Get IP & conversion */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    sprintf(logstring, "%s: %d.%d.%d.%d %d %s\n", time_str, a, b, c, d, size, uri);
}

/*
 * print_log - print to proxy.log
 */
void print_log(struct sockaddr_in *sockaddr, char *uri, int size)
{
    char *logstring[MAXLINE];
    format_log_entry(logstring, sockaddr, uri, size);
    P(&sem_log);
    fprintf(PROXY_LOG, logstring);
    fflush(PROXY_LOG);
    V(&sem_log);
}

