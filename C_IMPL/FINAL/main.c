/* PROGRAM: transport_protocol
 * AUTHOR: Leslie Dahlberg, Jonathan Larsson
 * EMAIL: ldg14001@student.mdh.se, jln14010@student.mdh.se
 * DATE: 2016-02-10
 * File: main.c
 * Test program for reliable UDP library transport.h
 * USAGE:
 * 1. For server:
 *  - ./transport_protocol server <SERVER_IP>
 * 2. For client:
 *  - ./transport_protocol client <SERVER_IP>
 */


#include "transport.h"

  /* USER DATA PROCESSING */
  /* ==================== */

  void processData(char* data){
    printf("<<RCVD BY APP. LAYER: %.*s>>\n", PACKET_DATA_SIZE, data);
  }

  /* MAIN */
  /* ==== */
  int main(int argc, char *argv[]){

    if(argc >= 2){
      /* START AS SERVER */
      if(!strcmp(argv[1], "server")){
          //Start threads for recieving data
          u_start();
          //Connect UDP sockets
          u_listen(argv[2]);
          //Set function for recieving data
          u_set_rcvr(&processData);
          //Start listening to connection
          u_start_recieving();
          sleep(25);
          //Shut down threads
          //u_exit();
          while(state != EXITING){
            getchar();
          }


      }else if(!strcmp(argv[1], "client")){
          /* START AS CLIENT */

          //Start threads for sending data
          u_start();
          //Connect UDP socket
          u_connect(argv[2]);
          //Place data onto buffer
          u_send("Archives (static libraries) are acted upon differently than are shared objects (dynamic libraries). With dynamic libraries, all the library symbols go into the virtual address space of the output file, and all the symbols are available to all the other files in the link. In contrast, static linking only looks through the archive for the undefined symbols presently known to the loader at the time the archive is processed.", sizeof("Archives (static libraries) are acted upon differently than are shared objects (dynamic libraries). With dynamic libraries, all the library symbols go into the virtual address space of the output file, and all the symbols are available to all the other files in the link. In contrast, static linking only looks through the archive for the undefined symbols presently known to the loader at the time the archive is processed."));
          //Start sliding window
          u_prep_sending();
          sleep(20);
          //Close connection
          u_close();
          sleep(5);
          //Shut down threads
          //u_exit();

          while(state != EXITING){
            getchar();
          }
          //u_close();
      }
    }else{
        /* EXIT */
        printf("INVALID ARGUMENTS\n");
        return 0;
    }

    return 1;
  }
