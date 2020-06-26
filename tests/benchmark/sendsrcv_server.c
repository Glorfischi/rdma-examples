#include "conn/sendsrcv.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]){ 
  SendSharedRcvListener listener = ssr_listener_init();
  if(ssr_listener_listen(listener, "172.17.5.101", 9999)) {
    perror("Listening failed");
    exit(EXIT_FAILURE); 
  }

  SendSharedRcvConn conn = ssr_conn_init(); 
  if(ssr_listener_accept(listener, conn)) {
    perror("Accept failed");
    exit(EXIT_FAILURE); 
  }

  printf("Connected\n");

  sleep(5);
  int ret;
  char data[15];
  ret = ssr_conn_read(conn, data, 15);
  if (ret < 0){
    perror("Read failed");
    exit(EXIT_FAILURE);
  }
  printf("read %d data %s\n", ret, data);
  ret = ssr_conn_read(conn, data, 15);
  if (ret < 0){
    perror("Read failed");
    exit(EXIT_FAILURE);
  }
  printf("read %d data %s\n", ret, data);
  return 0;
}
