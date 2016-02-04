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

#define FOREVER 1
#define MODULO 1024
#define WINDOW_MODULO 1024
#define WINDOW 2

#define SERVER_IP "127.0.0.111"
#define SERVER_PORT 8888



/* CHECKSUM */

uint16_t ip_checksum(void* vdata,size_t length) {
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;

    // Initialise the accumulator.
    uint32_t acc=0xffff;

    size_t i;
    // Handle complete 16-bit blocks.
    for (i=0;i+1<length;i+=2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Handle any partial block at the end of the data.
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Return the checksum in network byte order.
    return htons(~acc);
}

/* THREADs */
pthread_t tid, tid2;

/* PACKET */
typedef struct {
  int seq;
  int ack;
  int fin;
  int syn;
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

/* FUNCTIONS */
void* STATE_MACHINE(void* arg);
int client_window();
int next_seq(int seq);
int next_ack(int ack);
int timeout(TIMER t);
void reset_timer(TIMER *t);
void decrease_timer(TIMER *t);
clock_t clock_time(int milli_seconds);
void start();

/* BUFFERS */
BUFFER server_buf;
BUFFER client_buf;
BUFFER ack_buf;

int first_ack;

/* INPUTs */
typedef enum {
  EXIT,NONE,CONNECT,CLOSE,SYN_ACK,SYN,ACK,FIN,FIN_ACK,TIMEOUT,RESET,GOOD_PACKET,BAD_PACKET,LISTEN
} INPUT;

/* STATES */
typedef enum {
  EXITING,CLOSED,SYN_SENT,PRE_ESTABLISHED,ESTABLISHED_CLIENT,LISTENING,SYN_RECIEVED,ESTABLISHED_SERVER,FIN_WAIT_1,FIN_WAIT_2,TIME_WAIT,CLOSING,CLOSE_WAIT,LAST_ACK
} STATE;

/* STATE MACHINE */
STATE state;
INPUT input;

int is_server;

/* SOCKETS*/
//Client
struct sockaddr_in server_socket_address, client_address;
int server_socket, server_socket_length=sizeof(server_socket_address);
//Server
struct sockaddr_in server_socket_self_address, server_socket_client_address;
int server_socket, i, recv_len;


/* WINDOW SIZE */
int client_window(){
  if(client_buf.seq_1 > client_buf.seq_2){
    return client_buf.seq_1 - client_buf.seq_2;
  }else{
    return MODULO - (client_buf.seq_1 - client_buf.seq_2);
  }
}

/* MODULO NEXT SEQ. IN INCOMING BUFFER */
int next_seq(int seq){
  if(seq+1 > MODULO - 1){
    return 0;
  }else{
    return seq+1;
  }
}

/* MODULO NEXT ACK. IN INCOMING SLIDING WINDOW */
int next_ack(int ack){
  if(ack == -1){
    return 0;
  }
  if(ack+1 > WINDOW_MODULO - 1){
    return 0;
  }else{
    return ack+1;
  }
}

/* MODULO PREV. ACK. IN INCOMING SLIDING WINDOW */
int prev_ack(int ack){
  if(ack == 0){
    if(first_ack == 1){
      return 0;
    }else{
      return WINDOW_MODULO - 1;
    }

  }else{
    return ack-1;
  }
}

/* TIMER FUNCTIONS */
int timeout(TIMER t){
    if(clock()-t.start >= t.length){
        return 1;
    }else{
        return 0;
    }
}

void reset_timer(TIMER *t){
  t->on = -1;
}

void decrease_timer(TIMER *t){
  if(t->on > 0){
    t->on--;
  }else{
    t->on = 0;
  }
}

clock_t clock_time(int milli_seconds){
    //return (milli_seconds/1000)*CLOCKS_PER_SEC;
    return 1000;
}

/* SEND PACKET */
void send_packet(char data[32]){

  //printf("SEND PACKET: placed in client buffer\n");
  strcpy(client_buf.packet[client_buf.seq_0].data, data);
  client_buf.packet[client_buf.seq_0].seq = client_buf.seq_0;
  client_buf.packet[client_buf.seq_0].sum = ip_checksum(&client_buf.packet[client_buf.seq_0], sizeof(PACKET));
  printf("CHECKSUM:%04X\n", client_buf.packet[client_buf.seq_0].sum);
  client_buf.seq_0 = next_seq(client_buf.seq_0);
}

/* SEND ACK */
void send_ack(int seq, int ack, int syn, int fin){
  PACKET ack_packet;

  ack_packet.ack = ack;
  ack_packet.seq = seq;
  ack_packet.syn = syn;
  ack_packet.fin = fin;
  unsigned int slen;
  //SEND over network

    //printf("Socket: %d;\n", server_socket);
    if(is_server == 1){
      slen = sizeof(server_socket_client_address);
      if((sendto(server_socket, &ack_packet, sizeof(PACKET), 0, (struct sockaddr *) &server_socket_client_address, slen))==-1)
        {
            //die("sendto()");
            perror("Sendto.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(is_server == 0)
    {
        slen = sizeof(server_socket_address);
        if((sendto(server_socket, &ack_packet, sizeof(PACKET), 0, (struct sockaddr *) &server_socket_address, slen))==-1)
        {
            //die("sendto()");
            perror("Sendto.\n");
            exit(EXIT_FAILURE);
        }
    }

    //printf("SEND ACK: seq: %d; ack: %d; syn:%d; fin:%d;\n", ack_packet.seq, ack_packet.ack, ack_packet.syn, ack_packet.fin);
}

/* RECIEVE PACKET */
void recieve_packet()
{
    PACKET pack;
    int i = 4;
    while(1)
    {

        //printf("Waiting for data...");
        fflush(stdout);

        unsigned int slen = sizeof(server_socket_client_address);
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(server_socket, &pack, sizeof(PACKET), 0, (struct sockaddr *) &server_socket_client_address, &slen)) == -1)
        {
            //die("recvfrom()");
            perror("Recvfrom.\n");
            exit(EXIT_FAILURE);
        }

        if(pack.syn == 1 && pack.ack == 1){
          input = SYN_ACK;
        }else if(pack.fin == 1 && pack.ack == 1){
          input = FIN_ACK;
        }else if(pack.fin == 1){
          input = FIN;
        }else if(pack.ack == 1){
          input = ACK;
        }else if(pack.syn == 1){
          input = SYN;
        }else{




          if(ip_checksum(&pack, sizeof(PACKET)) == 0){
            if(pack.seq == server_buf.seq_1){


              i--;
              printf("RECIEVE PACKET: placed in server buffer\n");
              //printf("Received packet from %s:%d\n", inet_ntoa(server_socket_client_address.sin_addr), ntohs(server_socket_client_address.sin_port));
              printf("Data: %s\n" , pack.data);
              strcpy(server_buf.packet[server_buf.seq_2].data, pack.data);
              server_buf.packet[server_buf.seq_2].ack = pack.ack;
              server_buf.packet[server_buf.seq_2].fin = pack.fin;
              server_buf.packet[server_buf.seq_2].syn = pack.syn;
              server_buf.packet[server_buf.seq_2].sum = pack.sum;
              server_buf.packet[server_buf.seq_2].seq = pack.seq;
              server_buf.seq_2 = next_seq(server_buf.seq_2);
            }
          }else{
            printf("BAD PACKET\n");


          }

        }


        //print details of the client/peer and the data received

    }


}

/* RECIEVE ACK */
void recieve_ack()
{
    PACKET ack;
    int i = 4;
    while(1)
    {

        //printf("Waiting for ack...");
        fflush(stdout);

        unsigned int slen = sizeof(server_socket_address);
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(server_socket, &ack, sizeof(PACKET), 0, (struct sockaddr *) &server_socket_address, &slen)) == -1)
        {
            //die("recvfrom()");
            perror("Recvfrom.\n");
            exit(EXIT_FAILURE);
        }


        if(ack.syn == 1 && ack.ack == 1){
          input = SYN_ACK;
        }else if(ack.fin == 1 && ack.ack == 1){
          input = FIN_ACK;
        }else if(ack.fin == 1){
          input = FIN;
        }else if(ack.ack == 1 && ack.seq == -1){
          input = ACK;
        }else if(ack.syn == 1){
          input = SYN;
        }else{
          i--;
          //printf("RECIEVE ACK: placed in server buffer\n");
          printf("RECV ACK: %d\n", ack.seq);
          strcpy(ack_buf.packet[ack_buf.seq_2].data, ack.data);
          ack_buf.packet[ack_buf.seq_2].ack = ack.ack;
          ack_buf.packet[ack_buf.seq_2].fin = ack.fin;
          ack_buf.packet[ack_buf.seq_2].syn = ack.syn;
          ack_buf.packet[ack_buf.seq_2].sum = ack.sum;
          ack_buf.packet[ack_buf.seq_2].seq = ack.seq;
          ack_buf.seq_2 = next_seq(ack_buf.seq_2);
          //print details of the client/peer and the data received
          //printf("Received ack from %s:%d\n", inet_ntoa(server_socket_address.sin_addr), ntohs(server_socket_address.sin_port));
          //printf("ACK_SEQ: %d:%d, %d\n", ack.seq, ack.ack, ack_buf.packet[ack_buf.seq_2].seq);
        }







    }


}

/* OUT FUNCTIONS */
void OUT_send_ack(int seq){
  printf("OUT: send ack %d\n", seq);
  send_ack(seq, 1, 0, 0);
}

void OUT_send_syn(){
  printf("OUT: send syn\n");
  send_ack(-1, 0, 1, 0);
}

void OUT_send_packet(PACKET p){
  printf("OUT: send packet; DATA = %s;\n", p.data);
  p.sum = 0;
  p.sum = ip_checksum(&p, sizeof(PACKET));
  if(rand()%3 == 1){
    p.sum = 0;
  }

  if(sendto(server_socket, &p, sizeof(PACKET), 0, (struct sockaddr *) &server_socket_address, server_socket_length)==-1)
    {
        //die("sendto()");
        perror("Sendto.\n");
        exit(EXIT_FAILURE);
    }
}

void OUT_send_syn_ack(){
  printf("OUT: send syn ack\n");
  send_ack(-1, 1, 1, 0);
}

void OUT_send_fin(){
  printf("OUT: send fin\n");
  send_ack(-1, 0, 0, 1);
}

void OUT_send_fin_ack(){
  printf("OUT: send fin ack\n");
  send_ack(-1, 1, 0, 1);
}

int PACKET_ACK(PACKET t){
  return t.seq;
  //return next_ack(server_buf.last_ack);
}

int IS_PACKET_BAD(PACKET t){
  return 0;

  if(ip_checksum(&t, sizeof(PACKET)) == 0){
      printf("CHECKSUM:%d\n", ip_checksum(&t, sizeof(PACKET)));
      return 0;


  }else{
    printf("CHECKSUM:%d\n", ip_checksum(&t, sizeof(PACKET)));
    return 1;
  }
}

int window_size(){
  //printf("WINDOW SIZE(): %d - %d = %d\n", client_buf.seq_1, client_buf.seq_2, client_buf.seq_1-client_buf.seq_2);
  return client_buf.seq_1 - client_buf.seq_2;
}



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
int window = 512;

void start(){

    /* Intializes random number generator */
    time_t t;
    srand((unsigned) time(&t));

    first_ack = 1;

    /* BUFFERS */
    server_buf.seq_0 = 0; //Not used
    server_buf.seq_1 = 0; //Next packet to ack
    server_buf.seq_2 = 0; //Next packet to place on buffer


    client_buf.seq_0 = 0; //Next place to load new packet on buffer
    client_buf.seq_1 = 0; //Next packet to send
    client_buf.seq_2 = 0; //Next packet expecting ACK for

    ack_buf.seq_0 = 0; //Not used
    ack_buf.seq_1 = 0;
    ack_buf.seq_2 = 0;

    int i = 0;
    while (i < MODULO) {
      client_established[i].on = -1;
      i++;
    }

    /* INIT STATE MACHINE VARIABLES */
    state = CLOSED;
    input = NONE;

    /* TIMERS */
    reset_timer(&syn_sent_timer);
    reset_timer(&pre_established_timer);
    reset_timer(&syn_recieved_timer);
    reset_timer(&fin_wait_1_timer);
    reset_timer(&time_wait_timer);
    reset_timer(&closing_timer);
    reset_timer(&close_wait_timer);
    reset_timer(&last_ack_timer);

    /* Start thread recieving replies from server */
    if(pthread_create(&tid, NULL, STATE_MACHINE, 0) != 0){
      perror("Could not start thread.\n");
      exit(EXIT_FAILURE);
    }
}

/* USER FUNCTIONS */
void u_connect(){
    input = CONNECT;
}

void u_listen(){
    input = LISTEN;
}

void u_close(){
  input = CLOSE;
}

/* MAIN */
int main(int argc, char *argv[]){
  if(argc == 2){
    /* START AS SERVER */
    if(!strcmp(argv[1], "server")){
      is_server = 1;
        printf("<<<<<<SERVER>>>>>>:\n");

        if ((server_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            //die("socket");
            perror("Socket.\n");
            exit(EXIT_FAILURE);
        }
        // zero out the structure
        memset((char *) &server_socket_self_address, 0, sizeof(server_socket_self_address));

        server_socket_self_address.sin_family = AF_INET;
        server_socket_self_address.sin_port = htons(SERVER_PORT);
        server_socket_self_address.sin_addr.s_addr = htonl(INADDR_ANY);

        //bind socket to port
        if( bind(server_socket , (struct sockaddr*)&server_socket_self_address, sizeof(server_socket_self_address) ) == -1)
        {
            //die("bind");
            perror("Bind.\n");
            exit(EXIT_FAILURE);
        }


        start();
        sleep(1);
        input = LISTEN;
        recieve_packet();
        //sleep(1);
        //input = SYN;
        //sleep(1);
        //input = ACK;
        //sleep(1);

        //sleep(1);

        /*input = FIN;
        sleep(1);
        input = CLOSE;
        sleep(1);
        input = ACK;
        sleep(1);
        input = EXIT;*/
        getchar();


    }else if(!strcmp(argv[1], "client")){
      is_server = 0;
        /* START AS CLIENT */
        printf("<<<<<<CLIENT>>>>>>:\n");
        //Connect client
        if ((server_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            {
                //die("socket");
                perror("Socket.\n");
                exit(EXIT_FAILURE);
            }
        memset((char *) &server_socket_address, 0, sizeof(server_socket_address));
        server_socket_address.sin_family = AF_INET;
        server_socket_address.sin_port = htons(SERVER_PORT);

        // zero out the structure
        memset((char *) &client_address, 0, sizeof(client_address));
        client_address.sin_family = AF_INET;
        client_address.sin_port = htons(SERVER_PORT+1);
        client_address.sin_addr.s_addr = htonl(INADDR_ANY);
        //bind socket to port
        if( bind(server_socket , (struct sockaddr*)&client_address, sizeof(client_address) ) == -1)
        {
            //die("bind");
            perror("Bind.\n");
            exit(EXIT_FAILURE);
        }

        if (inet_aton(SERVER_IP , &server_socket_address.sin_addr) == 0)
            {

                fprintf(stderr, "inet_aton() failed\n");
                exit(1);
            }
        //Run machine
        start();
        sleep(1);


        input = CONNECT;
        sleep(1);

        //input = SYN_ACK;
        //sleep(1);

        send_packet("MSG1");
        send_packet("MSG2");
        send_packet("MSG3");
        send_packet("MSG4");

        /* Start thread recieving replies from server */
        if(pthread_create(&tid2, NULL, recieve_ack, NULL) != 0){
          perror("Could not start thread.\n");
          exit(EXIT_FAILURE);
        }


        sleep(10);


        input = CLOSE;
        /*sleep(1);

        input = ACK;
        sleep(1);

        input = FIN;
        sleep(1);

        input = EXIT;*/

        getchar();

    }
  }else{
      /* EXIT */
      printf("INVALID\n");
      return 0;
  }

  return 1;
}

/* STATE MACHINE IMPLEMENTATION */
void * STATE_MACHINE(void *arg){
      /* RUN */
      while(FOREVER) {
        //printf("seq1:%d - seq2:%d = winsize:%d\n", client_buf.seq_1, client_buf.seq_2, client_buf.seq_1-client_buf.seq_2);
        /* STATE CHECK */
        switch(state){

            /*=============*/
            /*STATE: CLOSED*/
            case CLOSED:{
                //printf("\nSTATE = CLOSED\n");
                /*INPUT: CONNECT*/
                if(input == EXIT){
                  printf("CLOSED >> IN: EXIT\n");
                  input = NONE;
                  state = EXITING;
                }else if(input == CONNECT){
                  printf("CLOSED >> IN: CONNECT\n");
                    input = NONE;
                    //Send SYN
                    OUT_send_syn();
                    //GO TO: SYN_SENT
                    state = SYN_SENT;
                }else if(input == LISTEN){
                    input = NONE;
                    state = LISTENING;
                }
            } break;

            /*=============*/
            /*STATE: SYN_SENT*/
            case SYN_SENT:{
                //printf("\nSTATE = SYN_SENT\n");
                if(input == SYN_ACK){
                  printf("SYN_SENT >> IN: SYN_ACK\n");
                    input = NONE;
                    //Send ACK
                    OUT_send_ack(-1);
                    //GO TO: PRE_ESTABLISHED
                    state = PRE_ESTABLISHED;
                    reset_timer(&syn_sent_timer);
                }else{
                    if(syn_sent_timer.on == -1){
                        syn_sent_timer.on = 3;
                        syn_sent_timer.start = clock();
                        syn_sent_timer.length = clock_time(10);
                    }else{
                        if(timeout(syn_sent_timer) == 1){
                            OUT_send_syn();
                            syn_sent_timer.on--;
                        }
                        if(syn_sent_timer.on == 0){
                            reset_timer(&syn_sent_timer);
                            state = CLOSED;
                            printf("CLOSING\n");
                        }
                    }
                }
            } break;

            /*=============*/
            /*STATE: PRE_ESTABLISHED*/
            case PRE_ESTABLISHED:{
                //printf("\nSTATE = PRE_ESTABLISHED\n");
                if(input == SYN_ACK){
                  printf("PRE_ESTABLISHED >> IN: SYN_ACK\n");
                    input = NONE;
                    OUT_send_ack(-1);
                }else{
                    if(pre_established_timer.on == -1){
                        pre_established_timer.on = 1;
                        pre_established_timer.start = clock();
                        pre_established_timer.length = clock_time(10);
                    }else{
                        if(timeout(pre_established_timer) == 1){
                            state = ESTABLISHED_CLIENT;
                            reset_timer(&pre_established_timer);
                        }
                    }
                }
            } break;

            /*=============*/
            /*STATE: ESTABLISHED_CLIENT*/
            case ESTABLISHED_CLIENT:{
              //printf("\nSTATE = ESTABLISHED_CLIENT\n");
              if(input == CLOSE){
                printf("ESTABLISHED_CLIENT >> IN: CLOSE\n");
                input = NONE;
                state = FIN_WAIT_1;
                OUT_send_fin();
              }else{

                if(ack_buf.seq_1 < ack_buf.seq_2 || ack_buf.seq_1 > ack_buf.seq_2){
                  //printf("-----READING ACK BUF---seq_1:%d < seq_2:%d----\n", ack_buf.seq_1, ack_buf.seq_2);
                  if(ack_buf.packet[ack_buf.seq_1].seq == client_buf.seq_2){
                    //printf("-----ACCEPTABLE ACK-------\n");
                    //printf("ACKBUF.seq:%d == client_buf.seq_2:%d\n", ack_buf.packet[ack_buf.seq_1].seq, client_buf.seq_2);
                    //printf("RECV ACK %d;\n", client_buf.seq_2);
                    reset_timer(&client_established[client_buf.seq_2]);
                    client_buf.seq_2 = next_seq(client_buf.seq_2);
                    //ack_buf.seq_1 = next_seq(ack_buf.seq_1);

                  }
                  ack_buf.seq_1 = next_seq(ack_buf.seq_1);
                }

                if(client_buf.seq_0 > client_buf.seq_1 && window_size() < WINDOW){
                  //printf("WINDOW: %d < %d\n", window_size(), WINDOW);
                  OUT_send_packet(client_buf.packet[client_buf.seq_1]);
                  if(client_established[client_buf.seq_1].on == -1){
                    client_established[client_buf.seq_1].on = 3;
                  }else{
                    decrease_timer(&client_established[client_buf.seq_1]);
                    if(client_established[client_buf.seq_1].on == 0){
                      input = CLOSE;
                      printf("PACKET NOT ACKED -> TIMEOUR -> CLOSE()\n");
                    }
                  }
                  client_established[client_buf.seq_1].start = clock();
                  client_established[client_buf.seq_1].length = clock_time(10);
                  //printf("SET TIMER: %d; ON=%d;\n", client_buf.seq_1, client_established[client_buf.seq_1].on);
                  client_buf.seq_1 = next_seq(client_buf.seq_1);
                }else{


                }


                int i;
                i = client_buf.seq_2;
                while(i < client_buf.seq_1){
                  if(timeout(client_established[i]) == 1){
                    //printf("PACK_TIMEOUT: %d; ON=%d;\n", i, client_established[i].on);
                    client_buf.seq_1 = i;
                  }
                  i++;
                }


              }
            } break;

            /*=============*/
            /*STATE: LISTENING*/
            case LISTENING:{
                //printf("\nSTATE = LISTENING\n");
                if(input == SYN){
                  printf("LISTENING >> IN: SYN\n");
                    input = NONE;
                    OUT_send_syn_ack();
                    state = SYN_RECIEVED;
                }else if(input == CLOSE){
                  printf("LISTENING >> IN: CLOSE\n");
                    input = NONE;
                    state = CLOSE;
                }
            } break;

            /*=============*/
            /*STATE: SYN_RECIEVED*/
            case SYN_RECIEVED:{
                //printf("\nSTATE = SYN_RECIEVED\n");
                if(input == ACK){
                  printf("SYN_RECIEVED >> IN: ACK\n");
                  input = NONE;
                  state = ESTABLISHED_SERVER;
                  reset_timer(&syn_recieved_timer);
                }else if(input == RESET){
                  printf("SYN_RECIEVED >> IN: RESET\n");
                  input = NONE;
                  state = LISTENING;
                  reset_timer(&syn_recieved_timer);
                }else{
                  if(syn_recieved_timer.on == -1){
                    syn_recieved_timer.on = 3;
                    syn_recieved_timer.start = clock();
                    syn_recieved_timer.length = clock_time(10);
                  }else{
                    if(timeout(syn_recieved_timer) == 1){
                        OUT_send_syn_ack();
                        syn_recieved_timer.on--;
                    }
                    if(syn_recieved_timer.on == 0){
                      state = LISTEN;
                      reset_timer(&syn_recieved_timer);
                    }
                  }
                }
            } break;

            /*=============*/
            /*STATE: ESTABLISHED_SERVER*/
            case ESTABLISHED_SERVER:{
                //printf("\nSTATE = ESTABLISHED_SERVER\n");
              if(input == FIN){
                printf("ESTABLISHED_SERVER >> IN: FIN\n");
                input = NONE;
                OUT_send_ack(-1);
                state = CLOSE_WAIT;
              }else{
                if(server_buf.seq_1 < server_buf.seq_2){
                  if(IS_PACKET_BAD(server_buf.packet[server_buf.seq_1]) == 1 || server_buf.packet[server_buf.seq_1].seq != server_buf.seq_1){
                    if(first_ack == 0){
                      printf("ESTABLISHED_SERVER >> discard old packet :ack %d;\n", prev_ack(server_buf.seq_1));
                      OUT_send_ack(prev_ack(server_buf.seq_1));
                    }
                  }else{
                    printf("ESTABLISHED_SERVER >> new packet\n");
                    printf("RECV: DATA=%s;\n", server_buf.packet[server_buf.seq_1].data);
                    OUT_send_ack(PACKET_ACK(server_buf.packet[server_buf.seq_1]));
                    server_buf.seq_1 = next_seq(server_buf.seq_1);
                    first_ack = 0;
                  }
                }
              }
            } break;

            /*=============*/
            /*STATE: FIN_WAIT_1 */
            case FIN_WAIT_1:{
                //printf("\nSTATE = FIN_WAIT_1\n");
              if(input == ACK){
                printf("FIN_WAIT_1 >> IN: ACK\n");
                input = NONE;
                state = FIN_WAIT_2;
              }else if(input == FIN_ACK){
                printf("FIN_WAIT_1 >> IN: FIN_ACK\n");
                input = NONE;
                OUT_send_ack(-1);
                state = TIME_WAIT;
              }else if(input == FIN){
                printf("FIN_WAIT_1 >> IN: FIN\n");
                input == NONE;
                OUT_send_ack(-1);
                state = CLOSING;
              }else{
                if(fin_wait_1_timer.on == -1){
                  fin_wait_1_timer.on = 3;
                  fin_wait_1_timer.start = clock();
                  fin_wait_1_timer.length = clock_time(10);
                }else{
                  if(timeout(fin_wait_1_timer) == 1){
                    decrease_timer(&fin_wait_1_timer);
                    OUT_send_fin();
                  }
                  if(fin_wait_1_timer.on == 0){
                    state = TIME_WAIT;
                  }
                }
              }

            } break;

            /*=============*/
            /* STATE: FIN_WAIT_2 */
            case FIN_WAIT_2:{
               // printf("\nSTATE = FIN_WAIT_2\n");
              if(input == FIN){
                printf("FIN_WAIT_2 >> IN: FIN\n");
                input = NONE;
                OUT_send_ack(-1);
                state = TIME_WAIT;
              }
            } break;

            /*=============*/
            /*STATE: TIME_WAIT*/
            case TIME_WAIT:{
                //printf("\nSTATE = TIME_WAIT\n");
              if(input == FIN){
                printf("TIME_WAIT >> IN: FIN\n");
                input = NONE;
                OUT_send_ack(-1);
              }else if(input == FIN_ACK){
                printf("TIME_WAIT >> IN: FIN_ACK\n");
                input = NONE;
                OUT_send_ack(-1);
              }else{
                if(time_wait_timer.on == -1){
                  time_wait_timer.on = 1;
                  time_wait_timer.start = clock();
                  time_wait_timer.length = clock_time(10);
                }else{
                  if(timeout(time_wait_timer) == 1){
                    state = CLOSED;
                    printf("CLOSING\n");
                  }
                }
              }
            } break;

            /*=============*/
            /*STATE: CLOSING*/
            case CLOSING:{
               // printf("\nSTATE = CLOSING\n");
              if(input == ACK){
                printf("CLOSING >> IN: ACK\n");
                input = NONE;
                state = TIME_WAIT;
              }else{
                if(closing_timer.on == -1){
                  closing_timer.on = 3;
                  closing_timer.start = clock();
                  closing_timer.length = clock_time(10);
                }else{
                  if(timeout(closing_timer) == 1){
                    decrease_timer(&closing_timer);
                    OUT_send_ack(-1);
                  }
                  if(closing_timer.on == 0){
                    closing_timer.on = -1;
                    state = TIME_WAIT;
                  }
                }
              }
            } break;

            /*=============*/
            /*STATE: CLOSE_WAIT*/
            case CLOSE_WAIT:{
                //printf("\nSTATE = CLOSE_WAIT\n");
              if(input == FIN){
                printf("CLOSE_WAIT >> IN: FIN\n");
                input = NONE;
                OUT_send_ack(-1);
              }else if(close_wait_timer.on == -1){
                close_wait_timer.on = 1;
                close_wait_timer.start = clock();
                close_wait_timer.length = clock_time(10);
              }else{
                if(timeout(close_wait_timer) == 1){
                  close_wait_timer.on = -1;
                  state = LAST_ACK;
                  OUT_send_fin();
                }
              }
            } break;

            /*=============*/
            /*STATE: LAST_ACK*/
            case LAST_ACK:{
              //printf("\nSTATE = LAST_ACK\n");
              if(input == ACK){
                printf("LAST_ACK >> IN: ACK\n");
                input = NONE;
                state = CLOSED;
                printf("CLOSING\n");
              }else{
                if(last_ack_timer.on == -1){
                  last_ack_timer.on = 3;
                  last_ack_timer.start = clock();
                  last_ack_timer.length = clock_time(10);
                }else{
                  if(timeout(last_ack_timer) == 1){
                    decrease_timer(&last_ack_timer);
                    OUT_send_fin();
                  }
                  if(last_ack_timer.on == 0){
                    state = CLOSED;
                    printf("CLOSING\n");
                    reset_timer(&last_ack_timer);
                  }
                }
              }
            } break;

            /*=============*/
            /*STATE: EXITING*/
            case EXITING:{
              printf("\nSTATE = EXITING\n");
              close(server_socket);
              return NULL;
            } break;

        }
        usleep(1000*100);
      }

}
