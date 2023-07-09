#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h> 
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <pthread.h>

#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 65536 // max number of bytes we can get at once 
#define MY_LOG_ERROR LOG_DEBUG//LOG_ERR


char file_name[]="/var/tmp/aesdsocketdata";
int sockfd, fd; 
pthread_mutex_t mutex;

struct thread_data{
     pthread_t thread;
     int new_fd;
     char s[INET6_ADDRSTRLEN];
     int thread_result;
};

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    char *buf = malloc(sizeof(char)*MAXDATASIZE);
    int numbytes=0;
    ssize_t nr;
        
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
    do
    {
	    int data_rx;
	    if ((data_rx = recv(thread_func_args->new_fd, (buf + numbytes), MAXDATASIZE-1, 0)) == -1) {
	      syslog(MY_LOG_ERROR,"%s:: recv error, numbytes:%d",__FILE__,numbytes);
	      close(thread_func_args->new_fd);
	      thread_func_args->thread_result = -1;
	      free(buf);
	      return thread_param;
	    }
	    numbytes += data_rx;
    } while(*(buf + numbytes-1) != '\n');
#ifdef PRINT_CONSOLE
    printf("numbytes:%d - last-char-ASCII:%d\n",numbytes,(int)buf[numbytes-1]);
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE

    if(pthread_mutex_lock(&mutex)!=0)
    	printf("Mutex lock failure \n");
      
    fd = open (file_name, O_RDWR | O_CREAT | O_APPEND,
               S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (fd == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: file not open", file_name);
      close(thread_func_args->new_fd);
      thread_func_args->thread_result = -1;
      free(buf);
      return thread_param;
    }
    
    /* write the string in 'buf' to 'fd' */
    nr = write (fd, buf, numbytes);
    if (nr == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: Unable to write to file", file_name);
      close(thread_func_args->new_fd);
      close(fd);
      thread_func_args->thread_result = -1;
      free(buf);
      return thread_param;
    }
    
    numbytes = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
#ifdef PRINT_CONSOLE
    printf("%s::%s has numbytes:%d",__FILE__,file_name,numbytes);
#endif
    if (read(fd, buf, numbytes) == -1)
    {
      syslog(MY_LOG_ERROR,"%s:: read error in %s with error %d",__FILE__, file_name,errno);
      close(thread_func_args->new_fd);
      close(fd);
      thread_func_args->thread_result = -1;
      free(buf);
      return thread_param;
    }
    
    if (send(thread_func_args->new_fd, buf, numbytes, 0) == -1){
      syslog(MY_LOG_ERROR,"%s:: send error",__FILE__);
      thread_func_args->thread_result = -1;
      free(buf);
      return thread_param;
    }
    
    if (close (fd) == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: file not closed", file_name);
      close(thread_func_args->new_fd);
      thread_func_args->thread_result = -1;
    }
    if (close(thread_func_args->new_fd) == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%d:: Connection not closed", thread_func_args->new_fd);
      thread_func_args->thread_result = -1;
    }
    syslog(LOG_DEBUG,"Closed connection from %s", thread_func_args->s);

    if(pthread_mutex_unlock(&mutex)!=0)
    	printf("Mutex lock failure \n");
    	
    free(buf);
    
    return thread_param;
}

void timer_signal_handler(int signum)
{
    if ( signum == SIGALRM ) {
        syslog(LOG_DEBUG,"%s::SIGALRM caught", __FILE__);
    }
}

void sig_int_and_term_handler(int signum)
{
  syslog(LOG_DEBUG,"%s:: Caught signal, exiting", __FILE__);
  
  pthread_mutex_destroy(&mutex);

  if (shutdown (sockfd, SHUT_RDWR) == -1){
    /* error */
    syslog(LOG_DEBUG,"%s::Unable to close sockfd or connection already closed",__FILE__);
  }

#if 0
  if (shutdown (new_fd, SHUT_RDWR) == -1){
    /* error */
    syslog(LOG_DEBUG,"%s::Unable to close or new_fd connection already closed",__FILE__);
  }
#endif

  if (close (fd) == -1){
    /* error */
    syslog(LOG_DEBUG,"%s::File not closed",__FILE__);
  }
  
  if (remove(file_name) == 0)
    syslog(LOG_DEBUG,"%s::%s Deleted successfully",__FILE__, file_name);
    
  exit(0);
}

