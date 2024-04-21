
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct {
	int  *socketfd;
	int  *acceptfd;
	FILE  *istream;
	FILE  *ostream;
	struct sockaddr * theiraddres;
	pid_t *child_pid;
	int *socfd;
	//int *acceptfd;
	struct addrinfo *hints;
	struct addrinfo *servinfo;
	struct sockaddr_storage *their_addr;

}Context;

char *readline(char * buffer, size_t * bufflen);
int recieve_text(FILE * istream, FILE *ostream);
int freeup(Context * ctx);