#DEFINE FOREVER 1;
#DEFINE MODULO 1024;
#DEFINE WINDOW 512;
#DEFINE WINDOW_MODULO 1024;

/* PACKET */
typedef struct {
  int ACK;
  int FIN;
  int SYN;
  int SUM;
  char data[32];
} PACKET;

/* INCOMING BUFFER */
typedef struct {
  int seq_0;
  int seq_1;
  int seq_2;
  int last_ack;
  PACKET packet[MODULO];
} BUFFER;


/* INIT. INCOMING BUFFER */
BUFFER server_buf;
server_buf.seq_0 = 0;
server_buf.seq_1 = 0;
server_buf.seq_2 = 0;

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
  if(ack+1 > WINDOW_SEQ - 1){
    return 0;
  }else{
    return ack+1;
  }
}

/* TIMER */
typedef struct {
    int on;
    clock_t start;
    clock_t length;
} TIMER;

/* CHECK TIMER */
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

/* MILLISECONDS TO CLOCK TIME */
clock_t clock_time(int milli_seconds){
    return ((float) milli_seconds)/1000f*CLOCKS_PER_SEC;
}

/* INPUTs */
typedef enum {
  NONE,
  CONNECT,
  CLOSE,
  SYN_ACK,
  SYN,
  ACK,
  FIN,
  TIMEOUT,
  RESET,
  GOOD_PACKET,
  BAD_PACKET,
} INPUT;

/* STATES */
typedef enum {
  CLOSED,
  SYN_SENT,
  PRE_ESTABLISHED,
  ESTABLISHED_CLIENT,
  LISTEN,
  SYN_RECIEVED,
  ESTABLISHED_SERVER,
  FIN_WAIT_1,
  FIN_WAIT_2,
  TIME_WAIT,
  CLOSING,
  CLOSE_WAIT,
  LAST_ACK
} STATE;

/* INIT STATE MACHINE VARIABLES */
STATE state = CLOSED;
INPUT input = NONE;

/* SET TIMERS */
TIMER syn_sent_timer;
syn_sent_timer.on = -1;

TIMER pre_established_timer;
pre_established_timer.on = -1;

TIMER syn_recieved_timer;
syn_recieved_timer.on = -1;

void start(){
    STATE_MACHINE();
}

void connect(){
    input = CONNECT

}

void STATE_MACHINE(){
      while(FOREVER) {
        switch(state){

            /*STATE: CLOSED*/
            case CLOSED:{
                /*INPUT: CONNECT*/
                if(input == CONNECT){
                    input = NONE;
                    //Send SYN
                    OUT_send_syn();
                    //GO TO: SYN_SENT
                    state = SYN_SENT
                }else if(input == LISTEN){
                    input = NONE;
                    state = LISTEN;
                }
            } break;

            /*STATE: SYN_SENT*/
            case SYN_SENT:{
                if(input == SYN_ACK){
                    input = NONE;
                    //Send ACK
                    OUT_send_ack();
                    //GO TO: PRE_ESTABLISHED
                    state = PRE_ESTABLISHED;
                    syn_sent_time.on = -1;
                }else{
                    if(syn_sent_timer.on == -1){
                        syn_sent_timer.on = 3;
                        syn_sent_timer.start = clock();
                        syn_sent_timer.length = clock_time(10);
                    }else{
                        if(timeout(syn_sent_timer) == 1){
                            if(syn_sent_time.on > 0){
                                OUT_send_syn();
                                syn_sent_timer.on--;
                            }else{
                                syn_sent_time.on = -1;
                                state = CLOSED;
                            }
                        }
                    }
                }
            } break;

            /*STATE: PRE_ESTABLISHED*/
            case PRE_ESTABLISHED:{
                if(input == SYN_ACK){
                    input = NONE;
                    OUT_send_ack();
                }else{
                    if(pre_established_timer.on == -1){
                        pre_established_timer.on = 1
                        pre_established_timer.start = clock();
                        pre_established_timer.length = clock_time(10);
                    }else{
                        if(timeout(pre_established_timer) == 1){
                            state = ESTABLISHED_CLIENT;
                            pre_established_timer.on = -1;
                        }
                    }
                }
            } break;

            /*STATE: ESTABLISHED_CLIENT*/
            case ESTABLISHED_CLIENT:{

            } break;

            /*STATE: LISTEN*/
            case LISTEN:{
                if(input == SYN){
                    input = NONE;
                    OUT_send_syn_ack();
                    state = SYN_RECIEVED;
                }else if(input == CLOSE){
                    input = NONE;
                    state = CLOSE;
                }
            } break;

            /*STATE: SYN_RECIEVED*/
            case SYN_RECIEVED:{
                if(input == ACK){
                  input = NONE;
                  state = ESTABLISHED_SERVER;
                  syn_recieved_timer.on = -1;
                }else if(input == RESET){
                  input = NONE;
                  state = LISTEN;
                  syn_recieved_timer.on = -1;
                }else{
                  if(syn_recieved_timer.on == -1){
                    syn_recieved_timer.on = 3;
                    syn_recieved_timer.start = clock();
                    syn_recieved_timer.length = clock_time(10);
                  }else{
                    if(timeout(syn_recieved_timer) == 1){
                      if(syn_recieved_timer.on > 0){
                        OUT_send_syn_ack();
                        syn_recieved_timer.on--;
                      }else{
                        state = LISTEN;
                        syn_recieved_timer.on = -1;
                      }
                    }
                  }
                }
            } break;

            /*STATE: ESTABLISHED_SERVER*/
            case ESTABLISHED_SERVER:{
              if(server_buf.seq_1 < server_buf.seq_2){
                if(IS_PACKET_BAD(server_buf.packet[server_buf.seq_1]) == 1 || PACKET_ACK(server_buf.packet[server_buf.seq_1]) != next_ack(server_buf.last_ack)){
                  OUT_send_ack(server_buf.last_ack);
                }else{
                  server_buf.seq_1 = next_seq(server_buf.seq_1);
                  OUT_send_ack(next_ack(server_buf.last_ack));
                  server_buf.last_ack++;
                }
              }
            } break;

            /*STATE: FIN_WAIT1 */
            case FIN_WAIT1:{

            } break;

            /* STATE: FIN_WAIT_2 */
            case FIN_WAIT2:{

            } break;
            /*STATE: TIME_WAIT*/
            case TIME_WAIT:{

            } break;

            /*STATE: CLOSING*/
            case CLOSING:{

            } break;

            /*STATE: CLOSE_WAIT*/
            case CLOSE_WAIT:{

            } break;

            /*STATE: LAST_ACK*/
            case LAST_ACK:{

            } break;

        }
        sleep(1);
      }
}
