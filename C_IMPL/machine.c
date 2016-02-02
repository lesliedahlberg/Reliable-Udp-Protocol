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

#define FOREVER 1
#define MODULO 1024
#define WINDOW_MODULO 1024

/* THREADs */
pthread_t tid;

/* PACKET */
typedef struct {
  int ACK;
  int FIN;
  int SYN;
  int SUM;
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
void STATE_MACHINE();
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
  if(ack+1 > WINDOW_MODULO - 1){
    return 0;
  }else{
    return ack+1;
  }
}

/* TIMER FUNCTIONS */
int timeout(TIMER t){
    if(t.start-clock() >= t.length){
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
    return ((float) milli_seconds)/1000.0f*CLOCKS_PER_SEC;
}

/* OUT FUNCTIONS */
void OUT_send_ack(){
  printf("OUT: send ack\n");
}

void OUT_send_syn(){
  printf("OUT: send syn\n");
}

void OUT_send_packet(){
  printf("OUT: send packet\n");
}

void OUT_send_syn_ack(){
  printf("OUT: send syn ack\n");
}

void OUT_send_fin(){
  printf("OUT: send fin\n");
}

void OUT_send_fin_ack(){
  printf("OUT: send fin ack\n");
}

int PACKET_ACK(){
  return 0;
}

int IS_PACKET_BAD(){
  return 0;
}

int window_size(){
  return 1;
}

/* INPUTs */
typedef enum {
  NONE,CONNECT,CLOSE,SYN_ACK,SYN,ACK,FIN,FIN_ACK,TIMEOUT,RESET,GOOD_PACKET,BAD_PACKET
} INPUT;

/* STATES */
typedef enum {
  CLOSED,SYN_SENT,PRE_ESTABLISHED,ESTABLISHED_CLIENT,LISTEN,SYN_RECIEVED,ESTABLISHED_SERVER,FIN_WAIT_1,FIN_WAIT_2,TIME_WAIT,CLOSING,CLOSE_WAIT,LAST_ACK
} STATE;

/* STATE MACHINE */
STATE state;
INPUT input;

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
    /* BUFFERS */
    server_buf.seq_0 = 0;
    server_buf.seq_1 = 0;
    server_buf.seq_2 = 0;

    client_buf.seq_0 = 0; //Next place to load new packet on buffer
    client_buf.seq_1 = 0; //Next packet to send
    client_buf.seq_2 = 0; //Next packet expecting ACK for

    ack_buf.seq_0 = 0; //Not used
    ack_buf.seq_1 = 0;
    ack_buf.seq_2 = 0;

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
    if(pthread_create(&tid, NULL, STATE_MACHINE, NULL) != 0){
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

    /* START AS SERVER */
    if(strcmp(argv[1], "server")){
        printf("<<<<<<SERVER>>>>>>:\n");
        start();
        sleep(1);
        input = LISTEN;
        sleep(1);
        input = SYN;
        sleep(1);
        input = ACK;
        sleep(1);
        input = FIN;
        sleep(1);
        input = CLOSE;
        sleep(1);
        input = ACK;
        sleep(1);


    }else if(strcmp(argv[1], "client")){
        /* START AS CLIENT */
        printf("<<<<<<CLIENT>>>>>>:\n");
        start();
        sleep(1);

        input = CONNECT;
        sleep(1);

        input = CLOSE;
        sleep(1);

        input = ACK;
        sleep(1);

        input = FIN;
        sleep(1);

    }else{
        /* EXIT */
        printf("INVALID\n");
        return 0;
    }
}

/* STATE MACHINE IMPLEMENTATION */
void STATE_MACHINE(){
      /* RUN */
      while(FOREVER) {
        /* STATE CHECK */
        switch(state){

            /*=============*/
            /*STATE: CLOSED*/
            case CLOSED:{
                printf("STATE = CLOSED\n");
                /*INPUT: CONNECT*/
                if(input == CONNECT){
                    input = NONE;
                    //Send SYN
                    OUT_send_syn();
                    //GO TO: SYN_SENT
                    state = SYN_SENT;
                }else if(input == LISTEN){
                    input = NONE;
                    state = LISTEN;
                }
            } break;

            /*=============*/
            /*STATE: SYN_SENT*/
            case SYN_SENT:{
                printf("STATE = SYN_SENT\n");
                if(input == SYN_ACK){
                    input = NONE;
                    //Send ACK
                    OUT_send_ack();
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
                        }
                    }
                }
            } break;

            /*=============*/
            /*STATE: PRE_ESTABLISHED*/
            case PRE_ESTABLISHED:{
                printf("STATE = PRE_ESTABLISHED\n");
                if(input == SYN_ACK){
                    input = NONE;
                    OUT_send_ack();
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
              printf("STATE = ESTABLISHED_CLIENT\n");
              if(input == CLOSE){
                input = NONE;
                state = FIN_WAIT_1;
                OUT_send_fin();
              }else{
                int i;
                i = client_buf.seq_2;
                while(i < client_buf.seq_1){
                  if(timeout(client_established[i]) == 1){
                    client_buf.seq_1 = i;
                  }
                }
                if(client_buf.seq_0 > client_buf.seq_1 && window_size() < window){
                  OUT_send_packet(client_buf.packet[client_buf.seq_1]);
                  client_buf.seq_1 = next_seq(client_buf.seq_1);
                  client_established[client_buf.seq_1].on = 1;
                  client_established[client_buf.seq_1].start = clock();
                  client_established[client_buf.seq_1].length = clock_time(10);
                }else{
                  if(ack_buf.seq_1 > ack_buf.seq_2 || ack_buf.seq_1 < ack_buf.seq_2){
                    if(PACKET_ACK(ack_buf.seq_2) == client_buf.last_ack+1){
                      client_buf.last_ack = next_ack(client_buf.last_ack);
                      reset_timer(&client_established[client_buf.seq_2]);
                      client_buf.seq_2 = next_seq(client_buf.seq_2);
                    }
                  }
                }
              }

            } break;

            /*=============*/
            /*STATE: LISTEN*/
            case LISTEN:{
                printf("STATE = LISTEN\n");
                if(input == SYN){
                    input = NONE;
                    OUT_send_syn_ack();
                    state = SYN_RECIEVED;
                }else if(input == CLOSE){
                    input = NONE;
                    state = CLOSE;
                }
            } break;

            /*=============*/
            /*STATE: SYN_RECIEVED*/
            case SYN_RECIEVED:{
                printf("STATE = SYN_RECIEVED\n");
                if(input == ACK){
                  input = NONE;
                  state = ESTABLISHED_SERVER;
                  reset_timer(&syn_recieved_timer);
                }else if(input == RESET){
                  input = NONE;
                  state = LISTEN;
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
                printf("State = ESTABLISHED_SERVER");
              if(input == FIN){
                input = NONE;
                OUT_send_ack();
                state = CLOSE_WAIT;
              }else{
                if(server_buf.seq_1 < server_buf.seq_2){
                  if(IS_PACKET_BAD(server_buf.packet[server_buf.seq_1]) == 1 || PACKET_ACK(server_buf.packet[server_buf.seq_1]) != next_ack(server_buf.last_ack)){
                    OUT_send_ack(server_buf.last_ack);
                  }else{
                    server_buf.seq_1 = next_seq(server_buf.seq_1);
                    OUT_send_ack(next_ack(server_buf.last_ack));
                    server_buf.last_ack++;
                  }
                }
              }
            } break;

            /*=============*/
            /*STATE: FIN_WAIT_1 */
            case FIN_WAIT_1:{
                printf("State = FIN_WAIT_1");
              if(input == ACK){
                input = NONE;
                state = FIN_WAIT_2;
              }else if(input == FIN_ACK){
                input = NONE;
                OUT_send_ack();
                state = TIME_WAIT;
              }else if(input == FIN){
                input == NONE;
                OUT_send_ack();
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
                printf("State = FIN_WAIT_2");
              if(input == FIN){
                input = NONE;
                OUT_send_ack();
                state = TIME_WAIT;
              }
            } break;

            /*=============*/
            /*STATE: TIME_WAIT*/
            case TIME_WAIT:{
                printf("State = TIME_WAIT");
              if(input == FIN){
                input = NONE;
                OUT_send_ack();
              }else if(input == FIN_ACK){
                input = NONE;
                OUT_send_ack();
              }else{
                if(time_wait_timer.on == -1){
                  time_wait_timer.on = 1;
                  time_wait_timer.start = clock();
                  time_wait_timer.length = clock_time(10);
                }else{
                  if(timeout(time_wait_timer) == 1){
                    state = CLOSED;
                  }
                }
              }
            } break;

            /*=============*/
            /*STATE: CLOSING*/
            case CLOSING:{
                printf("State = CLOSING");
              if(input == ACK){
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
                    OUT_send_ack();
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
                printf("State = CLOSE_WAIT");
              if(input == FIN){
                input = NONE;
                OUT_send_ack();
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
              printf("State = LAST_ACK");
              if(input == ACK){
                input = NONE;
                state = CLOSED;
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
                    reset_timer(&last_ack_timer);
                  }
                }
              }
            } break;

        }
        sleep(1);
      }
}
