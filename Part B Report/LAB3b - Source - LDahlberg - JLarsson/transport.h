/* PROGRAM: transport
 * AUTHOR: Leslie Dahlberg, Jonathan Larsson
 * EMAIL: ldg14001@student.mdh.se, jln14010@student.mdh.se
 * DATE: 2016-02-10
 * File: transport.h
 * Reliable data transfer over UDP using Go-Back-N as a sliding window mechanism,
 * cumulative ACKs and the Internet Checksum for error checking
 * USAGE: Use u_ functions to establish a connections and send and recieve data reliably

 * FOR SERVER:
 * u_start();
 * u_listen("SERVER IP ADDRESS");
 * u_set_rcvr("POINTER TO FUNCTION THAT PROCESSES RECIEVED DATA");
 * u_start_recieving();
 * while(state != EXITING){
 *   getchar();
 * }

 * FOR CLIENT:
 * u_start();
 * u_connect("SERVER IP ADDRESS");
 * u_send("DATA"));
 * u_prep_sending();
 * u_close();
 * u_exit();
 * while(state != EXITING){
 *   getchar();
 * }
 */


#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <signal.h>

#define FOREVER 1
#define MODULO 1024
#define WINDOW_MODULO 1024
#define WINDOW 2
#define SERVER_PORT 8888
#define PACKET_DATA_SIZE 32
#define RESEND_PACKS 10

/* =================
   TYPES
   ================= */

   /* INPUTs */
   typedef enum {
     EXIT,NONE,CONNECT,CLOSE,SYN_ACK,SYN,ACK,FIN,FIN_ACK,TIMEOUT,RESET,GOOD_PACKET,BAD_PACKET,LISTEN
   } INPUT;

   /* STATES */
   typedef enum {
     EXITING,CLOSED,SYN_SENT,PRE_ESTABLISHED,ESTABLISHED_CLIENT,LISTENING,SYN_RECIEVED,ESTABLISHED_SERVER,FIN_WAIT_1,FIN_WAIT_2,TIME_WAIT,CLOSING,CLOSE_WAIT,LAST_ACK
   } STATE;

   /* PACKET */
   typedef struct {
     int seq;
     int ack;
     int fin;
     int syn;
     int id;
     int window_size;
     short int sum;
     char data[32];
   } PACKET;


   /* BUFFER */
   typedef struct {
     int seq_0;
     int seq_1;
     int seq_2;
     int last_ack;
     PACKET packet[MODULO];
   } BUFFER;

   /* TIMER */
   typedef struct {
       int on;
       clock_t start;
       clock_t length;
   } TIMER;

/* =================
   GLOBALS
   ================= */

   /* STATE MACHINE */
   STATE state;
   INPUT input;

   /* BUFFERS */
   BUFFER server_buf;
   BUFFER client_buf;
   BUFFER ack_buf;

   /* TIMERS */
   TIMER syn_sent_timer;
   TIMER pre_established_timer;
   TIMER syn_recieved_timer;
   TIMER client_established[MODULO];
   TIMER fin_wait_1_timer;
   TIMER time_wait_timer;
   TIMER closing_timer;
   TIMER close_wait_timer;
   TIMER last_ack_timer;

   /* SLIDING WINDOW*/
   int window;

   /* SOCKETS*/
   struct sockaddr_in server_socket_address;
   struct sockaddr_in client_address;
   struct sockaddr_in server_socket_self_address;
   struct sockaddr_in server_socket_client_address;

   int server_socket;
   int recv_len;
   int server_socket_length;

   /* THREADs */
   pthread_t tid;
   pthread_t tid2;

   /* ADDITIONAL VARIABLES */
   int first_ack;
   int is_server;
   int re_ack;
   int drop_rate;
   int id;
   int neg_window_size;
   char server_ip[];

   /* DATA PROCESSING */
   void (*process_data)(char*);


 /* =================
    FUNCTION DEFINITIONS
    ================= */

    /* USER CONTROL */

    void u_start();
    void u_close();
    void u_exit();
    /* Server */
    void u_listen();
    void u_start_recieving();
    void u_set_rcvr(void (*rcvr)(char*));
    /* Client */
    void u_connect();
    void u_prep_sending();
    void u_send(char* data, int length);

    /* INTERNAL */
    void send_packet(char data[32]);
    void send_message(int seq, int ack, int syn, int fin);
    void* recieve_packets(void *arg);
    void* recieve_acks(void *arg);

    /* TOOLS */
    int client_window();
    int next_seq(int seq);
    int next_ack(int ack);
    int prev_ack(int ack);
    int timeout(TIMER t);
    void reset_timer(TIMER *t);
    void decrease_timer(TIMER *t);
    clock_t clock_time(int milli_seconds);
    uint16_t ip_checksum(void* vdata,size_t length);

    /* STATE MACHINE - OUTPUT */
    void OUT_send_ack(int seq);
    void OUT_send_syn();
    void OUT_send_packet(PACKET p);
    void OUT_send_syn_ack();
    void OUT_send_fin();
    void OUT_send_fin_ack();

    /* STATE MACHINE */
    void* STATE_MACHINE(void* arg);



#endif
