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

#define TCP_PORT "3327"		// the port branch will be connecting to 
#define TCP_PORT1 "21227"	// the tcp port user will be connecting to 
#define TCP_PORT2 "21327"	// the tcp port user will be connecting to 
#define TCP_PORT3 "21427"	// the tcp port user will be connecting to 

#define TOTAL_USERS 2		// Max number of users
#define MAXDATASIZE 100		// max number of bytes we can get at once 
#define BACKLOG 10			// how many pending connections queue will hold
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
	int m_iBranchId = 1, m_iMaxBranches = 3;
	bool bDebugPhaseOne = false,
		 bDebugPhaseThree = false;

	string cars[30][2];
	int iCarCount = 0;

	// Lets create all of our branches.
	for (int i = 2; i <= m_iMaxBranches; i++)
	{
		if (!fork())
		{
			//in child process
			m_iBranchId = i; 
			break; //prevent child process from spawning its own children
		}
	}

	// initialize the array
	// 0 is the car, 1 is the price
	for (int i = 0; i < MAXCARS; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			cars[i][j] = "";
		}
	}
	/* Phase 1 
		In this phase, the three car rental branches open their input file (branch1.txt, or branch2.txt
	or branch3.txt) and send the make and the model of the cars to the central database. More
	specifically, each car rental branch opens a TCP connection with the central database to
	send the make and the model of the cars that exist in the branch. It sends one packet per car
	that is in the branch. This means that the car rental branches should know the TCP port
	number of the central database in advance. In other words you must hardcode the TCP port
	number of the central database in the Branch code. Table 1 shows how static UDP and TCP
	port numbers should be defined for each entity in this project. Each car rental branch then
	will use one dynamically-assigned TCP port number (different for each car rental branch)
	and establish one TCP connection to the central database (see Requirement 1 of the project
	description). Thus, there will be three different TCP connections to the central database
	(one from each branch).
		As soon as the central database receives the packets with the make and the model of
	the cars from the three branches, it stores locally the available cars in the system along with
	the index of the branch(es) in which the cars can be found. Note that multiple car branches
	may have the same car and the central database should store all the branches in which one
	specific car can be found. If later on, one user is interested in a car that exists in more than
	one branches, the central database should give him/her the index of the branch with the
	smallest index among the ones that the specific car can be found. It is up to you to decide
	how you are going to store in which branches the different cars exist. It may be stored in a
	file or in the memory (e.g. through an array or a linked list of structs that you can define).
	Before you make this decision, you have to think of how you are going to query later the
	central database. By the end of phase 1, we expect that the central database knows which
	cars are available in the system and in which car rental branch(es) they exist.
	*/

	// Read in the file associated with the name "Branch#.txt" where # is the branchId
	// input file
	char infile[256];
	sprintf(infile, "branch%i.txt", m_iBranchId);
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

		if (str.length() > 3) // a#1 is smallest string
		{
			// temp vars
			string strA, strB;

			// get the two vars: car & price
			found = str.find('#');
			strA = str.substr (0, found); 
			strB = str.substr (found + 1, str.length()); 

			if (bDebugPhaseOne)
				printf("DEBUG: %s - %s.\n",strA.c_str(),strB.c_str());

			// assign car values
			cars[iCarCount][0] = strA;
			cars[iCarCount][1] = strB;
			iCarCount++;
		}
	}

	// beejs-start
	// Open a TCP connection with the central database
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT, &hints, &servinfo)) != 0) {
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

	// Event - Upon startup of Phase 1: <Branch#> has TCP port … and IP address …
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
	printf("Branch %d has TCP port %d and IP address %s.\n", m_iBranchId, port , s);

	freeaddrinfo(servinfo); // all done with this structure

	// Event - Upon establishing a TCP connection to the database: <Branch#> is now connected to the database.
    printf("Branch %d is now connected to the database.\n", m_iBranchId);
	// beejs-end

	// main loops for phase 1
	unsigned int byte_count;
	char cBranchID[2];
	// send branch id
	sprintf(cBranchID, "%i", m_iBranchId);
	if (send(sockfd, cBranchID, 1, 0) == -1)	// send branch id
		perror("send");
	byte_count = recv(sockfd, buf, sizeof buf, 0); // block until response received
	for (int i = 0; i < iCarCount; i++)
	{
		// temp vars
		char message[256];
		string szCar, szPrice;
		szCar = cars[i][0];
		szPrice = cars[i][1];

		// Message sending and receiving
		// Event - Sending a car’s make and model to the central database: <Branch#> has sent <car> to the database.
		printf("Branch %i has sent %s to the database.\n", m_iBranchId, szCar.c_str());
		sprintf(message, "%s,%s", szCar.c_str(), szPrice.c_str());
		if (send(sockfd, message, sizeof(message), 0) == -1)
			perror("send");
		if (bDebugPhaseOne)
			printf("DEBUG: Message %s - %s.\n", szCar.c_str(), szPrice.c_str());
		byte_count = recv(sockfd, buf, sizeof buf, 0); // block until response received
	}

	// Event - Upon sending all the cars’ make and model to the central database: Updating the database is done for <Branch#>
	printf("Updating the database is done for Branch %d.\n",m_iBranchId);
	close(sockfd);

	// Event - End of Phase 1: End of Phase 1 for <Branch#>
	printf("End of Phase 1 for Branch %d.\n",m_iBranchId);

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

	// Stuff for multiple I/O
	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int newfd;        // newly accepted socket descriptor
	struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    int nbytes;
	int yes=1;

    char remoteIP[INET6_ADDRSTRLEN];
	struct addrinfo *ai;

	// beejs-start
	// Setup the IP and Port for this central database.
	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;		// set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_STREAM;	// TCP Connection

	switch(m_iBranchId)
	{
		case 1:
			if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT1, &hints, &servinfo)) != 0) {
				fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
				return 1;
			}
			break;
		case 2:
			if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT2, &hints, &servinfo)) != 0) {
				fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
				return 1;
			}
			break;
		case 3:
			if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT3, &hints, &servinfo)) != 0) {
				fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
				return 1;
			}
			break;
	}

	 // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }
	
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

	// Event - Upon startup of Phase 3: <Branch#> has TCP port … and IP address … for Phase 3
	if (getsockname(sockfd, &address, &sin_size) != 0)
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
	printf("Branch %i has TCP port %i and IP address %s for Phase 3.\n", m_iBranchId, port , s);

    freeaddrinfo(servinfo); // all done with this structure

    // listen
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(3);
    }
	// add the listener to the master set
    FD_SET(sockfd, &master);

    // keep track of the biggest file descriptor
    fdmax = sockfd; // so far, it's this one

    // main loop
	bool bPhaseOneIsOn = true;
	int iNumberOfConnectionsDropped = 0;
    while(bPhaseOneIsOn) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == sockfd) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(sockfd, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
						if (bDebugPhaseOne)
							printf("DEBUG: selectserver: new connection from %s on "
								"socket %d\n",
								inet_ntop(remoteaddr.ss_family,
									get_in_addr((struct sockaddr*)&remoteaddr),
									remoteIP, INET6_ADDRSTRLEN),
								newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
							// connection closed
							if (bDebugPhaseThree)
								printf("DEBUG: selectserver: socket %d hung up\n", i);

							iNumberOfConnectionsDropped += 1;
							if (iNumberOfConnectionsDropped >= TOTAL_USERS)
								bPhaseOneIsOn = false;
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
						// mines-start
                        // we got some data from a client
						// Message format: UserId-carName
						string szUser, szCarName;
						stringstream sstr;
						string str;
						size_t found;

						// convert buffer to string
						buf[nbytes] = '\0';
						sstr << buf;
						str = sstr.str();
	
						// get the two vars: user & carName
						found = str.find('-');
						szUser = str.substr (0, found); 
						szCarName = str.substr (found + 1, str.length()); 

						// make sure userid is between 1-3
						if (atoi(szUser.c_str()) < 1 || atoi(szUser.c_str()) > 3)
							continue;

						// Event - Upon receiving a car’s make and model from a user: <Branch#> received query for <car> from <User#>
						printf("Branch %i received query for %s from User %s\n", m_iBranchId, szCarName.c_str(), szUser.c_str());

						//get cost
						string szCost;
						for (int j = 0; j < iCarCount; j++)
						{
							// temp vars
							char message[256];
							string szCar, szPrice;
							szCar = cars[j][0];
							szPrice = cars[j][1];

							// Message sending and receiving
							if (szCarName.compare(szCar) == 0){
								szCost = szPrice;
								break;
							}
						}

						// Send acknowledgement
						// Event - Sending the price of a car to a user: <Branch#> sent the price of <car> to <User#>
						printf("Branch %i sent the price of %s to User %s.\n", m_iBranchId, szCarName.c_str(), szUser.c_str());
						if (send(i, szCost.c_str(), sizeof(szCost.c_str()), 0) == -1)
							perror("send");
						// mines-end
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
	// beejs-end

	// Event - End of Phase 3: End of Phase 3 for the Branch#
	printf("End of Phase 3 for the Branch %i.\n", m_iBranchId);
    return 0;
}