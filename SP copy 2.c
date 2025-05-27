/*
Author: Vasudha Gangireddy
Descrition: This program simulates the Station Process 
of the Fast Ethernet.
Date: Dec-9-2017 
*/

#include "fastethernet.h"
#include <ctype.h>
#include <arpa/inet.h>

FILE *input_file;
FILE *output_file;
FILE *data_file;

int portno, station_num,sockfd, filefd;
struct sockaddr_in servaddr;
fd_set   rset;
int maxfdp1, nready;


char sim_input[100];

char data[MAX];
char reply[MAX];
char s1[10], s2[10], s3[10], s4[10];
int frm_no, destaddr; 
char write_buff[MAX];
char server_reply[MAX];

int count = 0;
int frmRecived = 0;
bool torf;

void sendDFrame(char *rdbuff);

//exits when an error occurs.
void errHandle(const char *msg)
{
	perror(msg);
	exit(0);
}

//Send request frame to server.
void sendRFrame(char *rbuff)
{
	//read the input line
	sscanf(rbuff, "%s %d %s %s %s %d", s1, &frm_no, s2, s3, s4, &destaddr);
		
	//write into the buffer
	strcpy(data,"Request");
	sprintf(write_buff, "%d %d %d %s \n", frm_no, station_num, destaddr, data);	
		
	//sending request frame to server
	if((write(sockfd, write_buff, strlen(write_buff))) < 0)
	{
		errHandle("ERROR writing to socket");
	}
	count++; //number of rejects counter.
	//write log to file
	fprintf(output_file,"\nSend request to CSP to send data frame %d to SP %d\n",frm_no,destaddr);
	//sendDFrame(rbuff);
}

//read server reply and send Data frame
void sendDFrame(char *rdbuff)
{
		int r1, r2, r3;
		
		//read the server reply
		sscanf(server_reply, "%d %d %d %s", &r1,&r2,&r3,reply);
		
		//checking the server reply
		if(strcmp(reply,"Accept") == 0)
		{
			//Reading data from a file.
			data_file = fopen("DataFile.txt" , "r");
	
			bzero(data,MAX);
			if(data_file == NULL) 
			{
				//write log to file
				fprintf(output_file,"\nError opening data file\n");
				
				//data = "No data in file, so sending this data.";
				strncpy(data,"No data in file, so sending this data.", MAX);
			}
			else
			{
				//getting data from file into buffer
				fgets(data,MAX,data_file);
			}	
		
			//write data into the buffer
			sprintf(write_buff, "%d %d %d %s \n", frm_no, station_num, destaddr, data);	
			
			//send data frame to server.
			if (write(sockfd, write_buff, strlen(write_buff)) < 0)
			{
				errHandle("ERROR writing to socket");
			}
			//write log to file
			fprintf(output_file,"\nSend data frame %d to SP %d \n",frm_no,station_num);
			fclose(data_file);
		}
		else
		{
			if((strcmp(reply,"Reject") == 0))
			{
				if(count < 3)//checking if send request rejected thrice.
				{
					count++;
					fprintf(output_file,"\nReceive reject reply from CSP to send data frame %d to SP %d\n",frm_no,destaddr);
					sendRFrame(sim_input);//resend request.
					
				}
				else
				{
					count = 0;
					fprintf(output_file,"\nServer busy; Unable to send frame to server.\n");
					errHandle("Server busy; Unable to send frame to server.\n");
				}
			}
		}
		
		if( r3 == station_num)
		{
			fprintf(output_file,"\nReceive a data frame from SP %d\n",r2);
		}
return;
}

//wait for receiving frames
bool waitFrames(char *rbuff)
{
	int nframe;
	//read the input line
	sscanf(rbuff, "%s %s %s %d %s", s1, s2, s3, &nframe, s4);
	
	if (frmRecived >= nframe)
	{
		frmRecived = 0; //making receievd farmes zero for next wait.
		return 1;
	}
	else
	{
		printf("\n Please wait until you receive %d frames from server before transmitting.\n", nframe);
		return 0;
	}	
}

//determines the frame type based on input data
void processInput(char input_line[])
{
	if(strstr(input_line, "SP") != "\0") 
	{
		sendRFrame(input_line); //type of frame is Request
	}
	else 
	{
		if(strstr(input_line, "Wait") != "\0")
		{
			torf = waitFrames(input_line);
		}
	}
		
}

int main(int argc, char *argv[])
{
	//check arguments to start the station process
	if (argc != 4) 
	{
		printf("\n INVALID format: Enter as: ./SP <server name> <port> <station number>\n");
		exit(0);
	}
	
	//open file and write output to file.
    output_file = fopen("SP_OUTPUT.txt" , "w");
	
	if(output_file == NULL) 
	{
		perror("Error opening output file");
		return(-1);
    }
	
	//hostname stores the host address.
	char * hostname = argv[1];
	
	//portno stores the port number given by the user.
	portno = atoi(argv[2]);
	
	//station number entered by the user 
	station_num = atoi(argv[3]);
	
	
	//open the input file based on the station number
    switch(station_num)
	{
     case 1: 
           input_file  = fopen("SP1.txt" , "r"); 
		   break;
     case 2:
           input_file  = fopen("SP2.txt" , "r");    
           break;
     case 3:
           input_file  = fopen("SP3.txt" , "r");    
           break;
     case 4:
           input_file  = fopen("SP4.txt" , "r");    
           break;
     case 5:
           input_file  = fopen("SP5.txt" , "r");    
           break;
     case 6:
           input_file  = fopen("SP6.txt" , "r");    
           break;
     case 7:
           input_file  = fopen("SP7.txt" , "r");    
           break;
     case 8:
           input_file  = fopen("SP8.txt" , "r");    
           break;
     case 9:
           input_file  = fopen("SP9.txt" , "r");    
           break;
     case 10:                      
           input_file  = fopen("SP10.txt" , "r");    
           break;
    }
	
	//check if input file is opened without any errors.
    if(input_file == NULL) 
	{
		perror("Error opening input file");
		return(-1);
    }
	
	//creating client socket descriptor --- IPPROTO_TCP : to specify TCP is being used
	if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		errHandle("Error opening client socket");
	 
	 
	//sets all the values to zero.
	bzero(&servaddr, sizeof(servaddr));
	
	//Fill in the server address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(portno);
	
	
	//Connecting to server
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
	{
		
		errHandle("ERROR in connecting 12345");
    }
	 
	//read from input file
	
	FD_ZERO(&rset);
	
	for(;;)
	{
		FD_SET(fileno(input_file), &rset);    //file descriptor
		FD_SET(sockfd, &rset);                //socket descriptor
		
		maxfdp1 = sockfd + 1;
		
		nready = select(maxfdp1 + 1, &rset, NULL, NULL, NULL);
		
		if(FD_ISSET(fileno(input_file), &rset)) /* File is readable*/
		{
			bzero(sim_input,100);
			if(feof(input_file))
				return -1;
			if(fgets(sim_input,100,input_file) != NULL)
			{
				processInput(sim_input);
			}
		}
		
		if(FD_ISSET(sockfd, &rset))
		{
			bzero(server_reply, MAX);
		
			//read reply from server using socket descriptor.
			if ((read(sockfd, server_reply, MAX)) < 0)
			{
				errHandle("ERROR reading from socket");
			}
			
			frmRecived++; //incrementing the frame recieved counter.
			
			sendDFrame(server_reply);
		}
		
	}
	 /* close the socket */
    close(sockfd);
	fclose(input_file);
	fclose(output_file);
	return 0;
}



