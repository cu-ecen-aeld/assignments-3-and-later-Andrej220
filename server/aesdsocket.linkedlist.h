
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
#define FILENAME "/dev/aesdchar"
//#define FILENAME "/var/tmp/aesdsocketdata"
const char *fileName = FILENAME;
#define LOGFACILITY "Assignments"

struct Connections_t {
    int 		done;
	FILE 	   *ostream;
	const char	   *filename;
	int  		istream;
	pthread_t 	pid;
	char 		s[INET6_ADDRSTRLEN];
    SLIST_ENTRY(Connections_t) entries;
} ;

typedef struct {
	FILE  *	backstream;
	char *	backstream_buffer;
	struct 	sockaddr * theiraddres;
	int *	socfd;
	struct 	addrinfo *hints;
	struct 	addrinfo *servinfo;
	struct 	sockaddr_storage *their_addr;
	char 	s[INET6_ADDRSTRLEN];
}Context;

typedef struct {
	struct Connections_t *conn;
} Textreciveargs_t;


void* recieve_text(void *t);
int closeconnection(Context * ctx);
int open_connection(Context *ctx);
int sendfile(FILE * fd, int stream);
int deleteFile();
void start_daemon();

int savetofile(char * buffer, FILE * ostream);
int savetime(char * buffer);
void setuptimer();
void * timer_thread();
void timer_handler();
void check_completed_threads();
void delElement(struct Connections_t *conn);
struct Connections_t * newElement();