#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <fstream>

#include <arpa/inet.h>

#define UDP_PORT "3427" // the udp port user will be connecting to 
#define TCP_PORT1 "21227" // the tcp port user will be connecting to 
#define TCP_PORT2 "21327" // the tcp port user will be connecting to 
#define TCP_PORT3 "21427" // the tcp port user will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define TOTAL_BRANCHES 3    // 
#define MAXCARS 30			// Max number of cars

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
	socklen_t sin_size;
    int rv;
    char s[INET6_ADDRSTRLEN];
	int m_iUserId = 1, m_iMaxUsers = 2;
	bool bDebugPhaseTwo = false,
		 bDebugPhaseThree = false;

	int iSocketBranch1, iSocketBranch2, iSocketBranch3;
	int yes=1;

	// Lets create all of our users.
	for (int i = 2; i <= m_iMaxUsers; i++)
	{
		if (!fork())
		{
			//in child process
			m_iUserId = i; 
			break; //prevent child process from spawning its own children
		}
	}


	string cars[MAXCARS][TOTAL_BRANCHES];
	int iCarCount = 0;
	// initialize the array
	// 0 is the car, 1 is the price, 2 is branchID
	for (int i = 0; i < MAXCARS; i++)
	{
		for (int j = 0; j < TOTAL_BRANCHES; j++)
		{
			cars[i][j] = "";
		}
	}
	/* Phase 2
		In phase 2 of this project, each of the users sends his/her queries to the central database
	through UDP connections. More specifically, each user opens a UDP connection to the
	central database to send the packets with the queries for cars. This means that each user
	should know in advance the static UDP port of the central database. You can hardcode this
	static UDP port setting the value according to Table 1. Then, it opens a UDP connection to
	this static UDP port of the central database. The UDP port number on the user side of the
	UDP connection is dynamically assigned (from the operating system). Thus, for this phase
	there are two UDP connections to the central database, one for each user in the system.
	Each user sends one packet for each car it wants to query the central database for.
	As soon as the central database receives a query for a car, it searches for it in the file
	or the array/linked list that stored the information regarding the cars in phase 1. If the car
	exists, it replies to the corresponding user with the index of the car rental branch that has
	the car. If the specific car is not in the system, the central database replies with the number
	0, and the user realizes that the car is not present in the system and no further handling for
	this car is required. If the same make and model of a car that the user queried is present in
	more than one branch, the central database should return the number of the branch with
	the smallest index.
	 */

	// Read in the file associated with the name "user#.txt" where # is the userId
	// input file
	char infile[256];
	sprintf(infile, "user%i.txt", m_iUserId);
	ifstream inputFile(infile);

	while (!inputFile.eof())
	{
		// temp vars
		char lineName[256];
		stringstream sstr;
		string str;
		size_t found;

		// get the line
		inputFile.getline(lineName, 256);
		sstr << lineName;
		str = sstr.str();
		sstr.clear();		

		// Find and replace: newline, return, escape
		found=str.find('\n');	// newline
		if (found!=string::npos)
			str.replace(str.find('\n'),1,"");
		found=str.find('\r');	// return
		if (found!=string::npos)
			str.replace(str.find('\r'),1,"");
		found=str.find('\e');	// escape
		if (found!=string::npos)
			str.replace(str.find('\e'),1,"");

		
		if (str.length() > 0)// make sure line isnt empty
		{
			cars[iCarCount][0] = str;
			cars[iCarCount][1] = "-1";
			cars[iCarCount][2] = "-1";
			
			if (bDebugPhaseTwo)
				printf("DEBUG: %s,%s,%s.\n", cars[iCarCount][0].c_str(),cars[iCarCount][1].c_str(),cars[iCarCount][2].c_str());
			iCarCount++;
		}
	}

	// beejs-start
	// Open a UDP connection with the central database
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo("nunki.usc.edu", UDP_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

	// Event - Upon Startup of Phase 2: <User#> has UDP port … and IP address …
	void* addr;
	struct sockaddr address;
	unsigned short port;
	if (getsockname(sockfd, &address, &sin_size) != 0)
	{
		perror("Branch could not read local socket.");
		return 3;
	}
	if (p->ai_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)&address;
		port = ntohs(ipv4->sin_port);
		addr = &(ipv4->sin_addr);
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
	}

	inet_ntop(p->ai_family, addr,s, sizeof s);
	printf("User %d has UDP port %d and IP address %s.\n", m_iUserId, port , s);

	freeaddrinfo(servinfo); // all done with this structure

	// beejs-end

	// main loop phase 2
	unsigned int byte_count;
	for (int i = 0; i < iCarCount; i++)
	{
		// temp values
		string szCarName, szPrice, szBranch;
		char message[256];
		int iRetValue;

		szCarName = cars[i][0];
		szPrice = cars[i][1];
		szBranch = cars[i][2];

		// Event - Sending a packet to the central database: Checking <car> in the database
		printf("Checking %s in the database.\n", szCarName.c_str());
		sprintf(message, "%i-%s", m_iUserId, szCarName.c_str());
		if (send(sockfd, message, sizeof(message), 0) == -1)
			perror("send");
		if (bDebugPhaseTwo)
			printf("DEBUG: %s\n",message);

		byte_count = recv(sockfd, buf, sizeof buf, 0); // block until response received
		buf[byte_count] = '\0';
		iRetValue = atoi(buf);
		if (iRetValue > 0){
			stringstream sstr;
			string str;
	
			sstr << iRetValue;
			str = sstr.str();
			cars[i][2] = str;
		}
		if (bDebugPhaseTwo)
			printf("DEBUG: retvalue - %i\n", iRetValue);
		// Event - Receiving a response packet from the database: Received location info of <car> from the database
		printf("Received location info of %s from the database\n", szCarName.c_str());
	}

	char message[256];
	sprintf(message, "%i-done", m_iUserId);
	if (send(sockfd, message, sizeof(message), 0) == -1)	// send 'done' message to say that we are at the end of the list.
		perror("send");

	// Event - Upon sending all the cars to the central database: Completed car queries to the database from <User#>.
	printf("Completed car queries to the database from User %d.\n", m_iUserId);
	close(sockfd);

	// Event - End of Phase 2: End of Phase 2 for <User#>
	printf("End of Phase 2 for User %d.\n", m_iUserId);

	/* Phase 3	
		In this phase, each user reads the reply packets from the central database and opens
	a TCP connection to the corresponding branches to ask for the price of the cars. This means,
	that if one user is interested in the price of three cars that each one of them is in a different
	car rental branch, the user has to open three different TCP connections, one for each branch
	and send a packet to the corresponding branch. If the user is interested in the price of more
	than one car that exist in the same branch, it has to use the same TCP connection and send
	the requests (one for each car) to the corresponding branch. This means that the users
	should know in advance the static TCP port numbers of the three different branches and
	you have to hardcode these values according to Table 1. As soon as a branch receives a
	request from a user, it should search for the make and the model of the car and send back a
	reply to the user containing the make and the model of the car along with the price. Then,
	when the user receives this packet should print an appropriate message in the screen.
	*/

	// beejs-start
	// Event - Upon Startup of Phase 3: <User#> has TCP port … and IP address …
	// Get branch1
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;		// set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_STREAM;	// TCP Connection

	if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT1, &hints, &servinfo)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((iSocketBranch1 = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (connect(iSocketBranch1, p->ai_addr, p->ai_addrlen) == -1) {
            close(iSocketBranch1);
            perror("server: connect");
            continue;
        }else{
			// Event - Upon establishing a TCP connection to each branch: <User#> is now connected to <Branch#>
			printf("User %i is now connected to Branch 1.\n",m_iUserId);
		}

        break;
    }
	
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
	freeaddrinfo(servinfo); // all done with this structure
	// Get branch2
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;		// set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_STREAM;	// TCP Connection

	if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT2, &hints, &servinfo)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((iSocketBranch2 = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (connect(iSocketBranch2, p->ai_addr, p->ai_addrlen) == -1) {
            close(iSocketBranch2);
            perror("server: connect");
            continue;
        }else{
			// Event - Upon establishing a TCP connection to each branch: <User#> is now connected to <Branch#>
			printf("User %i is now connected to Branch 2.\n",m_iUserId);
		}

        break;
    }
	
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
	freeaddrinfo(servinfo); // all done with this structure
	// Get branch3
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;		// set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_STREAM;	// TCP Connection

	if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT3, &hints, &servinfo)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((iSocketBranch3 = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (connect(iSocketBranch3, p->ai_addr, p->ai_addrlen) == -1) {
            close(iSocketBranch3);
            perror("server: connect");
            continue;
        }else{
			// Event - Upon establishing a TCP connection to each branch: <User#> is now connected to <Branch#>
			printf("User %i is now connected to Branch 3.\n",m_iUserId);
		}

        break;
    }

	
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
	freeaddrinfo(servinfo); // all done with this structure


	// Letsprint our tcp port number sand ips: <User#> has TCP port … and IP address …
	if (getsockname(iSocketBranch1, &address, &sin_size) != 0)
	{
		perror("Database could not read local socket.");
		return 3;
	}
	if (p->ai_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)&address;
		port = ntohs(ipv4->sin_port);
		addr = &(ipv4->sin_addr);
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
	}
	inet_ntop(p->ai_family, addr,s, sizeof s);
	printf("User %i has TCP port %d and IP address %s.\n", m_iUserId, port , s);
	if (getsockname(iSocketBranch2, &address, &sin_size) != 0)
	{
		perror("Database could not read local socket.");
		return 3;
	}
	if (p->ai_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)&address;
		port = ntohs(ipv4->sin_port);
		addr = &(ipv4->sin_addr);
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
	}

	inet_ntop(p->ai_family, addr,s, sizeof s);
	printf("User %i has TCP port %d and IP address %s.\n", m_iUserId, port , s);
	if (getsockname(iSocketBranch3, &address, &sin_size) != 0)
	{
		perror("Database could not read local socket.");
		return 3;
	}
	if (p->ai_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)&address;
		port = ntohs(ipv4->sin_port);
		addr = &(ipv4->sin_addr);
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
	}
	inet_ntop(p->ai_family, addr,s, sizeof s);
	printf("User %i has TCP port %d and IP address %s.\n", m_iUserId, port , s);
	// beejs-end

	// main loop phase 3
	for (int i = 0; i < iCarCount; i++)
	{
		// temp values
		string szCarName, szPrice, szBranch;
		char message[256];
		int iSocket = -1;

		// get the car info
		szCarName = cars[i][0];
		szPrice = cars[i][1];
		szBranch = cars[i][2];

		// get the correct socket
		switch (atoi(szBranch.c_str()))
		{
			case 1:
				iSocket = iSocketBranch1;
				break;
			case 2:
				iSocket = iSocketBranch2;
				break;
			case 3:
				iSocket = iSocketBranch3;
				break;
		}

		if (iSocket < 0)
			continue;

		// Event - Sending a car query to a branch: Sent a query for <car> to <Branch#>
		sprintf(message, "%i-%s", m_iUserId, szCarName.c_str());
		if (send(iSocket, message, sizeof(message), 0) == -1)
			perror("send");
		printf("Sent a query for %s to Branch %s.\n", szCarName.c_str(), szBranch.c_str());

		// Event - Receiving the price of a car: <car> in <Branch#> with price <price>
		int iPrice;
		byte_count = recv(iSocket, buf, sizeof buf, 0); // block until response received
		buf[byte_count] = '\0';
		iPrice = atoi(buf);
		printf("%s in Branch %s with price %i.\n", szCarName.c_str(), szBranch.c_str(), iPrice);
	}

	// Event - End of Phase 3: End of Phase 3 for <User#>
	printf("End of Phase 3 for User %i.\n", m_iUserId);
	close(iSocketBranch1);
	close(iSocketBranch2);
	close(iSocketBranch3);

    return 0;
}