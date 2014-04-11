/*
 * 18-213 proxy lab
 * Name: Yichao Xue
 * ID: yichaox
 *
 * proxy.c 
 * final version 2.0: a simple proxy program with cache deal with multiple
 *					  concurrent requests with synchronization.
 * 
 * 1. use rwlock to synchronize multiple threads
 * 2. use mutex for thread-safe gethostbyname
 */

#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
//rwlock
static pthread_rwlock_t rwlock;
//mutex for thread-safe gethostbyname
static sem_t mutex;
//external varible
extern cache_entry *head;

/* function declarations */
void doit(int fd);
int parse_uri(char *uri, char *hostname, char *path, char *port);
void read_requesthdrs(rio_t *rio, char *hostname, char *new_headers);
void clienterror(int fd, char *msg);
void *thread(void *vargp);

/* wrapper for rwlock functons */
void Pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
void Pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
void Pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

/* thread-safe gethostbyname and open_clientfd */
int open_clientfd_ts(char *hostname, int port);
struct hostent* gethostbyname_ts(char* host);


/* start of main function */
int main(int argc, char **argv)
{    

	printf("%s\n","Proxy is running...");

	int listenfd, *connfd, port;
	struct sockaddr_in clientaddr;		//client address
    socklen_t clientlen = sizeof(clientaddr);
    pthread_t tid;

    //ignore sigpipe
    signal(SIGPIPE,SIG_IGN);

	//check command line args
	if(argc != 2){
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	//get port number
	port = atoi(argv[1]);

	//init mutex
	Sem_init(&mutex, 0, 1);

	//init rwlock
	if(pthread_rwlock_init(&rwlock, NULL) != 0){
		unix_error("Error: initialize rwlock failed");
	}

	//init cache
	init_cache();

	//open listen fd
	listenfd = Open_listenfd(port);

	while(1){
		connfd = Malloc(sizeof(int));
		*connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

		printf("%s\n","Receive a request...");

		Pthread_create(&tid, NULL, thread, connfd);
	}
    
   	empty_cache();
   	
   	//destroy rwlock
   	if(pthread_rwlock_destroy(&rwlock)){
   		unix_error("Error: destroy rwlock failed");
   	}

    return 0;
}

/*doit: function to deal with multiple concurrent requests */
void doit(int fd){
	char buf[MAXLINE];
	char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char hostname[MAXLINE], path[MAXLINE], port_str[MAXLINE];
	char new_headers[MAXLINE] = {};
	char reply[MAXLINE];

	char request_buf[MAXLINE];
	char content_buf[MAX_OBJECT_SIZE];
	cache_entry *cache_buf;

	int port;
	int clientfd; 
	int nbytes;

	size_t obj_size = 0;
	rio_t rio, rio2;
    
	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);	//read the first line of the request

	//cpy request
	strcpy(request_buf, buf);


	sscanf(buf, "%s %s %s", method, uri, version);

	//check the method 
	if(strcasecmp(method, "GET")){
		clienterror(fd, "Error: Method not implemented.");
		return;
	}

	//read cache: add read lock
	Pthread_rwlock_rdlock(&rwlock);
	//hit
	if((cache_buf = is_cached(request_buf)) != NULL){
		
		Rio_writen(fd, cache_buf->content, cache_buf->size);
		//unlock r lock
		Pthread_rwlock_unlock(&rwlock);

		//add w lock
		Pthread_rwlock_wrlock(&rwlock);

		move2front(cache_buf);
		//unlock
		Pthread_rwlock_unlock(&rwlock);
		return;
	}	
	//finish read, unlock r lock
	Pthread_rwlock_unlock(&rwlock);

	//parse the uri, get hostname, path and port
	if(parse_uri(uri, hostname, path, port_str) == -1){
		clienterror(fd, "Error: cannot parse uri.");
		return;
	}
	port = atoi(port_str);

	//read the headers from client and form new headers
    read_requesthdrs(&rio, hostname, new_headers);

    /* right now, new headers is ready */
    sprintf(buf, "%s %s HTTP/1.0\r\n", method, path);
    strcat(buf, new_headers);

    if((clientfd = open_clientfd_ts(hostname, port)) < -1){
    	clienterror(fd, "Error: fail to connect to a host");
    }

    /* forwoard request to server*/
    Rio_writen(clientfd, buf, strlen(buf));

    //reset
    bzero(buf, MAXLINE);
    bzero(reply,MAXLINE);

    Rio_readinitb(&rio2, clientfd);

    //get server's response and send back to client
    while((nbytes = Rio_readnb(&rio2, reply, MAXLINE)) > 0){
    	obj_size += nbytes;

    	if(obj_size < MAX_OBJECT_SIZE){
    		sprintf(content_buf, "%s%s", content_buf, reply);
    	}

    	Rio_writen(fd, reply, nbytes);
    }

    if(obj_size < MAX_OBJECT_SIZE){
    	//write cache, add a w lock
    	Pthread_rwlock_wrlock(&rwlock);
    	//write content to cache
    	insert_cache_entry(request_buf, content_buf, obj_size);
    	//unlock
    	Pthread_rwlock_unlock(&rwlock);
    }

    bzero(request_buf, MAXLINE);
    Close(clientfd);
}

