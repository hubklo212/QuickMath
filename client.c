#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include "multicast.c"

#define PORT 50000

#define MULTICAST_ADDR "ff02::1"
#define MULTICAST_PORT 55555

int isNumber(const char* str) {
    char* endptr;
    strtol(str, &endptr, 10);
    return (*endptr == '\0');
}

double getElapsedTime(struct timespec start, struct timespec end) {
    return ((double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9);
}



void receiveDataFromServer(int sockfd) {
 char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        printf("%s", buffer);

        if (buffer[bytesRead - 1] == '\0') {
            break;
        }
    }

    if (bytesRead < 0) {
        perror("Error receiving message from server");
    } else {
        printf("\n");
    }
}

int main(int argc, char **argv) {

	//============================MULTICAST INIT==================================
	
	int sendfd, recvfd;
	const int on = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv;
	struct sockaddr_in6 *ipv6addr;
	struct sockaddr_in *ipv4addr;

	if (argc != 2){
		fprintf(stderr, "usage: %s <if name>\n", argv[0]);
		return 1;
	}

	sendfd = snd_udp_socket(MULTICAST_ADDR, MULTICAST_PORT, &sasend, &salen);

	if ( (recvfd = socket(sasend->sa_family, SOCK_DGRAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}

	if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		fprintf(stderr,"setsockopt error : %s\n", strerror(errno));
		return 1;
	}

	sarecv = malloc(salen);
	memcpy(sarecv, sasend, salen);

	setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, argv[1], strlen(argv[1]));
	
	if(sarecv->sa_family == AF_INET6){
		ipv6addr = (struct sockaddr_in6 *) sarecv;
		ipv6addr->sin6_addr =  in6addr_any;

		int32_t ifindex;
		ifindex = if_nametoindex(argv[1]);
		if(setsockopt(sendfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0){
			perror("setting local interface");
			exit(1);
		}
	}

	if(sarecv->sa_family == AF_INET){
		ipv4addr = (struct sockaddr_in *) sarecv;
		ipv4addr->sin_addr.s_addr =  htonl(INADDR_ANY);

		struct in_addr        localInterface;
		localInterface.s_addr = inet_addr("127.0.0.1");
		if (setsockopt(sendfd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0) {
			perror("setting local interface");
			exit(1);
		}
	}
	
	if( bind(recvfd, sarecv, salen) < 0 ){
	    fprintf(stderr,"bind error : %s\n", strerror(errno));
	    return 1;
	}
	
	if( mcast_join(recvfd, sasend, salen, argv[1], 0) < 0 ){
		fprintf(stderr,"mcast_join() error : %s\n", strerror(errno));
		return 1;
	}
	struct ServerInfo serverInfo;
	recv_all(recvfd, salen, &serverInfo);

	//======================================================================================

	int clientSocket;
	struct sockaddr_in6 serverAddr;
	struct timespec start, end;

	if ((clientSocket = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
	    perror("Socket creation failed");
	    exit(EXIT_FAILURE);
	}

	serverAddr.sin6_family = AF_INET6;
	serverAddr.sin6_port = htons(PORT);

	// Use inet_pton to convert IPv6 address from string to binary format
	if (inet_pton(AF_INET6, serverInfo.ip, &serverAddr.sin6_addr) <= 0) {
	    perror("Invalid server address");
	    exit(EXIT_FAILURE);
	}

	if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
		perror("Connection failed");
		exit(EXIT_FAILURE);
	}

	printf("Connected to the server\n");

	const char* text1 = "==============================\n--Welcome to QuickMath game!--\n==============================\n";
	const char* options = "What would you like to do?\n1. Play\n2. Display leaderboard\n3. Exit\n";
	
	printf("%s",text1);
	
	int cond = 1;
	do
	{
    		printf("%s\n", options);
		
	    	int integerValue=1;
	    	int integerValue2=1;
	   	int valid = 0;

		do {
			printf("Enter your choice: ");

			// Use scanf to get integer input
			if (scanf("%d", &integerValue) == 1) {
			valid = 1;
			} else {
				printf("Invalid input. Enter a number!\n");

				// Clear the input buffer in case of invalid input
				while (getchar() != '\n');
			}
		} while (!valid);
		
		printf("------------------------------\n");
		
		if(send(clientSocket, &integerValue, sizeof(int), 0) < 0){
			fprintf(stderr,"send() error : %s\n", strerror(errno));
		}
		
		switch (integerValue)
		{
			//----------------------------------GAME PART----------------------------------------
			case 1:
			{
				int numberOfEquations = 0;
				if(recv(clientSocket, &numberOfEquations, sizeof(int), 0) < 0){
            				fprintf(stderr,"recv() error : %s\n", strerror(errno));
            			}

				char equation[50];
				char* answer[numberOfEquations];
				char buffor[50];
				signed int score;
				int countdown = 3;
				
				while(countdown > 0){
					printf("%d...\n", countdown);
					sleep(1);
					countdown--;
				}
				printf("GO!!!\n");
				
				// Receive and answer equations
				for (int i = 0; i < numberOfEquations; i++) {
					if(recv(clientSocket, equation, sizeof(equation), 0) < 0){
            					fprintf(stderr,"recv() error : %s\n", strerror(errno));
            				}
					clock_gettime(CLOCK_MONOTONIC, &start); // Record the start time

					int validInput = 0;

					do {
						printf("Equation %d: %s\n", i + 1, equation);
						printf("Enter your answer: ");
						scanf("%s", buffor);

						if (isNumber(buffor)) {
							validInput = 1;
						} else {
							printf("Invalid input. The result consists of numbers only!.\n");

							// Clear the input buffer in case of invalid input
							while (getchar() != '\n');
						}
					} while (!validInput);
					clock_gettime(CLOCK_MONOTONIC, &end); // Record the end time

					// Calculate and print the time for each iteration
					double timePerQuestion = getElapsedTime(start, end);
					printf("Time: %f seconds\n", timePerQuestion);
					printf("------------------------------\n");

					answer[i] = (char*) malloc((strlen(buffor)+1)*sizeof(char));
					strcpy(answer[i], buffor);
					
		// time value has a fixed amount of memory allocated (sizeof(double)) so it's sent before the result
					if(send(clientSocket, &timePerQuestion, sizeof(double), 0) < 0){
						fprintf(stderr,"send() error : %s\n", strerror(errno));
					}

					if(send(clientSocket, answer[i], strlen(answer[i])+1, 0) < 0){
						fprintf(stderr,"send() error : %s\n", strerror(errno));
					}
				}

				for (int i = 0; i < numberOfEquations; ++i) {
					free(answer[i]);
				}

				// Receive and display the total score
				if(recv(clientSocket, &score, sizeof(score), 0) < 0){
            				fprintf(stderr,"recv() error : %s\n", strerror(errno));
            			}
				
				printf("\nTotal Score: %d\n\n", score);
				
				printf("Do you want to save your score in the leaderboard?\n");
				printf("1. Yes\n2. No\n");
				
				do {
					printf("Enter your choice: ");

					// Use scanf to get integer input
					if (scanf("%d", &integerValue2) == 1) {
					valid = 1;
					} else {
						printf("Invalid input. Enter a number!\n");

						// Clear the input buffer in case of invalid input
						while (getchar() != '\n');
					}
				} while (!valid);
				if(send(clientSocket, &integerValue2, sizeof(int), 0) < 0){
						fprintf(stderr,"send() error : %s\n", strerror(errno));
				}
				
				if (integerValue2 == 1) {
					char nickname[17];

					printf("Enter your nickname (max 16 characters): ");
					if (scanf("%16s", nickname) != 1) {
						// Handle input error
						printf("Error reading nickname. Score reamins unsaved.\n");
						break;
						
					}
					size_t len = strlen(nickname);
					if (len > 0 && nickname[len - 1] == '\n') {
						nickname[len - 1] = '\0';
					}
					if(send(clientSocket, nickname, strlen(nickname)+1, 0) < 0){
						fprintf(stderr,"send() error : %s\n", strerror(errno));
					}
				}				    

				printf("==============================\n\n");
				break;
			}
			//---------------------------------------------------------------------------------
			case 2:
				receiveDataFromServer(clientSocket);
				break;
			case 3:
				cond = 0;
				break;
			default:
				printf("Enter a number 1-3!\n");
		}
	} while (cond == 1);

	// Close the socket
	close(clientSocket);
	return 0;
}

