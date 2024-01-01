#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#define SIZE 1000
#define TIME_TO_TIMEOUT 10

/* This allows for the http header to be made using a single function with switch statement */
enum Response_Type {
	TEXT,
	HTML,
	CSS,
	PNG,
	ICO
};

/* This will be start as 0 and be changed with the interupt signal */
int should_exit;

void handle_sigint(int sig){
	should_exit = 1;
	printf("The program will exit after at MOST 10 seconds\n");
}

/* Since most of the header of a http response is the same it all goes here 
 * @param clientID this the file descriptor for the client that the response will be sent to
 * @param http_type this is an enum and can have the value of TEXT for plain text or HTML or CSS
 * @param responseLength this is the length of the response in bytes 
 * @return returns the sum of the bytes that were sent or -1 for an error */
int http_response_header(int clientID, enum Response_Type http_type, int responseLength){
	int sumOfBytes, previousSum;
	char currentResponse[SIZE];
	sumOfBytes = 0;
	previousSum = 0;
	sprintf(currentResponse, "HTTP/1.1 200 OK\n");
	sumOfBytes += send(clientID, currentResponse, strlen(currentResponse), 0);
	if(sumOfBytes < previousSum){
		return -1;
	}
	previousSum = sumOfBytes;
	sprintf(currentResponse, "Content-Length: %d\n", responseLength);
	sumOfBytes += send(clientID, currentResponse, strlen(currentResponse), 0);
	if(sumOfBytes < previousSum){
		return -1;
	}
	previousSum = sumOfBytes;
	switch (http_type) {
		case TEXT:
			sprintf(currentResponse, "Content-Type: text/plain; charset=utf-8\n");
			break;
		case HTML:
			sprintf(currentResponse, "Content-Type: text/html; charset=utf-8\n");
			break;
		case CSS:
			sprintf(currentResponse, "Content-Type: text/css; charset=utf-8\n");
			break;
		case PNG:
			sprintf(currentResponse, "Content-Type: image/png\n"); 
		case ICO:
			sprintf(currentResponse, "Content-Type: image/vnd.microsoft.icon\n");
	}
	sumOfBytes += send(clientID, currentResponse, strlen(currentResponse), 0);
	if(sumOfBytes < previousSum){
		return -1;
	}
	previousSum = sumOfBytes;
	sumOfBytes += send(clientID, "\n", strlen("\n"), 0);
	if(sumOfBytes < previousSum){
		return -1;
	}
	return sumOfBytes;
}

/* returns bytes send or -1 if error uses the http_response_header function and then sends input text as content */
int http_response_text(int clientID, char response[], int responseLength){
	int sumOfBytes, previousSum;
	sumOfBytes = http_response_header(clientID, TEXT, responseLength);
	if(sumOfBytes == -1){
		return -1;
	}
	previousSum = sumOfBytes;
	sumOfBytes += send(clientID, response, responseLength, 0);
	if(sumOfBytes < previousSum){
		return -1;
	}
	return sumOfBytes;
}

/* This will send the file represented by the file descriptor fd to the client @ file descriptor clientID
 * @param clientID the file descriptor of the client as found by the accept function man 2 accept 
 * @param fd this is the file descriptor of the requested file to be sent to the client make sure that the http_type is 
 * correct for the file
 * @param http_type this is the type of content being sent, it is important that the correct Response_Type is selected
 * so that the http response will be correct, the Response_Type's are TEXT for plain text, HTML for html files and CSS for css files*/
int http_response_file(int clientID, int fd, enum Response_Type http_type){
	struct stat fileInfo;
	int sumOfBytes, previousSum;
	if(fstat(fd, &fileInfo) == -1){
		printf("Error with reading file stats\n");
		return -1;
	}
	sumOfBytes = http_response_header(clientID, http_type, fileInfo.st_size);
	if(sumOfBytes == -1){
		return -1;
	}
	previousSum = sumOfBytes;
	sumOfBytes += sendfile(clientID, fd, NULL, fileInfo.st_size);
	if(sumOfBytes < previousSum){
		return -1;
	}
	return sumOfBytes;
}

int http_response_html(int clientID, int htmlfd){
	return http_response_file(clientID, htmlfd, HTML);
}

int http_response_css(int clientID, int cssfd){
	return http_response_file(clientID, cssfd, CSS);
}

/* This is the way that it is so that multithreading using pthreads can be done
 * @param client_void_id the clientID as a void pointer it will be typecast as an int imediately
 * @return void* a void pointer in this case it is nothing so it does not matter
 */
