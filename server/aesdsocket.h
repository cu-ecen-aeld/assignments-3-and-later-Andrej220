
#include <stdio.h>
//#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

typedef struct {
	//int  *socketfd;
	int  *acceptfd;
	FILE  *istream;
	FILE  *ostream;
	FILE  *backstream;
	char *backstream_buffer;
	struct sockaddr * theiraddres;
	pid_t *child_pid;
	int *socfd;
	//int *acceptfd;
	struct addrinfo *hints;
	struct addrinfo *servinfo;
	struct sockaddr_storage *their_addr;
	char s[INET6_ADDRSTRLEN];

}Context;

char *readline(char * buffer, size_t * bufflen);
int recieve_text(FILE * istream, FILE *ostream);
int closeconnection(Context * ctx);
int open_connection(Context *ctx);