void* timer_threadfunc(void* thread_param)
{
  char outstr[200];
  time_t t;
  struct tm *tmp;
  while(1)
  {
    sleep(10);
    t = time(NULL);
    tmp = localtime(&t);

    // "%y-%m-%d-%H-%M-%S"
    // Append timestamp
    if (strftime(outstr, sizeof(outstr), "timestamp:%a, %d %b %Y %T %z\n", tmp) == 0) {
      //
    }

    if(pthread_mutex_lock(&mutex)!=0)
      syslog(MY_LOG_ERROR,"%s:: Mutex lock failure", file_name);
      
    fd = open (file_name, O_RDWR | O_CREAT | O_APPEND,
               S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (fd == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: file not open", file_name);
      continue;
    }
    
    /* write the string in 'buf' to 'fd' */
    if(write (fd, outstr, strlen(outstr)) == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: Unable to write to file", file_name);
      close(fd);
      continue;
    }
    if (close (fd) == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: file not closed", file_name);
    }


    if(pthread_mutex_unlock(&mutex)!=0)
      syslog(MY_LOG_ERROR,"%s:: Mutex lock failure", file_name);
    //alarm(10); 
    //printf("%s:Waiting 10 seconds for alarm signal\n", (char *)thread_param);
    //pause();
    
  }
  return (void*) NULL;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
  int yes=1;  //char yes='1'; // Solaris people use this
  struct sockaddr_storage their_addr; // connector's address information
  struct addrinfo hints, *res;
  char s[INET6_ADDRSTRLEN];
  struct sigaction sigterm_sa;
  struct sigaction sigint_sa;
  pthread_t timer_thread;
  //struct sigaction timer_action;
  //bool success = true;
  
  //memset(&timer_action,0,sizeof(struct sigaction));
  //timer_action.sa_handler= timer_signal_handler;
  //sigaction(SIGALRM, &timer_action, NULL);
  
  memset(&sigint_sa, 0, sizeof(sigint_sa));
  sigint_sa.sa_handler = sig_int_and_term_handler;
  sigint_sa.sa_flags = 0;  
  // Mask other signals from interrupting SIGINT handler
  sigfillset(&sigint_sa.sa_mask);  
  // Register SIGINT handler
  sigaction(SIGINT, &sigint_sa, NULL);
  
  memset(&sigterm_sa, 0, sizeof(sigterm_sa));
  sigterm_sa.sa_handler = sig_int_and_term_handler;
  sigaction(SIGTERM, &sigterm_sa, NULL);
  
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    //printf("\n mutex init has failed\n");
    return -1;
  }                     
    
  // daemom
  if (argc == 2 && (strcmp(argv[1], "-d") == 0)) 
  {
    pid_t pid;
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
    //daemon (0, 0); // LSP Page 174
    /* create new process */
    pid = fork ( );
    if (pid == -1)
      return -1;
    else if (pid != 0)
      exit(0);
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
    /* create new session and process group */
    
    if (setsid ( ) == -1)
      return -1;
    /* set the working directory to the root directory */
    if (chdir ("/") == -1)
      return -1;
    /* close all open files--NR_OPEN is overkill, but works */
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE

    close (0);
    close (1);
    close (2);
    
    /* redirect fd's 0,1,2 to /dev/null */
    int fdNull = open("/dev/null", O_RDWR, 0666);
    if(fdNull<0){
    	printf("open() /dev/null failed\n");
    	exit(1);
    }
    printf("%s::%d\n",__FILE__,__LINE__);
    dup (0); 
    dup (0);  
  }
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
  
  // first, load up address structs with getaddrinfo():  
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  
  getaddrinfo(NULL, "9000", &hints, &res);
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
  
  // make a socket:  
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
  
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    syslog(MY_LOG_ERROR,"%s:: setsockopt error",__FILE__);
    return -1;
  }
  
  // bind it to the port we passed in to getaddrinfo(): 
  if(bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
    syslog(MY_LOG_ERROR,"%s:: bind error",__FILE__);
    close(sockfd);
    return -1;
  }

  if (listen(sockfd, BACKLOG) == -1) {
    syslog(MY_LOG_ERROR,"%s:: listen error",__FILE__);
    close(sockfd);
    return -1;
  }  
  freeaddrinfo(res); // all done with this structure
  
  pthread_create(&timer_thread,
                          NULL,
                          &timer_threadfunc,
                          "Timer thread"); 
  while(1)
  {
    socklen_t sin_size = sizeof their_addr;
    int new_fd, thread_s; 
    struct thread_data *data;
    
#ifdef PRINT_CONSOLE
    printf("Accepting new connection %d\n", sockfd);
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
    
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      syslog(MY_LOG_ERROR,"%s:: accept error %d",__FILE__, errno);
      close(sockfd);
      return -1;
    }
    inet_ntop(their_addr.ss_family,
    	get_in_addr((struct sockaddr *)&their_addr),
    	s, sizeof s);
    	
    syslog(LOG_DEBUG,"Accepted connection from %s", s);
    
    data = malloc(sizeof(struct thread_data));
    
    data->new_fd = new_fd;
    data->thread_result = 0;
    strcpy(data->s, s);
    
    thread_s = pthread_create(&(data->thread),
                          NULL,
                          &threadfunc,
                          data);
    data->thread_result = (thread_s==0?0:-1);
    pthread_join(data->thread, NULL);
    free(data);
                          
                   
  }
  return 0;
}