void *handle_client(void *client_void_id){
	int startTime;
	/* This is the time in seconds till the client is timed out and the client connection is timed out */
	int clientID = *((int*)client_void_id);
	printf("inside of the handle_client function now\n");
	if(clientID == -1){
		printf("clientID is -1 so client does not exist or some connection error\n");
		return NULL;
	}
	printf("Sending data to the client now\n");
	startTime = time(NULL);
	while(1){
		char clientRequest[SIZE] = {0};
		/* This will be the type of the http request method see 
		 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
		 * for more on this and a list of all of them, since length will be at most 7
		 * this string is 8 characters long, 7 + 1 for \0 */
		char requestType[8];
		char fileRequest[100];
		int fd, messageAvalible;
		if(should_exit == 1){
			printf("Breaking the loop now\n");
			break;
		}
		else if(time(NULL) - startTime >= TIME_TO_TIMEOUT){
			printf("Request has timed out\n");
			break;
		}
		/* This is implemented this way so that the recv will be non blocking but the listening will still be blocking */
		messageAvalible = recv(clientID, clientRequest, SIZE, MSG_PEEK|MSG_DONTWAIT);
		/* recv returns message size so if recv return 0 or -1 there is no message */
		if(messageAvalible != -1 && messageAvalible != 0){
			recv(clientID, clientRequest, SIZE, 0);
		}
		else{
			continue;
		}
		if(should_exit == 1){
			printf("Breaking the loop now\n");
			break;
		}
		printf("Full request is:\n%s", clientRequest);
		/* Isolates the file from the GET resquest */
		sscanf(clientRequest, "%s %s", requestType, fileRequest);
		/* Makes sure that the request is a GET request as no other requests are supported yet */
		if(strcmp(requestType, "GET") != 0){
			printf("Resquest of %s, an unsupported action has occured\n", requestType);
			continue;
		}
		/* The famous / request is the first request that a browser will make and so needs to be answered */
		if(strcmp(fileRequest, "/") == 0){
			fd = open("index.html", O_RDONLY);
			http_response_html(clientID, fd);
		}
		else if(strcmp(fileRequest, "/favicon.ico") == 0){
			fd = open("favicon.ico", O_RDONLY);
			http_response_file(clientID, fd, ICO);
		}
		else{
			char *fileName;
			fileName = fileRequest + 1;
			fd = open(fileName, O_RDONLY);
			if(fd == -1){
				printf("Failed to get file due to files descriptor error\n");
			}
			else{
				long fileNameLength;
				fileNameLength = strlen(fileName);
				if(fileNameLength < 3){
					printf("File Name does not have an extention\n");
				}
				/* This tests if the last four characters are the given extention */
				else if(fileNameLength - ((long)strstr(fileName, ".css") - (long)fileName) == 4){
					printf("Serving CSS page\n");
					http_response_css(clientID, fd);
					printf("Finished serving CSS page\n");
				}
				else if(fileNameLength - ((long)strstr(fileName, ".html") - (long)fileName) == 5){
					printf("Serving html page\n");
					http_response_html(clientID, fd);
					printf("Finished serving HTML page\n");
				}
				else{
					printf("Failed to get file due to being unable to sense correct formant\n");
				}
			}
		}
		printf("request served\n");
		if(fd != -1){
			shutdown(fd, 2);
		}
		if(should_exit == 1){
			printf("Breaking the loop now\n");
			break;
		}
	}
	if(shutdown(clientID, 2) == -1){
		printf("Failed to shutdown client successfully\n");
		return NULL;
	}
	else{
		return NULL;
	}
}


int main(int argc, char **argv){
	int socketID, shutdownStatus;
	struct sockaddr_in my_addr;
	/* sigaction allow for ctrl-C to be used to exit the program
	 * this also allows for the program to run unattended untill the user wants it to stop
	 * the other signals are not handled and so will be their default behavior 
	 * see man sigaction and man signal for more details */
	struct sigaction sa;
	should_exit = 0;
	/* This is 1 if the shutdown of the socket and clients is Successful */
	shutdownStatus = 1;
	sa.sa_handler = handle_sigint;
	sigaction(SIGINT, &sa, NULL);
	/* This creates a tcp network socket using IPv4 that can communicate both ways using TCP see man 2 socket 
	 * for more info about this system. */
	socketID = socket(AF_INET, SOCK_STREAM, 0);
	if(socketID < 0){
		printf("Unable to create socket exiting now\n");
		exit(-1);
	}
	/*
	if(fcntl(socketID, F_SETFL, fcntl(socketID, F_GETFL, 0) | O_NONBLOCK) == -1){
		printf("Unable to make socket nonblocking\n");
		shutdown(socketID, 2);
		exit(-1);
	}
	*/
	/* This defines the socket address using netinet/in.h sockaddr_in structure
	 * use man netinet_in.h to find out more */
	my_addr.sin_family = AF_INET;
	/* binds port and ip address as specified in man netinet_in.h, man htonl for both functions */
	my_addr.sin_port = htons(8080);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* man 2 bind and man 3 bind */
	if(bind(socketID, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0){
		printf("Unable to bind socket exiting now\n");
		shutdown(socketID, 2);
		exit(-1);
	}
	printf("Successfully made and binded the socket\n");
	/* The second paramater is the maxium backlog before the server refuses connections */
	if (listen(socketID, 10) == -1) {
		printf("failed to listen on the socket");
		shutdown(socketID, 2);
		exit(-1);
	}
	printf("Listening on the socket\n");
	while(1){
		pthread_t thread;
		int clientID;
		if(should_exit == 1){
			break;
		}
		else{
			clientID = accept(socketID, 0, 0);
			if(clientID != -1) {
				printf("Accepted the client on the socket\n");
				pthread_create(&thread, NULL, handle_client, (void *)&clientID);
				pthread_detach(thread);
			}
		}
	}
	/* closes the socket by stopping both reads and writes man 2 shutdown */
	if(shutdown(socketID, 2) ==  -1){
		printf("Failed to shutdown socket successfully\n");
		shutdownStatus = 0;
	}
	if(shutdownStatus){
		printf("Shutdown Successfully\n");
		return 0;
	}
	else{
		return -1;
	}
}
