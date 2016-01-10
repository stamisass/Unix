#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define FILENAMELEN 100
#define BUFSIZE 4096
#define PORT "9034"   // port we're listening on

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    const char* NoFile = "No such file !!";
    char filename[FILENAMELEN];
    char temp[4096];
    char fformat[100];
    FILE * fp;
    FILE * f_list;
    char flist[300];
    char c;
    int is_com = 0;
    int test=0;
    char Nickname[256][10];// array of strings
    char* pch;
    char who[300];
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    char appender[BUFSIZE*2]="/download ";
    int fd = -1;
    char* map;
    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[4096];    // buffer for client data
    char buftemp[300];
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
 
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
    // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        is_com = 0;
        read_fds = master; // copy it
        if (test=select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
	    {
        	//   printf("select=%d",test);
            	perror("select");
            	exit(4);
            }
        // printf("select=%d",test);
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) 
	    {
            	if (FD_ISSET(i, &read_fds)) 
			{
	   			// we got one!!
                		if (i == listener) 
		    		   {
                    			// handle new connections
                    			addrlen = sizeof remoteaddr;
            				newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);
					//	strcpy(Nickname[newfd-4],"client");//nickname
					//	strcat(Nickname[newfd-4],newfd);
		

                    			if (newfd == -1)
			 		  {
            		       			perror("accept");
                    			} else {
         	        			snprintf(Nickname[newfd-4],10,"client%d",newfd-4);
						strcat(who,Nickname[newfd-4]);
						strcat(who," , ");
                       
                       				FD_SET(newfd, &master); // add to master set
                        			if (newfd > fdmax) 
						{  
						  	// keep track of the max
		                                  	fdmax = newfd;
                        			}
                        		printf("selectserver: new connection from %s on socket %d\n",inet_ntop(remoteaddr.ss_family,
                                	get_in_addr((struct sockaddr*)&remoteaddr),
                                	remoteIP, INET6_ADDRSTRLEN),
                            		newfd-4);
                    	       		}
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
			    
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
			if(strstr(buf , "/") != NULL)
			{
				//it's a command 
				if(strstr(buf,"/who") != NULL)
				  {
					 // "who" command
					printf("/who \n");
					is_com = 1;
				  	send(i, who, sizeof who, 0);
					buf[0]='\0';
				  	break;	
				  }
				if(strstr(buf,"/nick") != NULL)
				  { 
					// change nickname
					printf("/nick \n");
					is_com = 1;
					pch= strtok(buf," ");
					pch= strtok(NULL,"\n");
					if(pch != NULL)
					    {
					   	strcpy(Nickname[i-4],pch);
						printf("Nickname : %s \n",Nickname[i-4]);
					    }
					
					who[0]= '\0';			
					for(j=0 ; j <= fdmax ; j++)
					   {
						strcat(who,Nickname[j]);
						strcat(who," , ");
					   }
					buf[0]='\0';
					//send "nickname changed!!"
					break;
				  }
				if(strstr(buf,"/ls") != NULL)
				  {  
					// "ls" command
					printf("/ls \n");
					is_com = 1;				
  					f_list = fopen("./file_list", "r");
					if(f_list == NULL) {
						printf("flist fopen failed\n");
						//send(i, NoFile, sizeof NoFile, 0);						
						break;
					}
					fd = open("./file_list", O_RDWR, 0);
					map = (char*)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
					if (map == MAP_FAILED) {
						printf("map failed\n");
						break;
					}
					
					strcpy(temp,map);
					temp[strlen(map)+1]='0';
					
					munmap(map, 4096);
					fclose(f_list);
				
					send(i, temp, sizeof temp, 0);
					break;

				  }
				if(strstr(buf,"/upload")!=NULL)
				  {   
					// upload file
					printf("/upload \n");
					is_com = 1;
					for(i=0; buf[i]!='+'; i++);
					for(i=i+1,j=0; i<strlen(buf); i++,j++) 
						{
							 // copying the file content
							temp[j]=buf[i];
						}
		
					
		
					pch = strtok(buf," ");
					pch = strtok(NULL," ");
					strcpy(filename, pch);
					pch = strtok(NULL,"+");	
					
					fp = fopen(filename, "w");
					if(fp == NULL) 
						{
							printf("fopen failed\n");
							break;
						}
		
					
					//fwrite(temp,sizeof temp,1,fp);
					fprintf(fp,"%s",temp);					
					fclose(fp);
		
					
					f_list = fopen("./file_list", "a");
					if(f_list == NULL) {
					printf("f_list fopen failed\n");
					break;
					}
		
					fprintf(f_list,"%s\n",filename);
					fclose(f_list);
	
					break;
					

     				  }
				if(strstr(buf,"/download") != NULL)
				  { 
					// dowmload file
					int nsent;
					is_com = 1;
					printf("/dowmload \n");
					is_com = 1;					
					
					pch = strtok(buf," ");
					pch = strtok(NULL,"\n");
					strcpy(filename, pch);
		
					strcat(appender,filename);
					strcat(appender,"+");
		
					fd = open(filename, O_RDWR, 0);
					if(fd == -1)
					{
						printf("open file failed");
						send(i, NoFile, sizeof NoFile, 0);
						break;
					}
					
					map = (char*)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
					if (map == MAP_FAILED) 
					{
        					printf("map failed\n");
					
						break;
					}
					
					strcat(appender,map);
		
					munmap(map, 4096);
	
					nsent=send(i, appender,(sizeof appender),0);
					appender[0]='\0';
					break;
					
				  }	
			}
		
			if(is_com == 0);
			   {
				printf("it's a message: \n");
                    		// we got some data from a client
                        	buf[nbytes]='\0';
				for(j = 0; j <= fdmax; j++)
				    {
                        		 // send to everyone!
                        		 if (FD_ISSET(j, &master)) 
					     {
                        			  // except the listener and ourselves
                              			  if (j != listener && j != i)
							 {
								int nsent;
								strcpy(buftemp,Nickname[i-4]);
								strcat(buftemp,"-> ");
								strcat(buftemp,buf);                  
								nbytes = strlen(buftemp);
								
								if ((nsent=send(j, buftemp, nbytes, 0)) == -1)
								    {
									perror("send");
								    }
            							
						  	 }
                            		    }
                        	   }
				break;
                    	  }
		  }
		
              } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}
