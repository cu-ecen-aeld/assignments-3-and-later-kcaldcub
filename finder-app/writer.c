
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<syslog.h>

int main(int argc, char *argv[]){
	char *writefile,*writestr;
	int w,no;
	
	openlog("writerApp", 0, LOG_USER);

	if(argc!=3){
		syslog(LOG_ERR, "Arguments error");
		printf("Arguments error\n");
		closelog();
		exit(1);
	}
	else{
		writefile=argv[1];
		writestr=argv[2];
		syslog(LOG_DEBUG, "Writing %s to %s\n", writefile, writestr);
	}

	no=open( writefile, O_CREAT|O_WRONLY|O_TRUNC, 0666);
	if(no<0){
		perror("open file");
		syslog(LOG_ERR, "open error");
		closelog();
		exit(1);
	}
	else{
		int w = write( no, writestr, strlen(writestr));
		if(w<0){
			perror("write to file");
			syslog(LOG_ERR, "writ error");
			close(no);
			closelog();
			exit(1);
		}
	}

	close(no);
	closelog();
	exit(0);
}