/*
 * read_requesthdrs: read headers from request, form new request header
 */
void read_requesthdrs(rio_t *rio, char *hostname, char *new_headers){
	char buf[MAXLINE]; 
	int has_host = 0;

	//form new headers
	strcat(new_headers, user_agent_hdr);
	strcat(new_headers, accept_hdr);
	strcat(new_headers, accept_encoding_hdr);
	sprintf(new_headers, "%sConnection: %s", new_headers, "close\r\n");
	sprintf(new_headers, "%sProxy-Connection: %s", new_headers, "close\r\n");

	Rio_readlineb(rio, buf, MAXLINE);		
	while(strcmp(buf, "\r\n")){
		if(strstr(buf, "User-Agent") || strstr(buf, "Accept") ||
		 strstr(buf, "Accept-Encoding") || strstr(buf, "Connection") ||
		  strstr(buf, "Proxy-Connection"))
		{
			//ignore
		}
		else
		{
			if(strstr(buf, "Host")){
				//if there is host, attach host to new headers
				strcat(new_headers, buf);	
				has_host = 1;
			}else{
				//attach other headers to new headers
				strcat(new_headers, buf);	
			}
		}

		Rio_readlineb(rio, buf, MAXLINE);
	}

	if(!has_host){
		sprintf(new_headers, "%sHost: %s\r\n", new_headers, hostname);
	}

	strcat(new_headers, "\r\n");
	return;
}

/*
 * parse_uri: parse the uri, get host name, path and port #
 */
int parse_uri(char *uri, char *hostname, char *path, char *port){

	char *hostname_ptr;
	char *path_ptr;
	char *port_ptr;
	char host[MAXLINE];

	if(strncasecmp(uri, "http://", strlen("http://")) != 0){
		printf("%s\n","invalid uri");
		return -1;
	}

	hostname_ptr = uri + strlen("http://"); 
	
	//get path
	if((path_ptr = strchr(hostname_ptr, '/')) != NULL){
		strcpy(path, path_ptr);
		strncpy(host, hostname_ptr, (path_ptr - hostname_ptr));
		host[path_ptr - hostname_ptr] = '\0';	//end of the string
	}else{
		strcpy(path, "/");
		strcpy(host, hostname_ptr);
	}

	//get hostname and port
	if((port_ptr = strchr(host, ':')) != NULL){
		strcpy(port, port_ptr + 1);
		strncpy(hostname, host, (port_ptr - host));
		hostname[port_ptr - host] = '\0';
	}else{
		strcpy(hostname, host);
		strcpy(port, "80");		//default port
	}

	return 0;
}

/*
 * clienterror: send an error message back to client
 */
void clienterror(int fd, char *msg){
	char buf[MAXLINE] = {};

	sprintf(buf, "%s\n", msg);
	Rio_writen(fd, buf, strlen(buf));
}

/* thread: a single thread to handle the request */
void *thread(void *vargp){
	int connfd = *((int *)vargp);
	Pthread_detach(pthread_self());
	Free(vargp);
	doit(connfd);
	Close(connfd);
	return NULL;
}

/* wrapper function for rwlock */
//add a read lock
void Pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    if(pthread_rwlock_rdlock(rwlock)) {
        unix_error("Error: add read lock failed");
    }
}
//add a write lock
void Pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    if(pthread_rwlock_wrlock(rwlock)) {
        unix_error("Error: add write lock failed");
    }
}
//unlock
void Pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    if(pthread_rwlock_unlock(rwlock)) {
        unix_error("Error! unlock failed");
    }
}

//thread-safe version of open_clientfd
int open_clientfd_ts(char *hostname, int port) 
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */

    //here use a thread-safe gethostbyname
    if ((hp = gethostbyname_ts(hostname)) == NULL)
	return -2; /* check h_errno for cause of error */

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
	  (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
	return -1;
    return clientfd;
}

//thread-safe version of gethostbyname
struct hostent* gethostbyname_ts(char* host)
{
    struct hostent* shared, * unsharedp;
    unsharedp = Malloc(sizeof(struct hostent));
    P(&mutex);
    shared = gethostbyname(host);
    *unsharedp = * shared;
    V(&mutex);
    return unsharedp;
}

