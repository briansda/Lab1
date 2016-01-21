#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "Parse.h"

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         100
#define HOST_NAME_SIZE      255
#define MAX_GET             1000

int main(int argc, char *argv[])
{
	int hSocket;                 /* handle to socket */
	struct hostent *pHostInfo;   /* holds info about a machine */
	struct sockaddr_in Address;  /* Internet socket address stuct */
	long nHostAddress;
	char pBuffer[BUFFER_SIZE];
	unsigned nReadAmount;
	char strHostName[HOST_NAME_SIZE];
	int nHostPort;


	int c;
	int download_times = 1;
	int cFlag = 0;
	int dFlag = 0;
	int SuccessfulDownloads = 0;
	int FailedDownloads = 0;
	char *path;
	char contentType[MAX_MSG_SZ];
	vector<char *> headerLines;
	
	if (argc < 4 || argc > 6)
	{
		printf("\nUsage: download host port path [-c #ofdownloads | -d]\n");
		return 0;
	}
	else
	{
		while ((c = getopt(argc, argv, "c:d")) != -1)
		{
			switch (c)
			{
				case 'c':
					download_times = atoi(optarg);
					cFlag = 1;
					break;
				case 'd':
					dFlag = 1;
					break;
				default:
					perror("\nUsage: download host port path [-c #ofdownloads | -d]\n");
					return -1;
			}
		}
		//if there aren't 3 arguments after the option there is an error
		if (argc - optind != 3)
		{
			perror("\nUsage: download host port path [-c #ofdownload | -d]\n");
		}
		//if we are supposed to debug output 
		if (dFlag)
		{
			printf("download_times = %d, printHeaders = %d\n", download_times, dFlag);
		}
		//copy hostname from arugments
		strcpy(strHostName, argv[optind++]);
		//copy portname from arguments
		std::string port = argv[optind++];
		//make sure port is only digits and then copy
		if (port.find_first_not_of("0123456789") != string::npos)
		{
			perror("Port must only contain numbers.\n");
			return -1;
		}
		else
		{
			nHostPort = atoi(&port[0]);
		}
		
		//copy path from arugments
		path = argv[optind];
	}

	//until it has been downloaded the proper amount of times, repeat process
	while (FailedDownloads + SuccessfulDownloads < download_times)
	{
		if (dFlag)
		{
			printf("\nMaking a socket.\n");
		}

		/* make a socket */
		hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (hSocket == SOCKET_ERROR)
		{
			perror("\nCould not make a socket.\n");
			return -1;
		}

		/* get IP address from name */
		pHostInfo = gethostbyname(strHostName);
		if (pHostInfo == NULL)
		{
			perror("\nCouldn't connect to host, because of IP address.\n");
			return -1;
		}

		/* copy address into long */
		memcpy(&nHostAddress, pHostInfo->h_addr, pHostInfo->h_length);

		/* fill address struct */	
		Address.sin_addr.s_addr = nHostAddress;
		Address.sin_port = htons(nHostPort);
		Address.sin_family = AF_INET;

		if(dFlag)
		{
			printf("\nConnecting to host:%s",strHostName);
		}

		/* connect to host */
		if (connect(hSocket, (struct sockaddr *) &Address, sizeof(Address)) == SOCKET_ERROR)
		{
			perror("\nCould not connect to host, because of Socket error\n");
			return -1;
		}

		// Create HTTP Message
		char *message = (char *) malloc(MAX_GET);
		sprintf(message, "GET %s HTTP/1.1\r\nHost:%s:%d\r\n\r\n", path, strHostName, nHostPort);

		// Send Message
		if (dFlag)
		{
			printf("Request to server: %s\n", message);
		}
		write(hSocket, message, strlen(message));

		// Read the status line of response
		char *startLine = GetLine(hSocket);
		if (dFlag)
		{
			printf("Status: %s.\n\n", startLine);
		}
		char status[2];
		status[0] = startLine[strlen(startLine) - 2];
		status[1] = startLine[strlen(startLine) - 1];
		//if end of status line is Not Fou  "nd" then print error
		if (status[0] == 'n' && status[1] == 'd')	
		{
			perror("Couldn't reach address.");
			return -1;
		}

		// Read the header lines
		headerLines.clear();	
		GetHeaderLines(headerLines, hSocket, false);
		if (dFlag)
		{
			// Now print them out
			for (int i = 0; i < headerLines.size(); i++)
			{
				printf("%s\n", headerLines[i]);
				if (strstr(headerLines[i], "Content-Type"))		
				{
					sscanf(headerLines[i], "Content-Type: %s", contentType);
				}		
			}
			printf("\n=======================\n");
			printf("Headers are finished, now read the file.\n");
			printf("Content Type is %s\n", contentType);
			printf("=======================\n\n");
		}

		// Now read and print the rest of the file
		if (!cFlag)
		{
			int rval;
			try
			{
				while ((rval = read(hSocket, pBuffer, MAX_MSG_SZ)) > 0)
				{
					write(1, pBuffer, (unsigned int) rval);
				}
			}
			catch (...)
			{
				perror("Error while writing file body.");
				return -1;
			}
		}

		/* close socket */
		if (dFlag)
		{
			printf("\nClosing socket.\n");
		}
		if (close(hSocket) == SOCKET_ERROR)
		{
			perror("\nCould not close socket.\n");
			return -1;
		}
        
		// Check to see if status gave the OK signal. If so, it was succssful
		if (status[0] == 'O' && status[1] == 'K')
		{
			SuccessfulDownloads++;
		}
		else
		{
			FailedDownloads++;
		}
	}
	if (cFlag)
	printf("\nSucceeded in Downloading file %d times. Failed to Download %d times.\n", SuccessfulDownloads, FailedDownloads);
}
