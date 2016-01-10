//#define DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define FILENAMELEN 100	
#define BUFSIZE 4096
#define log_deb(X) log_debug(__FILE__, __LINE__, X);
void log_debug(char *f, int n, char *s)
{
#ifdef DEBUG
	printf("%s:%d::%s\n",f,n,s);
#endif
}



#define STDIN 0 // file descriptor for standard input
//#define INET6_ADDRSTRLEN 256

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void){
  char buf[BUFSIZE];
  char str[INET6_ADDRSTRLEN];
  int test=0;
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  int s=socket(AF_INET,SOCK_STREAM,0);
  struct addrinfo my_SD,hints, *res,*servinfo,*p;
  int n =0;
  int r= 0 ;
  char temp[BUFSIZE];
  int sel;
  FILE * fp;
  char * pch;
  char filename[FILENAMELEN];
  char appender[BUFSIZE*2];
  char fformat[FILENAMELEN];
  int fd = -1;
  char* map;

  struct timeval tv;
  fd_set readfds;
  tv.tv_sec = 2000;
  tv.tv_usec = 50000;

  FD_ZERO(&readfds);
  FD_SET(STDIN, &readfds);
  FD_SET(s,&readfds);

  memset(&hints,0,sizeof(hints));
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;
  getaddrinfo("127.0.0.1","9034",&hints,&res);
  int conect = 1;
  printf("Type /server to connect!! \n");
  while(conect){
	fgets(buf,BUFSIZE-1,stdin);
	if(strstr(buf,"/server") != NULL)
		conect=0;
	}
        
	if(connect( s,res->ai_addr,res->ai_addrlen) == -1 ){
		perror("connect");
		exit(1);
	}
        printf("Connected!! \n");

	while(1){
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(s, &readfds);
		bzero(buf,250);
		log_deb("Before select\n");
		sel=select(s+1, &readfds, NULL, NULL, &tv);
		if(n= FD_ISSET(0, &readfds)) {
			  log_deb ("handling stdin\n");
			  //send
			  fgets(buf,BUFSIZE-1,stdin);
			  if(strstr(buf,"/upload") != NULL)
			  {
				  //uploading file
				printf("file upload \n");
				pch = strtok(buf," ");
	
				strcat(appender,pch);		
				strcat(appender," ");	
	
				pch = strtok(NULL," ");	
				strcpy(filename, pch);	
	
				strcat(appender,pch);
				strcat(appender," ");	
	
				pch = strtok(NULL,"+");	
				strcpy(fformat, pch);
	
				strcat(appender,pch);
				strcat(appender,"+");	
				fd = open(filename, O_RDWR, 0);
				if(fd == -1)
				{				
					 printf("open file failed!!\n"); //
					
					 break;
				}
				map = (char*)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
				if (map == MAP_FAILED) 
				{
        				printf("map failed!!\n");
					
					break;
				}
				
				strcat(appender, map);
				appender[BUFSIZE]='0';
	
				
  				munmap(map, 4096);
  				close(fd);
				
				test=write(s, appender, strlen(appender));
					
			}else {
			//sending msg / download / ls / nick / who

			  test= send(s , buf ,strlen(buf) , 0);
			}
		}//bezro(char*,size);
		if(FD_ISSET(s, &readfds)) {                      
			log_deb("handling s\n");
				buf[0]='\n';
			if(r = recv(s , buf , BUFSIZE , 0) < 0) {
				printf("recv failed!!\n");
				break;
			} else  if(strstr(buf,"/download") != NULL)
			   {	//download file
				int i,j;
				filename[0]='\0';
				for(i=0; buf[i]!='+'; i++);
				for(i=i+1,j=0; i<strlen(buf); i++,j++) { // copying the file content
					temp[j]=buf[i];
				}
	
				pch = strtok(buf," ");
				pch = strtok(NULL,"+");	 
				strcat(filename, pch);	
	
				fp = fopen(filename, "w");
				if(fp == NULL) {
					printf("fp fopen failed\n");
					
					break;
				}
	
				fprintf(fp,"%s",temp);
				fclose(fp);
				printf("file downloaded!! \n");

			  }else{
			
				buf[256]='\0';
				printf("%s\n",buf);
			}
		}
		if(sel < 0) {
			printf("Timed out.\n");
			return 0;
		}
	}
	close(s);
}
