#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdbool.h>

#define MAX 225
#define MAXLINE 300
#define BACKLOG 10
#define SA struct sockaddr

#define true 1
#define false 0

struct frame
{
		int seqno;
		int sadd;
		int dadd;
		char data[MAX];		
};



