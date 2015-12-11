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

#define MAX 1024

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
    int *connfd;
    void **vargp;
    struct sockaddr_in clientaddr;
    int clientlen=sizeof(clientaddr);
    	
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }
    
    /* Prepare samaphore */
    sem_init(&sem, 0, 1);
    sem_init(&sem_log, 0, 1);
    
    port = atoi(argv[1]);
    
    /* Open listen side */
    if ((listenfd=open_listenfd(port))==-1) unix_error("Listenfd Error");
    
    log_file = fopen(PROXY_LOG, "a");
    
    while(1){
    	pthread_t tid;
    	char *client_ip;
    	int client_port;
    	
    	/* Malloc variables for thread-safety */
    	vargp = Malloc(2*sizeof(void *));
    	connfd = Malloc(sizeof(int));
        
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        /* Prepare one vargs as an argument for Pthread_create */
        vargp[0] = connfd;
        vargp[1] = &clientaddr;
        printf("initial connfd %d at address %p\n", *connfd, connfd);

        /* Determne the domain name and IP of the client */
        hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
          sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        client_ip = inet_ntoa(clientaddr.sin_addr);
        client_port = ntohs(clientaddr.sin_port);
        printf("Client connected to %s (%s)\n", hp->h_name, client_ip);
        Pthread_create(&tid, NULL, process_request, (void *)vargp); 
    }
    exit(0);
}

/*
 * process_request - thread routine
 *
 */
void *process_request(void* vargp){
   rio_t rio_client, rio_server;
   int port, serverfd;
   int n, cnt=0;
   char buf[MAX], hostname[MAX], leadLine[MAX];
   char *token;
	
   int connfd = *(int *)*(void **)vargp;
   struct sockaddr_in *clientaddr = (struct sockaddr_in *)(vargp + sizeof(int));
   free(vargp);
   Pthread_detach(Pthread_self());
   
   n = Rio_readlineb_w(&rio_client, buf, MAX);
   Rio_writen_w(connfd, buf, n);
  
   /* Get argm from client command */
   token = strtok(buf, " ");
   while (token != NULL) {
	switch (cnt++)
	{
		case 0:
	        strcpy(hostname, token);
	        break;
	      	case 1:
	        strcpy(port, token);
	        break;
	 }
    token = strtok(NULL, " ");
  }
   
   // after a successful request, connect to the server
   if ((serverfd = open_clientfd_ts(hostname, port)) < 0){
   	perror("Proxy: Server connection error");
   	Close(connfd);
        return NULL;
   }
   Rio_readinitb(&rio_server, serverfd);

   // Write the initial header to the server
   sprintf(leadLine, "%s %s", hostname, port);
   Rio_writen_w(serverfd, leadLine, strlen(leadLine)); 

   while((n = Rio_readlineb_w(&rio_client, buf, MAXLINE)) > 0 && buf[0] != '\r' && buf[0] != '\n') {
	printf("%s", buf);
	Rio_writen_w(serverfd, buf, n);
  }
  Rio_writen_w(serverfd, "\r\n", 2);

  printf("\nRESPOND: \n%s", leadLine);
  
  // Read response from server
  cnt=0;
  while((n = Rio_readnb_w(&rio_server, buf, MAXLINE)) > 0) {
	// printf("%s\n", buf);
	Rio_writen_w(connfd, buf, n);
    	cnt += n;
  }

  /* Close all openfile and print log */
  Close(serverfd);
  Close(connfd);
  //print_log(clientaddr, uri, totalByteCount);
  return NULL;
}

/*
 * open_clientdf_ts - wrapper function for Open_clientfd
 * Use semaphore and call Open_clientfd,
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
void log_entry(char *logstring, struct sockaddr_in *sockaddr, int size)
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

    sprintf(logstring, "%s: %d.%d.%d.%d %d %d %s\n", time_str, a, b, c, d, port, size, echostring);
}

/*
 * print_log - print to proxy.log
 */
void print_log(struct sockaddr_in *sockaddr, char *uri, int size)
{
    char *logstring[MAXLINE];
    log_entry(logstring, sockaddr, uri, size);
    P(&sem_log);
    fprintf(PROXY_LOG, logstring);
    fflush(PROXY_LOG);
    V(&sem_log);
}

