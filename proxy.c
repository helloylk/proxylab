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
FILE *logfile

/* Functions to define */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

/* Helper Function */
int parse_uri(char *uri, char *hostname, char *pathname, int *port);
void log_entry(char *logstring, struct sockaddr_in *sockaddr, int size);
void print_log(struct sockaddr_in *sockaddr, int size);

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    int port, listenfd;
    int *connfd;
    void **vargp;
    struct sockaddr_in clientaddr;
    int clientlen=sizeof(clientaddr);
    struct hostent *hp;
    	
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
    
    logfile = fopen(PROXY_LOG, "a");
    
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
        printf("Client connected to %s (%s), port: %d\n", hp->h_name, client_ip, client_port);
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
	switch (cnt++){
		case 0:
	        strcpy(hostname, token);
	        break;
	      	case 1:
	        port=atoi(token);
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
   sprintf(leadLine, "%s %d", hostname, port);
   Rio_writen_w(serverfd, leadLine, strlen(leadLine)); 

   while((n = Rio_readlineb_w(&rio_client, buf, MAXLINE)) > 0 && buf[0] != '\r' && buf[0] != '\n') {
	printf("%s", buf);
	Rio_writen_w(serverfd, buf, n);
  }
  Rio_writen_w(serverfd, "\r\n", 2);

  printf("\nRESPOND: \n%s", leadLine);
  
  // Read response from server
  cnt=0;
  while((n = Rio_readn_w(serverfd, buf, MAXLINE)) > 0) {
	// printf("%s\n", buf);
	Rio_writen_w(connfd, buf, n);
    	cnt += n;
  }

  /* Close all openfile and print log */
  Close(serverfd);
  Close(connfd);
  //print_log(clientaddr, cnt);
  return NULL;
}

/*
 * open_clientdf_ts - wrapper function for Open_clientfd
 * Use semaphore and call Open_clientfd,
 * to make it thread safe
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

/*--------------------------------------------------------------------*/
/*
 * Rio_readn_w - wrapper function for rio_readn
 * Call rio_readn and if there is an error,
 * do not terminate, but print error msg and return 0.
 */
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes){
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
    size_t=wb;
    if ((wb = rio_writen(fd, usrbuf, n)) < 0){
        printf("rio_writen error");
        n=0;
    }
    return;
}

/*---------------------Helper Function--------------------------*/
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

    sprintf(logstring, "%s: %d.%d.%d.%d \n", time_str, a, b, c, d);
}

/*
 * print_log - print to proxy.log
 */
void print_log(struct sockaddr_in *sockaddr, int size)
{
    char *logstring[MAXLINE];
    log_entry(logstring, sockaddr, size);
    P(&sem_log);
    fprintf(logfile, logstring);
    fflush(logfile);
    V(&sem_log);
}

