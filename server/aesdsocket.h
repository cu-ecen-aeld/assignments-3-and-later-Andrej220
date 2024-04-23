
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

int recieve_text(int istream, FILE *ostream);
int closeconnection(Context * ctx);
int open_connection(Context *ctx);
int sendfile(char *filename, Context *ctx);
int deleteFile();
void start_daemon();