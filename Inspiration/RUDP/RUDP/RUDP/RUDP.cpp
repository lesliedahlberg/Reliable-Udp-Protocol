#include <stdio.h>
#include "RUDP.h"

SOCKET SOCK1, SOCK2;
sockaddr_in addr1, addr2;
int len=sizeof(sockaddr);

/*Socket for server*/
int RUDP_socketS(int port) {
	WSADATA wsaData;
	int Ret = WSAStartup(MAKEWORD(2,2),&wsaData);//version Winsock 2.2
	printf("Initialize winsock . . .\n");
	if(Ret != 0) {
		printf("Unable to initialize winsock.\n");
		WSACleanup();//unable to open a socket, then cleanup
		return -1;  //error
	}
	else {
		printf("Done.\n");
	}
	    
	/*Create a RUDP socket and bind it.*/
	printf("Creating RUDP server socket . . .\n");
	SOCK1 = ::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); //create UDP socket 1.
	SOCK2 = ::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); //create UDP socket 2.

	addr1.sin_addr.S_un.S_addr = inet_addr(IP);//define the server's address
	addr1.sin_family = AF_INET;
	addr1.sin_port = ntohs(port);
	addr2.sin_addr.S_un.S_addr = inet_addr(IP);//define the server's address
	addr2.sin_family = AF_INET;
	addr2.sin_port = ntohs(port+1);
	    
	if(SOCK1 == INVALID_SOCKET || SOCK2 == INVALID_SOCKET) {
		printf("Socket Error!\n");
		WSACleanup();
		return -1;  //error
	}
	printf("Done.\n");
	printf("Binding RUDP socket . . .\n");
	if (bind(SOCK1, (SOCKADDR *) &addr1, sizeof(addr1)) == SOCKET_ERROR) {
		printf("bind1() failed.\n");
		return -1;  //error
		WSACleanup();
	}
	printf("Done.\n");
	return 0; //return success
}

/*Socket for client*/
int RUDP_socketC(int port) {
	WSADATA wsaData;
	int Ret = WSAStartup(MAKEWORD(2,2),&wsaData);//version Winsock 2.2
	printf("Initializing winsock . . .\n");
	if (Ret != 0) {
		printf("Unable to initialize winsock.\n");
		WSACleanup();//unable to open a socket, then cleanup
		return -1;  //error
	}
	else {
		printf("Done.\n");
	}
	    
	/*Create a RUDP socket and bind it.*/
	printf("Creating RUDP client socket . . .\n");
	SOCK1 = ::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); //create UDP socket 1.
	SOCK2 = ::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); //create UDP socket 2.

	addr1.sin_addr.S_un.S_addr = inet_addr(IP);//define the server's address
	addr1.sin_family = AF_INET;
	addr1.sin_port = ntohs(port);
	addr2.sin_addr.S_un.S_addr = inet_addr(IP);//define the server's address
	addr2.sin_family = AF_INET;
	addr2.sin_port = ntohs(port+1);
	    
	if(SOCK1 == INVALID_SOCKET || SOCK2 == INVALID_SOCKET) {
		printf("Socket Error!\n");
		WSACleanup();
		return -1;  //error
	}
	printf("Done.\n");
	printf("Binding RUDP socket . . .\n");

	if (bind(SOCK2, (SOCKADDR *) &addr2, sizeof(addr2)) == SOCKET_ERROR) {
		printf("Bind socket 2 failed.\n");
		return -1;  //error
		WSACleanup();
	}
	printf("Done.\n");
	return 0; //return success
}

