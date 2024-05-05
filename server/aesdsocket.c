#include "aesdsocket.h"

#define PORTNUMBER "9000"
#define MAXBUFFER 4096
#define WAITINGTIME 10
#define FILENAME "/var/tmp/aesdsocketdata"
//#define FILENAME "/tmp/server.log"

Context global_ctx;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t filemutex = PTHREAD_MUTEX_INITIALIZER;

void sigetrm_handler(int signum){
	syslog(LOG_DEBUG, "Signum: %d", signum);
	if (signum == SIGINT || signum == SIGTERM){
		syslog(LOG_ERR,"Closing application...");
		closeconnection(&global_ctx);
		exit(EXIT_SUCCESS);
	}
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET){
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void * recieve_text(void *t){

	Textreciveargs_t *params = (Textreciveargs_t*) t;

	if (params->istream == NULL || params->ostream == NULL){
		syslog(LOG_ERR,"Invalid file pointers\n");
		return NULL;
	}

	char *buffer;
	size_t bytes_recieved = 0;
	buffer = (char *)malloc(MAXBUFFER+1);
	if (buffer == NULL){
		syslog(LOG_ERR,"Cannot allocate memory\n");
		return NULL;
	}
	pthread_mutex_lock(&mutex);
	while( (bytes_recieved = recv(*params->istream,buffer,MAXBUFFER,0)) > 0 ){
		buffer[bytes_recieved] = '\0';
		savetofile(buffer);
		char * newline = strchr(buffer, '\n');
		if (newline != NULL){
			sendfile(FILENAME, &global_ctx);
			}
	}
	pthread_mutex_unlock(&mutex);
	free(buffer);
	return NULL;
}

int savetofile( char * buffer){
	FILE * fd = fopen(FILENAME, "a+");
	if (fd == NULL){
		syslog(LOG_ERR, "Unable to open file, timestamp");
		return -1;
	}
	pthread_mutex_lock(&filemutex);
	if (fputs(buffer, fd) == EOF){
			syslog(LOG_ERR,"Error writing to output stream\n");
			pthread_mutex_unlock(&filemutex);
			fclose(fd);
			return -1;
	}
	pthread_mutex_unlock(&filemutex);
	fflush(fd);
	fclose(fd);
	return 0;
}

void timer_handler(){
	syslog(LOG_DEBUG, "timer handler");
	char timestamp[128];
	time_t rawtime;
	struct  tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(timestamp, sizeof(timestamp), "timestamp:%Y %m %d %H\n", timeinfo);
	savetofile(timestamp);
}

void * timer_thread(){
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timer_handler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGRTMIN+1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN+1;
    sev.sigev_value.sival_ptr = &timerid;

    its.it_interval.tv_sec = WAITINGTIME;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = WAITINGTIME;
    its.it_value.tv_nsec = 0;

	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
    	syslog(LOG_ERR,"timer_create");
    	exit(EXIT_FAILURE);
    }
    if (timer_settime(timerid, 0, &its, NULL) == -1) {
    	syslog(LOG_ERR,"timer settime");
        exit(EXIT_FAILURE);
    }
    while (1) {
        sleep(1);
    }
}

void setuptimer(){
	pid_t pid = fork();

	if (pid < 0){
		syslog(LOG_ERR, "Eror starting timer fork");
		exit(EXIT_FAILURE);
	} else if(pid == 0){
		pthread_t timerthread_id;
		if( pthread_create(&timerthread_id, NULL, timer_thread, NULL) < 0) {
			syslog(LOG_ERR,"Could not create a thread for timer");
			exit(EXIT_FAILURE);
		}
		while(1){
			sleep(1);
		}
	}
}

int sendfile(char *filename, Context *ctx){
	ssize_t bytes_read;
	if ((ctx->backstream = fopen(filename, "r")) == NULL){
		syslog(LOG_ERR,"Error opening file to send back");
		return -1;
	}

	ctx->backstream_buffer = (char *)malloc(MAXBUFFER);
	while ((bytes_read = fread(ctx->backstream_buffer, 1, MAXBUFFER, ctx->backstream))){
		if ((send(*ctx->acceptfd, ctx->backstream_buffer, bytes_read, 0)) != bytes_read){
			syslog(LOG_ERR,"Error sending file back.");
			return -1;
		}
	}
	fclose(ctx->backstream);
	ctx->backstream = NULL;
	free(ctx->backstream_buffer);
	ctx->backstream_buffer = NULL;
	return 0;
}

