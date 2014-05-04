/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/

#include <stdio.h>
#include <sys/types.h>	 // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>	 // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>	 // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */

void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	 int sockfd, newsockfd, portno, pid;
	 socklen_t clilen;
	 struct sockaddr_in serv_addr, cli_addr;
	 struct sigaction sa;		   // for signal SIGCHLD

	 if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	 }
	 sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd, 5);

	clilen = sizeof(cli_addr);

	/****** Kill Zombie Processes ******/
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	/*********************************/

	while (1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0)
			error("ERROR on accept");

		pid = fork(); // create a new process
		if (pid < 0)
			error("ERROR on fork");

		if (pid == 0)	{ // fork() returns a value of 0 to the child process
			close(sockfd);
			dostuff(newsockfd);
			exit(0);
		}
		else //returns the process ID of the child process to the parent
			close(newsockfd); // parent doesn't need this
	} /* end of while */
	return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connection has been established.
 *****************************************/
void dostuff (int sock) {
	int n;
	char buffer[256];
	char filename[256];

	bzero(buffer, 256);
	bzero(filename, 256);

	n = read(sock, buffer, 255);
	if (n < 0) error("ERROR reading from socket");
	printf("Here is the message: %s\n", buffer);

	// parse filename
	char* start = strstr(buffer, "GET /");
	if (start == buffer)
		start += 5;
	else {
		write(sock, "HTTP/1.1 ", 9);
		write(sock, "500 Internal Error\n", 18-1);
		error("ERROR request type not supported");
		return ;
	}

	char* end = strstr(start, " HTTP/");
	int length = end - start;
	strncpy(filename, start, length);
	filename[length] = '\0';
	printf("Filename: %s\n", filename);

	// begin header lines
	write(sock, "HTTP/1.1 ", 9);

	// check if file exists
	struct stat buf;
	if (length <= 0 || stat(filename, &buf) != 0) {
		// no file or file not found
		printf("404: File not found");
		write(sock, "404 Not Found\n", 15-1);
		write(sock, "Content-Language: en-US\n", 25-1);
		write(sock, "Content-Length: 0\n", 19-1);
		write(sock, "Content-Type: text/html\n", 23-1);
		write(sock, "Connection: close\n\n", 21-2);

		return ;
	}

	// file found
	write(sock, "200 OK\n", 6-1); // file found
	write(sock, "Content-Language: en-US\n", 25-1);
	FILE* file = fopen(filename, "r");

	// get filesize
	fseek(file, 0L, SEEK_END);
	int filesize = (int) ftell(file);
	fseek(file, 0L, SEEK_SET);

	// TODO: optional information
	// send RFC 1123 date
	// send RFC 1123 last-modified
	// send Content-Range
	// send Content-Type

	// send content length
	sprintf(buffer, "Content-Length: %d\n", filesize);
	printf("Filesize: %d\n", filesize);
	write(sock, buffer, strlen(buffer));

	// load file into memory
	char* filebuf = (char *) malloc(sizeof(char) * filesize);
	fread(filebuf, 1, filesize, file);

	// send file
	write(sock, "Connection: keep-alive\n\n", 26-2);
	write(sock, filebuf, filesize);
	if (ferror(file)) error("ERROR reading file");
	write(sock, "Connection: close\n\n", 21-2);

	if (n < 0) error("ERROR writing to socket");
	free(filebuf);
	fclose(file);
	return ;
}
