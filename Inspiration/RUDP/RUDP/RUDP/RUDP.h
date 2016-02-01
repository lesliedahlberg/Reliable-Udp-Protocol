/*
 *Project properties -> Configuration Properties -> Linker -> Input -> Additional Dependencies -> ws2_32.lib
 */

#include <WinSock2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TIMEOUT 3000 //integer in ms
#define IP "127.0.0.1" //IP address, in format of "x.x.x.x"
#define LIMIT 5 //try out time limit

int RUDP_socketC(int port); //for server use
int RUDP_socketS(int port); //for client use
int RUDP_sendto(char * buf, int length);
int RUDP_receiveFrom(char * buf, int c); //if c is set to 0, random drop/delay is enabled. 0 means disabled.
void RUDP_closesocket();
int RUDP_speed();
void delay(int n);
