#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>
#include <string.h>

#include "conn/sendrcv.h"

struct sendRcvConn{
  struct rdma_addrinfo *addrinfo;

  struct rdma_cm_id* id;
  struct ibv_qp * qp;


  struct ibv_mr *mr;
};

struct sendRcvListener{
  struct rdma_cm_id* id;
};

SendRcvListener sr_listener_init(){
  SendRcvListener listener = calloc(1, sizeof(struct sendRcvListener));
  return listener;
}

int sr_listener_listen(SendRcvListener listener,  char *ip, int port) {
  assert(ip != NULL);
  assert(listener != NULL);
  // If we do not specify a port fallback to default port 18515
  if (port == 0) {
    port = 18515;
  }

  int ret;
  struct rdma_addrinfo hints;
  struct rdma_addrinfo *addrinfo;

  memset(&hints, 0, sizeof hints);
  hints.ai_port_space = RDMA_PS_TCP;
  hints.ai_flags = RAI_PASSIVE;
       
  char strport[80];
  sprintf(strport, "%d", port);

  ret = rdma_getaddrinfo(ip, strport, &hints, &addrinfo);
  if (ret) {
    return ret;
  }

  ret = rdma_create_ep(&listener->id, addrinfo, NULL, NULL);
  if (ret) {
    return ret;
  }

  rdma_freeaddrinfo(addrinfo);
  
  return rdma_listen(listener->id, 2);
}

int sr_listener_accept(SendRcvListener listener, SendRcvConn conn){
  assert(listener != NULL);
  assert(conn != NULL);

  int ret;
  struct rdma_cm_id *conn_id;

  ret = rdma_get_request(listener->id, &conn_id);
  if(ret){
    return ret;
  }
  
  // ToDo: Move to init
  struct ibv_qp_init_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.cap.max_send_wr = 1;  // The maximum number of outstanding Work Requests that can be
  //posted to the Send Queue in that Queue Pair
  attr.cap.max_recv_wr = 2;  // The maximum number of outstanding Work Requests that can be
  //posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_send_sge = 1; // The maximum number of scatter/gather elements in any Work 
  //Request that can be posted to the Send Queue in that Queue Pair.
  attr.cap.max_recv_sge = 1; // The maximum number of scatter/gather elements in any 
  // Work Request that can be posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_inline_data = 0;
  attr.qp_type = IBV_QPT_RC;

  ret = rdma_create_qp(conn_id, NULL, &attr);
  if (ret) {
    return ret;
  }

  // ToDo: Move to init
  struct rdma_conn_param conn_param;
  memset(&conn_param, 0 , sizeof(conn_param));
  conn_param.responder_resources = 0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will accept from the remote side.
  conn_param.initiator_depth =  0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will have to the remote side.
  conn_param.retry_count = 3;  
  conn_param.rnr_retry_count = 3; 

  conn->id = conn_id;
  conn->qp = conn_id->qp;
  
  ret = rdma_accept(conn_id, &conn_param);
  if (ret) {
    return ret;
  }

  // ToDo: Setup MR - How do we manage memory regions?
  size_t buf_size = 4096;
  char* buf = (char*)malloc(buf_size);
 
  struct ibv_mr * mr = ibv_reg_mr(conn_id->pd, buf, buf_size, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  // konst: for Fabian 
  conn->mr = mr;

  return 0;
}



/*
 *  Constructor 
 */
SendRcvConn sr_conn_init(){
  SendRcvConn conn = calloc(1, sizeof(struct sendRcvConn));
  return conn;
}


int sr_conn_dial(SendRcvConn conn, char *ip, int port) {
  assert(ip != NULL);
  assert(conn != NULL);
  // If we do not specify a port fallback to default port 18515
  if (port == 0) {
    port = 18515;
  }

  int ret;

  struct rdma_addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_port_space = RDMA_PS_TCP;
       
  char strport[80];
  sprintf(strport, "%d", port);

  ret = rdma_getaddrinfo(ip, strport, &hints, &conn->addrinfo);
  if (ret) {
    return ret;
  } 

  struct ibv_qp_init_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.cap.max_send_wr = 1;  // The maximum number of outstanding Work Requests that can be posted to the Send Queue in that Queue Pair
  attr.cap.max_recv_wr = 2;  // The maximum number of outstanding Work Requests that can be posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_send_sge = 1; // The maximum number of scatter/gather elements in any Work Request that can be posted to the Send Queue in that Queue Pair.
  attr.cap.max_recv_sge = 1; // The maximum number of scatter/gather elements in any Work Request that can be posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_inline_data = 0;
  attr.qp_type = IBV_QPT_RC;

  struct rdma_conn_param conn_param;
  memset(&conn_param, 0 , sizeof(conn_param));
  conn_param.responder_resources = 0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will accept from the remote side.
  conn_param.initiator_depth =  0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will have to the remote side.
  conn_param.retry_count = 3;  
  conn_param.rnr_retry_count = 3; 
  
  struct rdma_cm_id *id;

  attr.qp_type = IBV_QPT_RC;

  ret = rdma_create_ep(&id, conn->addrinfo, NULL, NULL); 
  if (ret) {
    return ret;
  }

  ret = rdma_create_qp(id, NULL, &attr);
  if (ret) {
    return ret;
  }
      
  ret = rdma_connect(id, &conn_param);
  if (ret) {
    return ret;
  }

  conn->id = id;
  conn->qp = id->qp;

  // ToDo: Setup MR - How do we manage memory regions?
  size_t buf_size = 4096;
  char* buf = (char*)malloc(buf_size);
 
  struct ibv_mr * mr = ibv_reg_mr(id->pd, buf, buf_size, IBV_ACCESS_REMOTE_WRITE | 
      IBV_ACCESS_LOCAL_WRITE);  // konst: for Fabian 
  conn->mr = mr;
  return 0;
}

int sr_conn_write(SendRcvConn conn, void *buf, size_t len){
  // ToDo: Make send work
  struct ibv_mr *mr = conn->mr;

  memcpy(mr->addr, buf, len);

  struct ibv_sge sge;
  sge.addr = (uintptr_t)mr->addr;
  sge.length = len;
  sge.lkey =  mr->lkey ;
  struct ibv_send_wr wr, *bad;

  wr.wr_id = 0;
  wr.next = NULL;
  wr.sg_list = &sge;
  wr.num_sge = 1;
  wr.opcode = IBV_WR_SEND;

  wr.send_flags = IBV_SEND_SIGNALED;  

  ibv_post_send(conn->qp, &wr, &bad);

  struct ibv_wc wc;
  while(ibv_poll_cq(conn->qp->send_cq, 1, &wc) == 0){
    // ToDo: Only check some
    // nothing
  }
  printf("sent with status %d\n",wc.status);
  return len;
}


int sr_conn_read(SendRcvConn conn, void *buf, size_t len){
  
  struct ibv_sge sge;
  sge.addr = (uintptr_t)conn->mr->addr;
  sge.length = len;
  sge.lkey = conn->mr->lkey;

  struct ibv_recv_wr wr, *bad;
  wr.wr_id = 1;
  wr.next = NULL;
  wr.sg_list = &sge;
  wr.num_sge = 1;

  ibv_post_recv(conn->qp, &wr, &bad);

  struct ibv_wc wc;
  while(ibv_poll_cq(conn->qp->recv_cq, 1, &wc) == 0){}

  if (wc.status) {
    return 0 - wc.status;
  }

  memcpy(buf, conn->mr->addr, wc.byte_len);
  return wc.byte_len;
}


