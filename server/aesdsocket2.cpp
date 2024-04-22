#include "aesdsocket.h"
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

using namespace std;

#define PORTNUMBER "9000"
#define MAXBUFFER 10

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int recieve_text(FILE * stream){

}

char *readline(char * buffer){

}

int main(){
	// get socket discriptor
	int status;
	int socfd, newfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct sockaddr *addr;
	socklen_t addr_size;
	struct sockaddr_storage their_addr;
	
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	const char *msg = "Hello there\n";
	char s[INET6_ADDRSTRLEN];
	char recvbuff[MAXBUFFER];
	
	if ((status = getaddrinfo(NULL, PORTNUMBER, &hints, &servinfo)) != 0) {
		printf("getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	socfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (socfd==-1){
		freeaddrinfo(servinfo);
		perror("socket");
		exit(1);
	}
	
	freeaddrinfo(servinfo);

	//bind
	if (bind(socfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
		perror("bind");
		exit(1);
	}

	//listen
	if (listen(socfd, 1) == -1){
	perror("listen");
		exit(1);
	}

	//connect
	while(1){
		addr_size = sizeof their_addr;
		newfd = accept(socfd, (struct sockaddr *)&their_addr, &addr_size);
		if (newfd == -1) {
			perror("accept");
			continue;
		}
		FILE *stream = fdopen(dup(newfd),"r");
		inet_ntop(their_addr.ss_family,	get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		//add logging
		char * buffer = NULL;
		size_t bufflen = 0;
		bool endoffile = false;
		if (! fork()){
			do {
				buffer = (char *) realloc(buffer, bufflen + MAXBUFFER);
				if (fgets(buffer + bufflen, MAXBUFFER ,stream) == NULL){
					endoffile = true;
					printf("%s", buffer);
					perror("End of file");
					break;
				} 
				//printf("recieved data :%s", buffer);
				printf("%s", buffer);

				char * newline_p = strchr(buffer + bufflen, '\n');
				printf("%s",newline_p);
				if (newline_p != NULL ){
					*newline_p = '\0';
					printf("%s", buffer);
					free(buffer);
					bufflen = 0;
					//flushbuffer(buffer, bufferlen, newline_p);
				}else{
					perror("No new line found.");
					bufflen += MAXBUFFER -1 ;
				}
			} while(!endoffile);
			perror("End of loop");
			if (buffer != NULL){
				printf("%s", buffer);
			}
			//free(buffer);
			exit(0);

		}
		//save buffer if is not empty
		fclose(stream);
	}
	close(newfd);
	close(socfd);
	perror("End of connetion");
	return 0;
}

