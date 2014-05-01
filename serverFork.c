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
	char* start = strpbrk(buffer, "GET /");
	if (start == buffer)
		start += 5;
	else {
		write(sock, "HTTP/1.1 ", 9);
		write(sock, "500 Internal Error\n", 18-1);
		error("ERROR request not supported");
		return ;
	}

	char* end = strpbrk(start, " HTTP/");
	int length = end - start;
	printf("Filename length: %i\n", length);

	strncpy(filename, start, length);
	filename[length] = '\0';
	printf("Filename: %s\n", filename);

	// send header lines
	write(sock, "HTTP/1.1 ", 9);

	// Optional TODO: send RFC 1123 date
	// Optional TODO: send RFC 1123 last-modified
	// Optional TODO: send Content-Range
	// Optional TODO: send Content-Length

	// check if file exists
	struct stat buf;
	if (stat(filename, &buf) == 0)
		write(sock, "200 OK\n", 6-1); // file found
	else {
		write(sock, "404 File not found\n", 18-1); // file not found
		return ;
	}

	// content language
	write(sock, "Content-Language: en-US\n", 25-1);

	// check content type
	char* ext = strrchr(filename, '.');
	if (ext != NULL) {
		write(sock, "Content-Type: ", 14);

		if (strpbrk(ext, "html") != NULL)
			write(sock, "text/html; charset=UTF-8\n", 26-1);
		else if (strpbrk(ext, "gif") != NULL)
			write(sock, "image/gif\n", 11-1);
		else if (strpbrk(ext, "jpg") != NULL)
			write(sock, "image/jpg\n", 11-1);
		else if (strpbrk(ext, "jpeg") != NULL)
			write(sock, "image/jpeg\n", 12-1);
	}

	// send file
	FILE* file = fopen(filename, "r");
	int packet_size;
	while (packet_size = fread(buffer, 1, 256, file))
		write(sock, buffer, packet_size);
	if (ferror(file)) error("ERROR reading file");

	if (n < 0) error("ERROR writing to socket");
	return ;
}
