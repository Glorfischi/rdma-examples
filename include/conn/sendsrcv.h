/*
 *
 *  sendrcv.h
 *
 *  Exposes a simple connection wrapping a RDMA queue pair and allows send and 
 *  receive operations on it.
 *
 */

#ifndef SENDSRCV_H_
#define SENDSRCV_H_

#include <stddef.h>

/*
*  Connection using send and shared receive queue
*/
typedef struct sendSharedRcvConn *SendSharedRcvConn;

/*
 *  Constructor 
 */
SendSharedRcvConn ssr_conn_init();

/*
 *  Destructor
 */
void ssr_conn_close(SendSharedRcvConn);


int ssr_conn_dial(SendSharedRcvConn conn, char* ip, int port);

/*
 * Sends the content of the buffer to remote
 */
int ssr_conn_write(SendSharedRcvConn, void *buf, size_t len);

/*
 * Receives to buffer
 */
int ssr_conn_read(SendSharedRcvConn, void *buf, size_t len);


/*
*  Listener  using send and receive
*/
typedef struct sendSharedRcvListener *SendSharedRcvListener;

SendSharedRcvListener ssr_listener_init();

void ssr_listener_close(SendSharedRcvListener);

int ssr_listener_listen(SendSharedRcvListener, char* ip, int port);

int ssr_listener_accept(SendSharedRcvListener, SendSharedRcvConn);

#endif /* SENDRCV_H_ */
