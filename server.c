#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <wait.h>
#include "GenerateEquasions.c"
#include "RankingDatabase.c"
#include "multicast.c"

#define PORT 50000
#define LISTENQ 2
#define SA struct sockaddr
#define NUM_OF_EQUATIONS 10

#define MULTICAST_ADDR "ff02::1"
#define MULTICAST_PORT 55555

void sig_chld(int signo)
{
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}

void sig_pipe(int signo)
{
	printf("Server received SIGPIPE: Exiting\n");
	exit(1);
}

signed int computeScore(char* usrAns[], char* corRes[], double ansTime[]) 
{
	signed int score = 0;
	int streak = 0;

	for (int i = 0; i < NUM_OF_EQUATIONS; ++i) {
		if (strcmp(usrAns[i], corRes[i]) == 0) {
			//printf("Correct answer!\n");
			streak++;
			score += streak * 10 * (1.0 / ansTime[i]);
		} else {
			if (streak > 0) {
				streak -= 3;
			}
			if (ansTime[i]<1){
				score -= 10 * 1.0/ansTime[i];
			}
			else {
				score -= 10 * ansTime[i];
			}
		}
		//printf("%d\t", score);
		//printf("user result = (%s), time = (%f), correct answer = (%s)\n", usrAns[i], ansTime[i], corRes[i]);
	}
	return score;
}

void play_game(int sockfd, char* cli_addr, int cli_port, sqlite3* db)
{
	char** equationsAndResults = generateRandomEquations(NUM_OF_EQUATIONS);
	int numberOfEquations = NUM_OF_EQUATIONS;
	char* userAnswer[NUM_OF_EQUATIONS];
	char* correctResults[NUM_OF_EQUATIONS];
	double answerTimes[NUM_OF_EQUATIONS];	// time player spent solving each equation (in seconds)
	double totalTime = 0.0;
	
	if(send(sockfd, &numberOfEquations, sizeof(int), 0) < 0){
		fprintf(stderr,"send() error : %s\n", strerror(errno));
	}

	for (int i = 0; i < 2*NUM_OF_EQUATIONS; i += 2) 
	{	
		if(send(sockfd, equationsAndResults[i], strlen(equationsAndResults[i])+1, 0) < 0){
			fprintf(stderr,"send() error : %s\n", strerror(errno));
		}
		
		userAnswer[i/2] = (char*)malloc(50 * sizeof(char));
            	if(recv(sockfd, &answerTimes[i/2], sizeof(double), 0) < 0){
            		fprintf(stderr,"recv() error : %s\n", strerror(errno));
            	}
            	if(recv(sockfd, userAnswer[i/2], sizeof(userAnswer[i/2]), 0) < 0){
            		fprintf(stderr,"recv() error : %s\n", strerror(errno));
            	}
            	correctResults[i/2] = equationsAndResults[i+1];
            	totalTime+=answerTimes[i/2];
		//printf("user result = (%s); (%s)\n", userAnswer[i/2], equationsAndResults[i+1]);
	}

	signed int finalScore = computeScore(userAnswer, correctResults, answerTimes);
	printf("Client %s (port: %d) finished with score: %d\n", cli_addr, cli_port, finalScore);
	freeEquations(equationsAndResults, NUM_OF_EQUATIONS);
	
	 // inform the client about the score
	if(send(sockfd, &finalScore, sizeof(signed int), 0) < 0){
		fprintf(stderr,"send() error : %s\n", strerror(errno));
	}
	
	Player newPlayer = {"", finalScore, totalTime};
	int decision = 0;
	
	if(recv(sockfd, &decision, sizeof(int), 0) < 0){
		fprintf(stderr,"recv() error : %s\n", strerror(errno));
	}
	
	//printf("decision: %d\n",decision);
	if (decision == 1) {
	
		if(recv(sockfd, newPlayer.username, sizeof(newPlayer.username), 0) < 0){
			fprintf(stderr,"recv() error : %s\n", strerror(errno));
		}
		//printf("username: %s\n",newPlayer.username);
		addPlayer(db, newPlayer);
	}
}

void handle_client(int sockfd, char* cli_addr, int cli_port, sqlite3* db)
{	
	int cond = 1;
	int input = 1;
	do
	{
		if(recv(sockfd, &input, sizeof(int), 0) < 0){
			fprintf(stderr,"recv() error : %s\n", strerror(errno));
		}
		
		switch (input)
		{
		case 1:
			play_game(sockfd, cli_addr, cli_port, db);
			break;
		case 2:
			sendRankingToClient(db, sockfd);
			break;
		case 3:
			cond = 0;
			break;
		default:
			break;
		}
	} while (cond == 1);
}



int main(int argc, char **argv)
{
	//===========================MULTICAST==========================
	
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
	  
	mcast_set_loop(sendfd, 1);

	if (fork() == 0)
		send_all(sendfd, sasend, salen);	/* child -> sends */
	
	//===================================================================
	
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in6	cliaddr, servaddr;
	void				sig_chld(int);
	char 				addr_buf[INET6_ADDRSTRLEN+1];

	if ( (listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
		fprintf(stderr,"socket error : %s\n", strerror(errno));
		return 1;
	}


	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr   = in6addr_any;
	servaddr.sin6_port   = htons(PORT);

	if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		fprintf(stderr,"bind error : %s\n", strerror(errno));
		return 1;
	}

	if ( listen(listenfd, LISTENQ) < 0){
		fprintf(stderr,"listen error : %s\n", strerror(errno));
		return 1;
	}

	signal(SIGCHLD, sig_chld);
	signal(SIGPIPE, sig_pipe);

	//---------------------database init-------------------------
	sqlite3* db;
	int rc = sqlite3_open("game_database.db", &db);
	handleSQLiteError(rc, db);
    	initDatabase(db);
	//-----------------------------------------------------------

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
			continue;
		else
			perror("accept error");
			exit(1);
		}

		bzero(addr_buf, sizeof(addr_buf));
		inet_ntop(AF_INET6, (struct sockaddr  *) &cliaddr.sin6_addr,  addr_buf, sizeof(addr_buf));
		printf("New client: %s, port %d\n",
		addr_buf, ntohs(cliaddr.sin6_port));


		if ((childpid = fork()) == 0) {	// the client is handled by child process
			close(listenfd);
			handle_client(connfd, addr_buf, ntohs(cliaddr.sin6_port), db);
			exit(0);
		}
		close(connfd);
	}
}

