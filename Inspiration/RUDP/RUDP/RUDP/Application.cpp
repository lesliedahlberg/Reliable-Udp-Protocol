/*
 *QZMAO
 *
 *IEG3310 2009-2010 Term 1 Project
 *
 *RUDP Server and Client with random drop/delay
 */

#include <stdio.h>
#include "RUDP.h"
#define PORT 3310

int main(void) {
	char choice[16];
	while(1){
	printf("\nPlease select the usage of this program:");
	printf("\n[1]Server or [2]Client or [3]Show Credits or [4]Quit: ");
	scanf("%s", &choice);
	printf("\n");

	if (strcmp(choice, "1\0") == 0) {//act as server
		printf("This is server demo.\n\n");

		if (RUDP_socketS(PORT) == -1) {
			printf("Failed to create RUDP socket!\n");
			return 0;
		}
		char rbuf[32768] = "\0";		
				
		if (RUDP_receiveFrom(rbuf, 1) == -1) {
			printf("Failed to receive data!\n");
		}

		printf("\nThe data received is:\n%s\n\n", rbuf);
	
		RUDP_closesocket();
		printf("\n");
	}
	else if (strcmp(choice, "2\0") == 0) { //act as client

		printf("This is client demo.\n\nType :exit to close exit the program.\n\n");

		if (RUDP_socketC(PORT) == -1) {
			printf("Failed to create RUDP socket!\n");
			return 0;
		}

		while (1) {
			char buf[32768] = "\0";
			printf("\nType your message here:\n");
			fflush(stdin);
			gets_s(buf);
			
			if (strcmp(buf, ":exit") == 0) {
				exit(0);
			}

			if (RUDP_sendto(buf, strlen(buf)) == -1)
				printf("Failed to send RUDP data!\n");
		}

		RUDP_closesocket();
	}
	else if (strcmp(choice, "3\0") == 0) //act as client
		printf("RUDP Server and Client(R)\nVersion: 0.1 Alpha\nDeveloper: QZMAO\nAll rights reserved.\n\n");
	else if (strcmp(choice, "4\0") == 0) //quit program
		exit(0);
	else
		printf("Please enter 1 or 2 or 3 or 4!\n\n");
	}
	return 0;
}