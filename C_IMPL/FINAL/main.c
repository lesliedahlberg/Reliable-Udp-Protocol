#include "transport.h"

  /* USER DATA PROCESSING */
  /* ==================== */

  void processData(char* data){
    printf("<<%s>>\n", data);
  }

  /* MAIN */
  /* ==== */
  int main(int argc, char *argv[]){



    if(argc >= 2){
      /* START AS SERVER */
      if(!strcmp(argv[1], "server")){
          u_start();
          u_listen(argv[2]);
          u_set_rcvr(&processData);
          u_start_recieving();
          while(state != EXITING){
            getchar();
          }

      }else if(!strcmp(argv[1], "client")){
          /* START AS CLIENT */


          u_start();
          u_connect(argv[2]);
          u_send("Archives (static libraries) are acted upon differently than are shared objects (dynamic libraries). With dynamic libraries, all the library symbols go into the virtual address space of the output file, and all the symbols are available to all the other files in the link. In contrast, static linking only looks through the archive for the undefined symbols presently known to the loader at the time the archive is processed.", sizeof("Archives (static libraries) are acted upon differently than are shared objects (dynamic libraries). With dynamic libraries, all the library symbols go into the virtual address space of the output file, and all the symbols are available to all the other files in the link. In contrast, static linking only looks through the archive for the undefined symbols presently known to the loader at the time the archive is processed."));
          u_prep_sending();
          sleep(20);
          u_close();
          sleep(5);
          u_exit();

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
