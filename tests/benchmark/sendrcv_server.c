#include "conn/sendrcv.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]){ 
  SendRcvListener listener = sr_listener_init();
  if(sr_listener_listen(listener, "172.17.5.101", 9999)) {
    perror("Listening failed");
    exit(EXIT_FAILURE); 
  }

  SendRcvConn conn = sr_conn_init(); 
  if(sr_listener_accept(listener, conn)) {
    perror("Accept failed");
    exit(EXIT_FAILURE); 
  }

  printf("Connected\n");

  int ret;
  char data[15];
  ret = sr_conn_read(conn, data, 15);
  if (ret < 0){
    perror("Read failed");
    exit(EXIT_FAILURE);
  }
  printf("read %d data %s\n", ret, data);
  ret = sr_conn_read(conn, data, 15);
  if (ret < 0){
    perror("Read failed");
    exit(EXIT_FAILURE);
  }
  printf("read %d data %s\n", ret, data);
  return 0;
}