int open_connection(Context *ctx){
	struct addrinfo hints;
	ctx->hints = &hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	if (getaddrinfo(NULL, PORTNUMBER, &hints, &ctx->servinfo) != 0){
		syslog(LOG_ERR,"getaddrinfo error: ");
		return -1;
	}

	*ctx->socfd = socket(ctx->servinfo->ai_family, ctx->servinfo->ai_socktype, ctx->servinfo->ai_protocol);
	if (*ctx->socfd == -1){
		freeaddrinfo(ctx->servinfo);
		ctx->servinfo = NULL;
		syslog(LOG_ERR,"Error creating socket");
		return -1;
	}

	int opt = 1;
    if (setsockopt(*ctx->socfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        syslog(LOG_ERR,"Could not set socket options");
        exit(EXIT_FAILURE);
    }

	if (bind(*ctx->socfd, ctx->servinfo->ai_addr, ctx->servinfo->ai_addrlen) == -1){
		syslog(LOG_ERR,"bind");
		return -1;
	}
	syslog(LOG_DEBUG, "Bind Socket");
	free(ctx->servinfo);
	ctx->servinfo = NULL;
	// listen
	if (listen(*ctx->socfd, 1) == -1){
		syslog(LOG_ERR,"Error to listen socket");
		return -1;
	}
	return 0;
}

int closeconnection(Context *ctx)
{
	if (ctx->acceptfd != NULL){
		if (shutdown(*ctx->acceptfd, SHUT_RDWR) < 0){
			syslog(LOG_ERR,"Shutdown socket error.");
		}
		close(*ctx->acceptfd);
	}

	if (ctx->socfd != NULL){
		close(*ctx->socfd);
	}

	if (ctx->istream != NULL){
		fclose(ctx->istream);
	}

	if (ctx->ostream != NULL){
		fclose(ctx->ostream);
	}

	if (ctx->hints != NULL){
		//free(ctx->hints);
	}
	if (ctx->servinfo != NULL){
		free(ctx->servinfo);
	}
	if (ctx->their_addr != NULL){

		// free(ctx->their_addr);
	}
	if (ctx->backstream_buffer != NULL){
		free(ctx->backstream_buffer);
	}
	syslog(LOG_DEBUG, "Deleting file");
	deleteFile();
	return 0;
}

int deleteFile(){
		if (access(FILENAME, F_OK) != -1){
		if (remove(FILENAME)){
			syslog(LOG_ERR,"Unable to delete file.");
			return -1;
		}
	}
	return 0;
}

int setupsigaction(){
	struct sigaction sa;
	sa.sa_handler = sigetrm_handler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1){
		syslog(LOG_ERR,"Error installing signal handler");
		return -1;
	}
	if (sigaction(SIGTERM, &sa, NULL) == -1){
		syslog(LOG_ERR,"Error installing signal handler");
		return -1;
	}
	return 0;
}

void start_daemon(){
	pid_t daemon_pid;
	daemon_pid = fork();
	if (daemon_pid < 0){
		syslog(LOG_ERR,"Unable to fork");
		exit(EXIT_FAILURE);
	}
	if(daemon_pid > 0){
		exit(EXIT_SUCCESS);
	}

	if(setsid() < 0){
		exit(EXIT_FAILURE);
	}

	umask(0);
	chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
	syslog(LOG_DEBUG, "Started daemon");

}

int main( int argc, char *argv[] ){

	int daemon = 0;
	openlog("Assignment 5-1", LOG_PID, LOG_LOCAL1);
	syslog(LOG_DEBUG, "Starting application");
	int opt;

	if (setupsigaction() == -1){
		syslog(LOG_ERR,"Error setup sig action");
		exit(EXIT_FAILURE);
	}
	
	while((opt=getopt(argc, argv, "d")) != -1 ){
		if (opt == 'd'){
			daemon = 1;
		}
	}

	pid_t child_pid;
	socklen_t addr_size;
	struct sockaddr_storage their_addr;
	//int child_status;
	int socfd, acceptfd;
	Context *ctx = &global_ctx;

	ctx->socfd = &socfd;
	ctx->acceptfd = &acceptfd;
	if (open_connection(ctx) == -1){
		syslog(LOG_ERR,"Unable to establish connection");
		closeconnection(ctx);
		return -1;
	}

	if(daemon){
		start_daemon();
	}

	setuptimer();

	while (1){
		addr_size = sizeof their_addr;
		acceptfd = accept(*ctx->socfd, (struct sockaddr *)&their_addr, &addr_size);
		if (acceptfd == -1){
			syslog(LOG_ERR,"Eroor accepting connection");
			continue;
		}
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), ctx->s, sizeof ctx->s);
		syslog(LOG_DEBUG, "Accepted connection from %s", ctx->s);
		
		if ((child_pid = fork()) < 0){
			syslog(LOG_ERR,"Could not create fork.");
			fclose(ctx->istream);
			close(acceptfd);
			continue;
		}
		if (child_pid == 0){
			// add thread handler 
			ctx->ostream = fopen(FILENAME, "a+");
			pthread_t thread_id;
			Textreciveargs_t  *thread_args = malloc(sizeof(Textreciveargs_t));
			thread_args->istream = ctx->acceptfd;
			thread_args->ostream = ctx->ostream;
			if( pthread_create(&thread_id, NULL, recieve_text,(void*) thread_args) < 0) {
				syslog(LOG_ERR,"Could not create a thread");
				free(thread_args);
				close(*ctx->acceptfd);
				ctx->istream = NULL;
				fclose(ctx->ostream);
				ctx->ostream = NULL;
				exit(EXIT_FAILURE);
			}
			pthread_join(thread_id,NULL);
			syslog(LOG_DEBUG, "Closed connection from %s", ctx->s);
			close(*ctx->acceptfd);
			ctx->istream = NULL;
			fclose(ctx->ostream);
			ctx->ostream = NULL;
			exit(EXIT_SUCCESS);
		} else {
			/*
			if (waitpid(child_pid, &child_status, 0) == -1){
				syslog(LOG_ERR,"Waitpid failed");
			}
			
			if (WIFEXITED(child_status)){
				//syslog(LOG_DEBUG, "Closed connection from %s", ctx->s);
			}*/
			close(acceptfd);
		}
	}
	close(acceptfd);
	close(socfd);
	return 0;
}
