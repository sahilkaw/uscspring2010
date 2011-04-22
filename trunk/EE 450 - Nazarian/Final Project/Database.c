#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>

#define TCP_PORT "3327" // the TCP port branch will be connecting to 
#define UDP_PORT "3427" // the UDP port user will be connecting to 

#define BACKLOG 10			// how many pending connections queue will hold
#define TOTAL_BRANCHES 3    // 
#define MAXCARS 30			// Max number of cars

#define MAXBUFLEN 100

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

	// Stuff for multiple I/O
	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];
	struct addrinfo *ai;

	// Stuff for printing our adress
	void* addr;
	struct sockaddr address;
	unsigned short port;

	bool bDebugPhaseOne = false,
		 bDebugPhaseTwo = false;

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
	// beejs-start
	// Setup the IP and Port for this central database.
	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;		// set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_STREAM;	// TCP Connection

    if ((rv = getaddrinfo("nunki.usc.edu", TCP_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        return 1;
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

	// Event - Upon startup of Phase 1: The central database has TCP port … and IP address …
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
	printf("The central database has TCP port %d and IP address %s.\n", port , s);

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
	// beejs-end

    // main loop
	bool bPhaseOneIsOn = true;
	int iNumberOfConnectionsDropped = 0;
	int portPackage[TOTAL_BRANCHES][2];
	int iPortpkgCount = 0;
	// 0 - branch; 1 - port
	for (int i = 0; i < TOTAL_BRANCHES; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			portPackage[i][j] = -1;
		}
	}

	// beejs-start
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
							if (bDebugPhaseOne)
								printf("DEBUG: selectserver: socket %d hung up\n", i);

							// Event - Upon receiving all the cars’ make and model from a branch: Received the car list from <Branch#>
							int iTempBranchID = -1;
							for (int j = 0; j < iPortpkgCount; j++)
							{
								if (portPackage[j][1] == i)
									iTempBranchID = portPackage[j][0];
							}
							printf("Received the car list from Branch %d.\n", iTempBranchID);

							iNumberOfConnectionsDropped += 1;
							if (iNumberOfConnectionsDropped >= TOTAL_BRANCHES)
								bPhaseOneIsOn = false;
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
						// mines-start
                        // we got some data from a client
						buf[nbytes] = '\0';
						if (nbytes == 1){
							// save branchid associated with portid
							portPackage[iPortpkgCount][0] = atoi(buf);
							portPackage[iPortpkgCount][1] = i;
							iPortpkgCount++;
						}else{
							// save branchId
							int iTempBranchID = -1;
							for (int j = 0; j < iPortpkgCount; j++)
							{
								if (portPackage[j][1] == i)
									iTempBranchID = portPackage[j][0];
							}
							// save car name and price
							string szCarName, szPrice;
							stringstream sstr,sstr2;
							string str, str2;
							size_t found;

							// convert buffer to string
							buf[nbytes] = '\0';
							sstr << buf;
							str = sstr.str();
							sstr.clear();
	
							// get the two vars: car name & price
							found = str.find(',');
							szCarName = str.substr (0, found); 
							szPrice = str.substr (found + 1, str.length()); 

							sstr2 << iTempBranchID;
							str2 = sstr2.str();
							sstr2.clear();

							cars[iCarCount][0]=szCarName;
							cars[iCarCount][1]=szPrice;
							cars[iCarCount][2]=str2;
							if (bDebugPhaseOne)
								printf("DEBUG: %s,%s,%s\n", cars[iCarCount][0].c_str(), cars[iCarCount][1].c_str() ,cars[iCarCount][2].c_str());
							iCarCount++;


						}

						// Send acknowledgement
						if (bDebugPhaseOne)
							printf("DEBUG: Server got message '%s'.\n", buf);
						send(i, "0", 1, 0);
						// mines-end
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	// beejs-end

	// Event - End of Phase 1: End of Phase 1 for the database
	printf("End of Phase 1 for the database.\n");

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
	int m_iMaxUsers = 2;
	socklen_t addr_len;

	// beejs-start
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;		// set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;		// UDP Connection

    if ((rv = getaddrinfo("nunki.usc.edu", UDP_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

	// Event - Upon startup of Phase 2: The central database has UDP port … and IP address …
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
	printf("The central database has UDP port %d and IP address %s.\n", port , s);

	// beejs-end

	// main loop phase 2
	bool bPhaseTwoIsOn = true;
	int numbytes;
	int iNumberUsersFinished = 0;
	while(bPhaseTwoIsOn)
	{
		addr_len = sizeof their_addr;
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		if (numbytes > 0)
		{
			buf[numbytes] = '\0';

			if (bDebugPhaseTwo)
				printf("DEBUG: Message Recieved %s.\n",buf);
			// Message format: 'userid-carName'
			string szUser, szCarName;
			stringstream sstr;
			string str;
			size_t found;

			// convert buffer to string
			sstr << buf;
			str = sstr.str();
	
			// get the two vars: user & carName
			found = str.find('-');
			szUser = str.substr (0, found); 
			szCarName = str.substr (found + 1, str.length()); 

			if (bDebugPhaseTwo)
				printf("DEBUG: Message parsed %s and %s.\n", szUser.c_str(), szCarName.c_str());

			if (szCarName.compare("done") == 0)		// we are done with sending the list of cars
			{
				iNumberUsersFinished += 1;
				if (iNumberUsersFinished >= m_iMaxUsers)
					bPhaseTwoIsOn = false;
			}
			else
			{
				int iBranch = 0;
				// get branch
				for (int i = 0; i < iCarCount; i++)
				{
					if (cars[i][0].compare(szCarName) == 0)
					{
						iBranch = atoi(cars[i][2].c_str());
					}
				}

				// Event - Sending a response to a car query: Sent branch info about <car> to <User#>
				printf("Sent branch info about %s to User %s.\n", szCarName.c_str(), szUser.c_str());
				char message[5];
				sprintf(message, "%i", iBranch);
				if ((numbytes = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
					perror("talker: sendto");
					exit(1);
				}
				
			}
		}
	}

	// Event - End of Phase 2: End of Phase 2 for the database
	printf("End of Phase 2 for the database.\n");
    return 0;
}