/*
** client.c -- a stream socket client demo
*/

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
#include <list>
#include <sstream>
#include <iostream>
#include <fstream>

#include <arpa/inet.h>

#define UDP_PORT "3427" // the udp port user will be connecting to 
#define TCP_PORT1 "21227" // the tcp port user will be connecting to 
#define TCP_PORT2 "21327" // the tcp port user will be connecting to 
#define TCP_PORT3 "21627" // the tcp port user will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 
using namespace std;

struct Car{
	string szName;
	int iBranch;
};

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
	list <Car> listOfCars = list <Car>();
	bool bDebugPhaseOne = false,
		 bDebugPhaseTwo = false,
		 bDebugPhaseThree = true;

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

	bool bIsString = true;
	while (!inputFile.eof())
	{
		int iNumber = -1;
		string szName;
		char tempName[256];
		stringstream out;
		stringstream sstr;
		inputFile.getline(tempName, 256);

		if (strlen(tempName) > 1)			// Length > 1 because end of line char \n 
		{
			Car theCar = Car();
			theCar.iBranch = -1;
			tempName[strlen(tempName)-1] = '\0';		// some weird thing i shappening when parsing text
			sstr << tempName;
			theCar.szName = sstr.str();
			if (bDebugPhaseTwo)
				printf("DEBUG: %s.\n", sstr.str().c_str());
			listOfCars.push_back(theCar);
		}
	}

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
	char *ipver;
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
		ipver = "IPv4";
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
		ipver = "IPv6";
	}

	inet_ntop(p->ai_family, addr,s, sizeof s);
	printf("User %d has UDP port %d and IP address %s.\n", m_iUserId, port , s);

	freeaddrinfo(servinfo); // all done with this structure

	// main loop phase 2
	unsigned int byte_count;
	for (list<Car>::iterator it = listOfCars.begin(); it != listOfCars.end(); it++)
	{
		char cMessage[256];
		int iRetValue;
		string carName = it->szName;

		printf("Checking %s in the database.\n",carName.c_str());
		// Event - Sending a packet to the central database: Checking <car> in the database
		sprintf(cMessage, "%i#%s", m_iUserId, carName.c_str());
		if (send(sockfd, cMessage, sizeof(cMessage), 0) == -1)
			perror("send");
		if (bDebugPhaseTwo)
			printf("DEBUG: %s\n",cMessage);
		//printf("Checking %s in the database\n",carName.c_str());
		byte_count = recv(sockfd, buf, sizeof buf, 0); // block until response received
		buf[byte_count] = '\0';
		iRetValue = atoi(buf);
		if (iRetValue > 0)
			it->iBranch = iRetValue;
		if (bDebugPhaseTwo)
			printf("DEBUG: retvalue - %i\n", iRetValue);
		// Event - Receiving a response packet from the database: Received location info of <car> from the database
		printf("Received location info of %s from the database\n", carName.c_str());
	}

	char cEndMessage[256];
	sprintf(cEndMessage, "%i#end", m_iUserId);
	if (send(sockfd, cEndMessage, sizeof(cEndMessage), 0) == -1)	// send empty meassage to let them know we are done.
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
		ipver = "IPv4";
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
		ipver = "IPv6";
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
		ipver = "IPv4";
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
		ipver = "IPv6";
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
		ipver = "IPv4";
	}
	else
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
		port = ntohs(ipv6->sin6_port);
		addr = &(ipv6->sin6_addr);
		ipver = "IPv6";
	}
	inet_ntop(p->ai_family, addr,s, sizeof s);
	printf("User %i has TCP port %d and IP address %s.\n", m_iUserId, port , s);

	// main loop phase 3
	for (list<Car>::iterator it = listOfCars.begin(); it != listOfCars.end(); it++)
	{
		int iSocket = -1;
		// get the correct socket to send query
		switch (it->iBranch)
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
		char cMessage[256];
		sprintf(cMessage, "%i#%s", m_iUserId,it->szName.c_str());
		if (send(iSocket, cMessage, sizeof(cMessage), 0) == -1)
			perror("send");
		printf("Sent a query for %s to Branch %i.\n", it->szName.c_str(), it->iBranch);

		// Event - Receiving the price of a car: <car> in <Branch#> with price <price>
		int iPrice;
		byte_count = recv(iSocket, buf, sizeof buf, 0); // block until response received
		buf[byte_count] = '\0';
		iPrice = atoi(buf);
		printf("%s in Branch %i with price %i.\n", it->szName.c_str(), it->iBranch, iPrice);
	}

	//if (send(iSocketBranch1, "", 0, 0) == -1)	// send empty meassage to let them know we are done.
	//	perror("send");
	//if (send(iSocketBranch2, "", 0, 0) == -1)	// send empty meassage to let them know we are done.
	//	perror("send");
	//if (send(iSocketBranch3, "", 0, 0) == -1)	// send empty meassage to let them know we are done.
	//	perror("send");

	// Event - End of Phase 3: End of Phase 3 for <User#>
	printf("End of Phase 3 for User %i.\n", m_iUserId);
	close(iSocketBranch1);
	close(iSocketBranch2);
	close(iSocketBranch3);

    return 0;
}