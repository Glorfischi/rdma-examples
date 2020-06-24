#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "conn/sendrcv.h"


int main(int argc, char* argv[]){ 
  SendRcvConn conn = sr_conn_init(); 
  if(sr_conn_dial(conn, "172.17.5.101", 9999)) {
    perror("Connecting failed");
    exit(EXIT_FAILURE); 
  }


  printf("Connected\n");

  sleep(1); // To "solve" race condition
  char *data = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  sr_conn_write(conn, data, 15);
  char *data2 = "BBBBBBBBBBBBBBBBBBBBBBB";
  sr_conn_write(conn, data2, 15);
  return 0;
}
