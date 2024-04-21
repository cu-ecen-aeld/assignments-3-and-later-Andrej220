#include "aesdsocket.h"

using namespace std;

#define PORTNUMBER "9000"
#define MAXBUFFER 512
//#define FILENAME "/var/tmp/aesdsocketdata"
#define FILENAME "/tmp/server.log"


Context global_ctx;

int freeup(Context * ctx){
        if (ctx->acceptfd != NULL) {
			if(shutdown (*ctx->acceptfd, SHUT_RDWR) < 0){
				perror("Shutdown socket error.");
			}
            close(*ctx->acceptfd);
        }

 		if (ctx->socketfd != NULL) {
            close(*ctx->socketfd);
        }
		
        if (ctx->istream != NULL) {
            fclose(ctx->istream);
        }
		
        if (ctx->ostream != NULL) {
            fclose(ctx->ostream);
        }
		
        if (ctx->hints != NULL) {
            free(ctx->hints);
        }
        if (ctx->servinfo != NULL) {
            free( ctx->servinfo);
        }
        if (ctx->their_addr != NULL) {
			
            //free(ctx->their_addr);
        }
	return 0;
}

void sigetrm_handler(int signum){
	if(signum == SIGINT || signum == SIGTERM){
		perror("Closing application...");
		freeup(&global_ctx);
		exit(EXIT_SUCCESS);
	}
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int recieve_text(FILE * istream, FILE *ostream){

	if(istream == NULL || ostream == NULL){
		perror("Invalid file pointers\n");
		return -1;
	}

    if (ferror(istream)) {
        perror("Error reading from input stream\n");
        return -1;
    }
    char *buffer;
    size_t bufflen = 0;
    buffer = (char *) malloc(MAXBUFFER);
    if (buffer == NULL){
        perror("Cannot allocate memory\n");
        return -1;
    }
    while(fgets(buffer + bufflen, MAXBUFFER ,istream) != NULL){
		char *line = readline(buffer, &bufflen);
		if (line == NULL){
			continue;
		}
		if (fputs(line, ostream) == EOF){
			perror("Error writing to output istrea\n");
		}
        printf("%s", line);
		fflush(ostream);
		free(line);
    }
    free(buffer);
    return 0;
}

char *readline(char * buffer, size_t *bufflen){
	if (buffer == NULL){
		perror("Invalid pointer");
		return NULL;
	}
	char * new_line_p = strchr(buffer, '\n');
	if (new_line_p != NULL){
		size_t newline_len = new_line_p - buffer ;
		char * line = (char *) malloc(newline_len +2);
		if (line == NULL){
			perror("Error allocating memory.");
			return NULL;
		}
		strncpy(line, buffer, newline_len +1);
		line[newline_len +1 ] = '\0';
		memmove(buffer, new_line_p +  1, strlen(new_line_p +1) +1);
		return line;
	}else{
		*bufflen = strlen(buffer);
		buffer = (char  *)realloc(buffer, *bufflen + MAXBUFFER + 1);
		if (buffer == NULL){
			perror("Error reallocating memory");
			exit(EXIT_FAILURE);
		}
	}
	return NULL;
}

int sendfile(char * filename, Context * ctx){
	return -1;
}

int connect(Context * ctx){
	return -1;
}

int closeconnection(Context * ctx){
	return -1;

}

int setupsigaction(){
	struct sigaction sa;
	sa.sa_handler = sigetrm_handler;
	sa.sa_flags = SA_SIGINFO;
	if(sigaction(SIGINT, &sa, NULL) == -1){
		perror("Error installing signal handler");
		return -1;
	}
	if(sigaction(SIGTERM, &sa, NULL) == -1){
		perror("Error installing signal handler");
		return -1;
	}
	return 0;
}

int main(){
	int child_status;
    pid_t pid;
	int socfd, acceptfd;
	struct addrinfo hints;
	//struct addrinfo *servinfo;
	socklen_t addr_size;
	struct sockaddr_storage their_addr;
	
	Context *ctx = &global_ctx;
	ctx->socketfd = &socfd;
	ctx->child_pid = &pid;
	ctx->acceptfd = &acceptfd;
	ctx->their_addr = & their_addr; 

	if (setupsigaction() == -1){
		perror("Error setup sig action");
		exit(EXIT_FAILURE);
	}

    openlog("Assignment 5-1", LOG_PID, LOG_LOCAL1);
	
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	char s[INET6_ADDRSTRLEN];
	
	if ((child_status = getaddrinfo(NULL, PORTNUMBER, &hints, &ctx->servinfo)) != 0) {
		printf("getaddrinfo error: %s\n", gai_strerror(child_status));
		exit(1);
	}
	
	socfd = socket(ctx->servinfo->ai_family, ctx->servinfo->ai_socktype, ctx->servinfo->ai_protocol);
	if (socfd==-1){
		freeaddrinfo(ctx->servinfo);
		ctx->servinfo = NULL;
		perror("socket");
		exit(1);
	}


	//bind
	if (bind(*ctx->socketfd, ctx->servinfo->ai_addr, ctx->servinfo->ai_addrlen) == -1){
		perror("bind");
		exit(1);
	}

	free(ctx->servinfo);
	ctx->servinfo = NULL;
	//listen
	if (listen(socfd, 1) == -1){
	    perror("listen");
		exit(1);
	}

	//connect
	while(1){
		addr_size = sizeof their_addr;
		acceptfd = accept(*ctx->socketfd, (struct sockaddr *)&their_addr, &addr_size);
		if (acceptfd == -1) {
			perror("Eroor accepting connection");
			continue;
		}
		inet_ntop(their_addr.ss_family,	get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
        syslog(LOG_DEBUG, "Accepted connection from %s", s);

		ctx->istream = fdopen(dup(acceptfd),"rw");
        if (ctx->istream == NULL){
            perror("Unable to open stream");
            return -1;
        }

        //int child_status;
		if ( (pid = fork()) < 0 ){
            perror("Could not create fork.");
            fclose(ctx->istream);
            close(acceptfd);
            continue;
        } 
        if (pid == 0){
            //FILE * ostream = fopen(FILENAME,"a+");
			ctx->ostream = fopen(FILENAME,"a+");
            recieve_text(ctx->istream, ctx->ostream);
			//send data back befoe return;
            fclose(ctx->istream);
			ctx->istream = NULL;
            fclose(ctx->ostream);
			ctx->ostream = NULL;
            //close(acceptfd);
            exit(EXIT_SUCCESS);
        } else{
            if (waitpid(pid, &child_status,0) == -1){
                perror("Waitpid failed");
                exit(EXIT_FAILURE);
            } 
            if ( WIFEXITED(child_status)){
				syslog(LOG_DEBUG, "Closed connection from %s", s);
                printf("Child process exited with status %d\n", WEXITSTATUS(child_status));
            }
        }
	}
	//close(acceptfd);
	//close(socfd);
	return 0;
}

