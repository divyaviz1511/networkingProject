/*
Author: Vasudha Gangireddy
Description: This program simulates the Communication Switch Process 
of the Fast Ethernet.
Date: Dec-9-2017 
*/

#include "fastethernet.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>
#define asize 15

struct cliaddr
{
		int descriptor;
		int station_num;
};

int port_number; //used to store server port number entered by the user
int maxi, maxfd, listenfd, connfd, sockfd;
int frm_no,srcaddr,destaddr,i;
char data[MAX];

FILE *output_file; // File pointer for server output file.

//Data frame queue and request frame queue.
struct frame data_frame[asize];
struct frame requ_frame[asize];
struct cliaddr cliaddress[asize];
int rfcnt, dfcnt;
char rbuff[MAXLINE];
char write_buff[MAXLINE];

/**
Handles error in code with an appropraite error message and exit code
**/
void errHandler(const char *message,int exitCode) 
{
	perror(message);
	exit(exitCode);
}

/*processes the requests from clients*/
void processRequest(char *rbuff, int i)
{
	if((strcmp(data,"Request") == 0))
	{				
		if(dfcnt < asize)
		{
			strcpy(data, "Accept");
			sprintf(write_buff, "%d %d %d %s \n", frm_no, srcaddr, destaddr, data);
							
			//write log to file
			fprintf(output_file,"\nSent positive reply to SP%d\n",srcaddr);
		}
		else
		{
			if(rfcnt < asize)
			{
				//add request frame to queue.
				requ_frame[rfcnt].seqno = frm_no;
				requ_frame[rfcnt].sadd = srcaddr;
				requ_frame[rfcnt].dadd = destaddr;
				strcpy(requ_frame[rfcnt].data,data);
								
				rfcnt++;
			}
			else
			{
				strcpy(data, "Reject");
				sprintf(write_buff, "%d %d %d %s \n", frm_no, srcaddr, destaddr, data);
								
				//write log to file
				fprintf(output_file,"\nSent reject to SP%d\n",srcaddr);
			}
							
		}
	}
}

