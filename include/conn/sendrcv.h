/*
 *
 *  sendrcv.h
 *
 *  Exposes a simple connection wrapping a RDMA queue pair and allows send and 
 *  receive operations on it.
 *
 */

#ifndef SENDRCV_H_
#define SENDRCV_H_

#include <stddef.h>

/*
*  Connection  using send and receive
*/
typedef struct sendRcvConn *SendRcvConn;

/*
 *  Constructor 
 */
SendRcvConn sr_conn_init();

/*
 *  Destructor
 */
void sr_conn_close(SendRcvConn);


int sr_conn_dial(SendRcvConn conn, char* ip, int port);

/*
 * Sends the content of the buffer to remote
 */
int sr_conn_write(SendRcvConn, void *buf, size_t len);

/*
 * Receives to buffer
 */
int sr_conn_read(SendRcvConn, void *buf, size_t len);


/*
*  Listener  using send and receive
*/
typedef struct sendRcvListener *SendRcvListener;

SendRcvListener sr_listener_init();

void sr_listener_close(SendRcvListener);

int sr_listener_listen(SendRcvListener, char* ip, int port);

int sr_listener_accept(SendRcvListener, SendRcvConn);

#endif /* SENDRCV_H_ */