/*send data + receive ACK*/
int RUDP_sendto(char * buf, int length) {
	if (strlen(buf) != length)
		return -1;
	if (length == -1)
		return -1;
	if (length > 32768) {
		printf("A single message's length must be less than 32 kilobytes!\n");
		return -1;  //error
	}

	int pks = int((length-1)/1024)+1; //message divided to pks packages
	int i, cnt;

	for (i=0;i<pks;i++) { //Client sends message to server and receives ACK	
		char stmp[1028] = "\0"; //buffer for each dadagram to be sent

		/*length of the message = 200*{stmp[0]} + {stmp[1]}, where {stmp[2]} is the package number*/
		/*{stmp[0]}:0-163, {stmp[1]}:0-199,{stmp[2]}:0-32*/
		/*each value plus 33 are in defined punctuation or letters area*/
		stmp[0] = strlen(buf)/200+33; 
		stmp[1] = strlen(buf)%200+33; 
		stmp[2] = i+33;
		/*put every 1024 bytes in the message to the stmp[] sequentially*/
		for (int j=0;j<1024 && buf[i*1024+j]!='/0';j++) {
			stmp[j+3] = buf[i*1024+j];
		}

		for (cnt=0;cnt<LIMIT;cnt++) {			
			int ds = sendto(SOCK1,stmp,strlen(stmp)+1,0,(SOCKADDR*)&addr1,len); //the origianl address
			char ackbuf[2] = "\0"; //buffer to store received ACK			ackbuf=(char*)malloc(sizeof(char)*2);

			int timeout = TIMEOUT; //set timeout time
			setsockopt(SOCK2, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout,sizeof(timeout)); //non-block
			int rev = recvfrom(SOCK2,ackbuf,sizeof(ackbuf),0,(SOCKADDR*)&addr2,&len); // receive ACK

			if (rev == -1)
				printf("Receive ACK timeout! Send again.\n");
			else if (ackbuf[0] == i+33) //package i's ACK not received, re-transfer
				break;
		}
		if (cnt == LIMIT) {
			printf("Tried too many times! Transmission cancelled.\n");
			return -1;	//error
		}
	}
	return (length+4*pks); //return total bytes that have been sent
}


/*receive data + ACK*/
int RUDP_receiveFrom(char * rbuf, int c) {
	int n = 0, rlen;
				
	while(1) {
		char rtmp[1028] = "\0", ack[2] = "\0"; //receive buffer and ACK

		if (recvfrom(SOCK1,rtmp,sizeof(rtmp),0,(SOCKADDR*)&addr1,&len) == -1) 
			return -1;  //error

		rlen = (rtmp[0]-33)*200+rtmp[1]-33;
		
		if (c!=0 && c!=1)
			printf("Second arg wrong!\n");
		if (c == 1) { //random drop enabled
			srand(time(NULL));
			if (n != rand()%32) {
				if (rtmp[2] != n+33) //Packages not received in sequence
					ack[0] = n+33;				
				else //packages received in sequence, continue
					ack[0] = rtmp[2];
			}
			else if (n!=0) 
				n--;	
		}
		if (c == 0) { //random drop disabled
			if (rtmp[2] != n+33) //Packages not received in sequence
				ack[0] = n+33;				
			else //packages received in sequence, continue
				ack[0] = rtmp[2];	
		}
		
		if (c!=0 && c!=1)
			printf("Second arg wrong!\n");
		if (c == 1) { //random delay enabled
			srand(time(NULL));
			delay(rand()%16);
		}

		sendto(SOCK2,ack,strlen(ack)+1,0,(SOCKADDR*)&addr2,len); //send ACK
		for (int m=0;m<1024&&rtmp[3+m]!='\0';m++) {
			rbuf[m+1024*n] = rtmp[3+m];
		}
		n++; //prepare to add next package
		if(strlen(rbuf) >= rlen)
			break;
	}
	return rlen; //return length of received message
}


void RUDP_closesocket() {
	printf("Closing sockets . . .\n");
	WSACleanup();
	closesocket(SOCK1);
	closesocket(SOCK1);
	printf("Done.");
}


/*function to detech connection speed(in delay time unit ms)*/
int RUDP_speed() {
	time_t start, end;
	char test[1024] = "\0";
	int t = 0;
	for (int i=0;i<1024;i++) {
		test[i]=i%10+33;
	}
	
	for (int j=0;j<5;j++) {
		time(&start); 
		if ((RUDP_sendto(test, strlen(test))) == -1) 
			return -1;
		time(&end);
		t = t + difftime(end, start);
	}
	return  t/5;
}

/*delay function*/
 void delay(int n) {
	time_t start, end;
	volatile long unsigned t;
	start = time(NULL);
	end = time(NULL);
	while (difftime(end,start)<n) {
		end = time(NULL);
	}
 }

