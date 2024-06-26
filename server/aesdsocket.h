
#include <stdio.h>
//#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>

#define PORTNUMBER "9000"
#define MAXBUFFER 4096
#define WAITINGTIME 10
#define FILENAME "/var/tmp/aesdsocketdata"

struct Connections_t {
    int done;
	pthread_t pid;
    SLIST_ENTRY(Connections_t) entries;
} ;

typedef struct {
	//int  *socketfd;
	int  *acceptfd;
	FILE  *istream;
	FILE  *ostream;
	FILE  *backstream;
	char *backstream_buffer;
	struct sockaddr * theiraddres;
	int *socfd;
	struct addrinfo *hints;
	struct addrinfo *servinfo;
	struct sockaddr_storage *their_addr;
	char s[INET6_ADDRSTRLEN];
}Context;

typedef struct {
	int  istream;
	FILE *ostream;
	struct Connections_t *conn;
	char *s;
	
} Textreciveargs_t;


void* recieve_text(void *t);

int closeconnection(Context * ctx);
int open_connection(Context *ctx);
int sendfile(char *filename, Context *ctx);
int deleteFile();
void start_daemon();

int savetofile(char * buffer);
void setuptimer();
void check_completed_threads();
void delElement(struct Connections_t *conn);
struct Connections_t * newElement(pthread_t pid);