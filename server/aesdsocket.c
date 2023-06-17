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


#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 65536 // max number of bytes we can get at once 
#define MY_LOG_ERROR LOG_DEBUG//LOG_ERR


char file_name[]="/var/tmp/aesdsocketdata";
int sockfd, new_fd, fd; 

void sig_int_and_term_handler(int signum)
{
  syslog(LOG_DEBUG,"Caught signal, exiting");

  if (shutdown (sockfd, SHUT_RDWR) == -1){
    /* error */
    syslog(LOG_DEBUG,"%s::Unable to close sockfd or connection already closed",__FILE__);
  }

  if (shutdown (new_fd, SHUT_RDWR) == -1){
    /* error */
    syslog(LOG_DEBUG,"%s::Unable to close or new_fd connection already closed",__FILE__);
  }
  
  if (close (fd) == -1){
    /* error */
    syslog(LOG_DEBUG,"%s::File not closed",__FILE__);
  }
  
  if (remove(file_name) == 0)
    syslog(LOG_DEBUG,"%s::%s Deleted successfully",__FILE__, file_name);
    
  exit(0);
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
  ssize_t nr;
  int yes=1;  //char yes='1'; // Solaris people use this
  struct sockaddr_storage their_addr; // connector's address information
  struct addrinfo hints, *res;
  char s[INET6_ADDRSTRLEN];
  struct sigaction sigterm_sa;
  struct sigaction sigint_sa;
  
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
  
  while(1)
  {
    char buf[MAXDATASIZE];
    int numbytes=0;
    socklen_t sin_size = sizeof their_addr;
    
#ifdef PRINT_CONSOLE
    printf("Accepting new connection %d\n", sockfd);
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
    
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      syslog(MY_LOG_ERROR,"%s:: accept error",__FILE__);
      close(sockfd);
      return -1;
    }
    inet_ntop(their_addr.ss_family,
    	get_in_addr((struct sockaddr *)&their_addr),
    	s, sizeof s);
    	
    syslog(LOG_DEBUG,"Accepted connection from %s", s);
    
#ifdef PRINT_CONSOLE
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
    do
    {
	    int data_rx;
	    if ((data_rx = recv(new_fd, &buf[numbytes], MAXDATASIZE-1, 0)) == -1) {
	      syslog(MY_LOG_ERROR,"%s:: recv error, numbytes:%d",__FILE__,numbytes);
	      close(new_fd);
	      close(sockfd);
	      return -1;
	    }
	    numbytes += data_rx;
    } while(buf[numbytes-1] != '\n');
#ifdef PRINT_CONSOLE
    printf("numbytes:%d - last-char-ASCII:%d\n",numbytes,(int)buf[numbytes-1]);
    printf("%s::%d\n",__FILE__,__LINE__);
#endif //PRINT_CONSOLE
      
    fd = open (file_name, O_RDWR | O_CREAT | O_APPEND,
               S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (fd == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: file not open", file_name);
      close(new_fd);
      close(sockfd);
      return -1;
    }
    
    /* write the string in 'buf' to 'fd' */
    nr = write (fd, buf, numbytes);
    if (nr == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: Unable to write to file", file_name);
      close(new_fd);
      close(sockfd);
      close(fd);
      return -1;
    }
    
    numbytes = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
#ifdef PRINT_CONSOLE
    printf("%s::%s has numbytes:%d",__FILE__,file_name,numbytes);
#endif
    if (read(fd, buf, numbytes) == -1)
    {
      syslog(MY_LOG_ERROR,"%s:: read error in %s with error %d",__FILE__, file_name,errno);
      close(new_fd);
      close(sockfd);
      close(fd);
      return -1;
    }
    //buf[numbytes] ='\0';
    
    if (send(new_fd, buf, numbytes, 0) == -1){
      syslog(MY_LOG_ERROR,"%s:: send error",__FILE__);
      close(new_fd);
      close(sockfd);
      close(fd);
      return -1;
    }
    
    if (close (fd) == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: file not closed", file_name);
      close(new_fd);
      return -1;
    }
    if (close(new_fd) == -1){
      /* error */
      syslog(MY_LOG_ERROR,"%s:: Connection not closed", s);
      return -1;
    }
    syslog(LOG_DEBUG,"Closed connection from %s", s);
  }
  return 0;
}