/**Main function**/
int main(int argc, char * argv[]) 
{
	socklen_t client_length;
	struct sockaddr_in serv_addr, cli_addr;
	int nready,client[FD_SETSIZE]; 
	fd_set rset, allset;
	int i, n, j;
	
	
	//Checking user input.
    if (argc != 2) 
	{
		printf("\n INVALID format :Enter as: ./CSP <SERVER PORT NUMBER> \n");
		exit(0);
	}
	port_number = atoi(argv[1]);//port number from user
	//opening output file in write mode
	output_file = fopen("OUTPUT_CSP.txt", "w");
					

    if(output_file == NULL)
    {
		errHandler("ERROR opening the output file.",-1);
    }

	//Creating an internet stream TCP socket.
	if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		errHandler("Error opening server socket",1);
	}
	
	int enable = 1;
	
	//setting the SO_REUSEADDR socket option before calling bind function.
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) 
	{
		errHandler("ERROR setting setsockopt",1);
	}
	
	
	//sets the entire structure to zero.
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	//port_number is entered by user
	serv_addr.sin_port = htons(port_number);
	
	//Binding the socket.
	if (bind(listenfd, (SA *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		errHandler("ERROR on binding",1);
	}
	
	
	/*the socket is converted to a listening socket with a queue of 10
     where the incoming connections will be accepted by the kernel.*/
	listen(listenfd, BACKLOG);

	maxfd = listenfd; //initialize
	maxi = -1;  /*index into client[] array*/
	
	for(i = 0; i < FD_SETSIZE; i++)
		client[i] = -1; /*-1 indicates available entry */
	
	
	for(i = 0; i < asize; i++)
	{
		cliaddress[i].descriptor = -1; /*-1 indicates available entry*/
		cliaddress[i].station_num = -1;
	}
	
	
	/*Initializing the data frame queue and request frame queue*/
	for(i=0; i < asize; i++)
	{
		data_frame[i].seqno = -1;
		data_frame[i].sadd = -1;
		data_frame[i].dadd = -1;
		strcpy(data_frame[i].data,"");
		
		requ_frame[i].seqno = -1;
		requ_frame[i].sadd = -1;
		requ_frame[i].dadd = -1;
		strcpy(requ_frame[i].data,"");
	}
	
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	
	//Initializes the file descriptor set to contain no file decriptors.
	FD_ZERO(&rset);
	FD_SET(listenfd, &allset); //turning listenfd bit ON.
	
	rfcnt = 0;
	dfcnt = 0;
	
	for(;;)
	{
		rset = allset; // structure assignment
		
		//waiting for activities from station processes
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		
		//new client connection
		if(FD_ISSET(listenfd, &rset))
		{
			//Length of client
			client_length = sizeof(cli_addr);
			
			connfd = accept(listenfd, (SA *) &cli_addr, &client_length); //accepting clients
			
			//placing the connection descriptor in client array.
			for(i=0; i < FD_SETSIZE; i++)
			{
				if(client[i] < 0)
				{
					client[i] = connfd;
					break;
				}
			}
			
			if(i == FD_SETSIZE)
			{
				//write log to file
				fprintf(output_file,"\n Cannot accept anymore connections\n");
			}
				
			FD_SET(connfd, &allset); //add new client descriptor to set.
			if(connfd > maxfd)
				maxfd = connfd; //adjust the maxfd of select function.
				
			//maximum array index of client 
			if (i > maxi)
				maxi = i;
				
			//no more readable descriptors (no more new clients)
			if (--nready <= 0)
				continue;
		}
		
		//check all the clients for data.
		for(i=0; i <= maxi; i++) //maxi represents the last client in the client array.
		{
			if((sockfd = client[i]) < 0)
				continue; //No client. Connection terminated with client.
			
			//Is socket ready for read.
			if(FD_ISSET(sockfd, &rset))
			{
				if(( n = read(sockfd, rbuff, MAXLINE)) == 0)
				{
					/* Connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					
					/*Clearing the corresponding client addresses*/
					cliaddress[i].descriptor = -1;  
					cliaddress[i].station_num = -1;
				}
				else
				{
					//reading  the frame
					sscanf(rbuff, "%d %d %d %s", &frm_no,&srcaddr,&destaddr,data);
					
					if((strcmp(data,"Request") == 0))
					{
						/*Map the client address with the corresponding descriptor*/
						if(cliaddress[i].descriptor < 0)/*only if the descriptor is empty*/
						{
							cliaddress[i].descriptor = client[i];
							cliaddress[i].station_num = srcaddr;
						}
						
						//write log to file
						fprintf(output_file,"\nReceive request from SP%d\n",srcaddr);
						//fflush(output_file);
						processRequest(rbuff, i);
						
						if((write(sockfd, write_buff, strlen(write_buff))) < 0)
						{
							errHandler("ERROR writing to socket",2);
						}
						fprintf(output_file,"\nSent %s for Request Frame %d from SP%d  to SP%d\n", data, frm_no, srcaddr, destaddr);
						fflush(output_file);
					}
					else
					{
						//add data frame to queue.
						data_frame[dfcnt].seqno = frm_no;
						data_frame[dfcnt].sadd = srcaddr;
						data_frame[dfcnt].dadd = destaddr;
						strcpy(data_frame[dfcnt].data,data);
								
						dfcnt++;
					}					
				}
				
				if(--nready <= 0)
					break;
			}
		}
		
		/*Reading from queues and processing the frames accordingly*/
		if(nready <= 0)
		{
			/*Read from the dataframe and send it to the destination*/
			for(j=0; j < asize; j++)
			{
				if(data_frame[j].sadd > 0) //checking if frame present.
				{
					for(i = 0; i < maxi+1; i++) /*iterating through cliaddress to get the descriptor details*/
					{
						if(cliaddress[i].station_num = data_frame[j].dadd)
						{
							/*Sending data frame to correcponding SP*/
							sprintf(write_buff, "%d %d %d %s \n", data_frame[j].seqno, data_frame[j].sadd, data_frame[j].dadd, data_frame[j].data);
							if((write(cliaddress[i].descriptor, write_buff, strlen(write_buff))) < 0)
							{
								errHandler("ERROR writing to socket",3);
							}
							
							//write log to file
							fprintf(output_file,"\nForward data frame from SP%d to SP%d \n",data_frame[j].sadd, data_frame[j].dadd);
							fflush(output_file);
							/*Delete frame from data frame*/
							//data_frame[j].seqno = -1;
							//data_frame[j].sadd = -1;
							//data_frame[j].dadd = -1;
							//strcpy(data_frame[j].data,NULL);
						}
					}
				}
			}
			
			/*Read from the request frame and send appropriate reply*/
			for(j = 0; j < asize; j++)
			{
				if(requ_frame[j].sadd > 0) //request frame present?
				{
					sprintf(write_buff, "%d %d %d %s \n", requ_frame[j].seqno, requ_frame[j].sadd, requ_frame[j].dadd, requ_frame[j].data);
					sscanf(write_buff, "%d %d %d %s", &frm_no,&srcaddr,&destaddr,data);
					processRequest(write_buff, j);
					
					/*Delete frame from request frame*/
					//requ_frame[j].seqno = -1;
					//requ_frame[j].sadd = -1;
					//requ_frame[j].dadd = -1;
					//strcpy(requ_frame[j].data,NULL);
				}
			}
		}
		else
			break;
	}
	printf("The end");
	fclose(output_file);
}


