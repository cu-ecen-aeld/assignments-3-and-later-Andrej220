#include "aesdsocket.linkedlist.h"
#include "aesd_ioctl.h"
#include <errno.h>

Context global_ctx;
int activethreads = 0;

SLIST_HEAD(element, Connections_t) head = SLIST_HEAD_INITIALIZER(head);


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t filemutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sendmutex = PTHREAD_MUTEX_INITIALIZER;

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

	struct Connections_t *params = (struct Connections_t*) t;
	syslog(LOG_DEBUG, "Accepted connection from %s", params->s);
	params->ostream = fopen(FILENAME, "a+");
	if ( params->ostream == NULL){
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

	FILE * sendfd = fopen(params->filename, "r");
	if ( sendfd == NULL ){
		syslog(LOG_ERR, "Can't open file for read");
	}

	pthread_mutex_lock(&mutex);
	do {
		bytes_recieved = recv(params->istream,buffer,MAXBUFFER,0);
		buffer[bytes_recieved] = '\0';

		struct aesd_seekto seekto;
        if (sscanf(buffer, "AESDCHAR_IOCSEEKTO:%u,%u", &seekto.write_cmd, &seekto.write_cmd_offset) == 2) {
            // Perform the ioctl seek command
            int device_fd = open(FILENAME, O_RDWR);
            if (device_fd == -1) {
                syslog(LOG_ERR, "Error opening device file");
                continue;
            }
            if (ioctl(device_fd, AESDCHAR_IOCSEEKTO, &seekto) == -1) {
                syslog(LOG_ERR, "ioctl AESDCHAR_IOCSEEKTO failed: %s", strerror(errno));
                close(device_fd);
                continue;
            }

            // Send the content of the device back to the client
            char read_buffer[MAXBUFFER];
            ssize_t bytes_read;
            while ((bytes_read = read(device_fd, read_buffer, MAXBUFFER)) > 0) {
                if (send(params->istream, read_buffer, bytes_read, 0) != bytes_read) {
                    syslog(LOG_ERR, "Error sending file back.");
                    break;
                }
            }
            close(device_fd);
        } else {

			savetofile(buffer,params->ostream);
			char * newline = strchr(buffer, '\n');
			if (newline != NULL){
				sendfile(sendfd, params->istream);
			}
		}
	}while(bytes_recieved > 0);
	pthread_mutex_unlock(&mutex);

	free(buffer);
	fclose(sendfd);
	close(params->istream);
	fclose(params->ostream);
	syslog(LOG_DEBUG, "Closed connection from %s", params->s);
	params->done = 1;
	activethreads --;
	//delElement(params->conn);
	free(params);
	return NULL;
}

int savetime( char * buffer){
	FILE * fd = fopen(FILENAME, "a+");
	if (fd == NULL){
		syslog(LOG_ERR, "Unable to open file, timestamp");
		return -1;
	}
	savetofile(buffer,fd);

	return 0;
}

int savetofile( char * buffer, FILE * ostream){
	int res = 0;
	pthread_mutex_lock(&filemutex);
	if (fputs(buffer, ostream) == EOF){
			pthread_mutex_unlock(&filemutex);
			syslog(LOG_ERR,"Error writing to output stream\n");
			return -1;
	}
	pthread_mutex_unlock(&filemutex);
	fflush(ostream);
	return res;
}

int sendfile(FILE * fd, int stream){
	int bytes_read;
	char * buffer = (char *)malloc(MAXBUFFER);
	while ((bytes_read = fread(buffer, 1, MAXBUFFER, fd))){
		pthread_mutex_lock(&sendmutex);
		if ((send(stream, buffer, bytes_read, 0)) != bytes_read){
			pthread_mutex_unlock(&sendmutex);
			syslog(LOG_ERR,"Error sending file back.");
			return -1;
		}
		pthread_mutex_unlock(&sendmutex);
	}
	free(buffer);
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
	if (ctx->socfd != NULL){
		close(*ctx->socfd);
	}

	if (ctx->servinfo != NULL){
		free(ctx->servinfo);
	}

	if (ctx->backstream_buffer != NULL){
		free(ctx->backstream_buffer);
	}

 
	if (SLIST_EMPTY(&head)){
	    struct Connections_t *current;
		SLIST_FOREACH(current,&head, entries){
			pthread_cancel(current->pid);
			pthread_join(current->pid, NULL);
			syslog(LOG_DEBUG, "closing pid");
			delElement(current);
		}	
	}
	//syslog(LOG_DEBUG, "Deleting file");
	//deleteFile();
	pthread_mutex_destroy(&filemutex);
	pthread_mutex_destroy(&sendmutex);
	pthread_mutex_destroy(&mutex);
	
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
	openlog(LOGFACILITY, LOG_PID, LOG_LOCAL1);
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
	int socfd;
	Context *ctx = &global_ctx;
	ctx->socfd = &socfd;
	if (open_connection(ctx) == -1){
		syslog(LOG_ERR,"Unable to establish connection");
		closeconnection(ctx);
		return -1;
	}

	if(daemon){
		start_daemon();
	}
	//setuptimer();
	SLIST_INIT(&head);

	while (1){
		socklen_t addr_size;
		struct sockaddr_storage their_addr;
		addr_size = sizeof their_addr;
		int acceptfd = accept(*ctx->socfd, (struct sockaddr *)&their_addr, &addr_size);
		if (acceptfd == -1){
			syslog(LOG_ERR,"Eroor accepting connection");
			continue;
		}
		
		// call thread
		pthread_t thread_id;
		struct Connections_t *newthread = newElement(0);
		newthread->istream = acceptfd;
		syslog(LOG_DEBUG, "accept FD: %d", acceptfd);
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), newthread->s, sizeof newthread->s);
		newthread->filename = fileName;

		if( pthread_create(&thread_id, NULL, recieve_text,(void*) newthread) < 0) {
			syslog(LOG_ERR,"Unable to create a thread");
			free(newthread);
			continue;
		}
		newthread->pid = thread_id;
		SLIST_INSERT_HEAD(&head,newthread,entries);
		check_completed_threads();
	}
	close(socfd);
	return 0;
}

struct Connections_t * newElement(){
	struct Connections_t * conn =  malloc(sizeof(struct Connections_t));
	if (conn == NULL){
		return NULL;
	}
	conn->done = 0;
	conn->pid = 0;
	return conn;
}

void delElement(struct Connections_t *conn){
	SLIST_REMOVE(&head,conn,Connections_t,entries);
	close(conn->istream);
	fclose(conn->ostream);
	free(conn);
}

void check_completed_threads(){
	struct Connections_t *current;
	if (SLIST_EMPTY(&head)){
		SLIST_FOREACH(current,&head, entries){
			if (current->done){
				pthread_join(current->pid, NULL);
				delElement(current);
			}
		}
	}	
}
void timer_handler(){
	syslog(LOG_DEBUG, "timer handler");
	char timestamp[128];
	time_t rawtime;
	struct  tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(timestamp, sizeof(timestamp), "timestamp:%Y %m %d %H\n", timeinfo);
	savetime(timestamp);
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
			pthread_join(timerthread_id, NULL);
		}
	}
}