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

  struct ibv_qp_init_attr attr;
  struct rdma_conn_param conn_param;



  size_t transfer_size; // The maximum size of a message

  // ToDo(fabian): To support send bursts we would need to add multiple MRs
  struct ibv_mr *snd_mr; // memory region for sending. 

  struct ibv_mr **rcv_mrs; // List of memory regions to receive into
  size_t rcv_mr_count; // Number of memory regions to receive into
  size_t rcv_mr_next; // Memory region index we will read from next.
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
  
  ret = rdma_create_qp(conn_id, NULL, &conn->attr);
  if (ret) {
    return ret;
  }
  conn->id = conn_id;
  conn->qp = conn_id->qp;
  
  ret = rdma_accept(conn->id, &conn->conn_param);
  if (ret) {
    return ret;
  }

  char* buf = (char*)malloc(conn->transfer_size);
  conn->snd_mr = ibv_reg_mr(conn_id->pd, buf, conn->transfer_size, 
      IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  

  for (int i = conn->rcv_mr_next; i < conn->rcv_mr_count; i++){
    char* buf = (char*)malloc(conn->transfer_size);
    struct ibv_mr * mr = ibv_reg_mr(conn_id->pd, buf, conn->transfer_size, 
        IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  

    struct ibv_sge sge;
    sge.addr = (uintptr_t)mr->addr;
    sge.length = conn->transfer_size;
    sge.lkey = mr->lkey;

    struct ibv_recv_wr wr, *bad;
    wr.wr_id = 1;
    wr.next = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    ibv_post_recv(conn->qp, &wr, &bad);

    conn->rcv_mrs[i] = mr;
  }
  
  return 0;
}



/*
 *  Constructor 
 */
SendRcvConn sr_conn_init(){
  SendRcvConn conn = calloc(1, sizeof(struct sendRcvConn));

  // ToDo: Parameterize
  conn->transfer_size = 4096;

  conn->rcv_mr_count = 5;
  conn->rcv_mr_next = 0;
  conn->rcv_mrs = calloc(conn->rcv_mr_count, sizeof(struct ibv_mr*));

  memset(&conn->attr, 0, sizeof(conn->attr));
  conn->attr.cap.max_send_wr = 1;  // The maximum number of outstanding Work Requests that can be
  // posted to the Send Queue in that Queue Pair
  conn->attr.cap.max_recv_wr = conn->rcv_mr_count;  // The maximum number of outstanding Work Requests that can be
  // posted to the Receive Queue in that Queue Pair. 
  conn->attr.cap.max_send_sge = 1; // The maximum number of scatter/gather elements in any Work 
  // Request that can be posted to the Send Queue in that Queue Pair.
  conn->attr.cap.max_recv_sge = 1; // The maximum number of scatter/gather elements in any 
  // Work Request that can be posted to the Receive Queue in that Queue Pair. 
  conn->attr.cap.max_inline_data = 0;
  conn->attr.qp_type = IBV_QPT_RC;

  memset(&conn->conn_param, 0 , sizeof(conn->conn_param));
  conn->conn_param.responder_resources = 0;  // The maximum number of outstanding RDMA read and atomic operations that 
  // the local side will accept from the remote side.
  conn->conn_param.initiator_depth =  0;  // The maximum number of outstanding RDMA read and atomic operations that the 
  // local side will have to the remote side.
  conn->conn_param.retry_count = 3;  
  conn->conn_param.rnr_retry_count = 3; 

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

  struct rdma_cm_id *id;

  conn->attr.qp_type = IBV_QPT_RC;

  ret = rdma_create_ep(&id, conn->addrinfo, NULL, NULL); 
  if (ret) {
    return ret;
  }

  ret = rdma_create_qp(id, NULL, &conn->attr);
  if (ret) {
    return ret;
  }
      
  ret = rdma_connect(id, &conn->conn_param);
  if (ret) {
    return ret;
  }

  conn->id = id;
  conn->qp = id->qp;

  char* buf = (char*)malloc(conn->transfer_size);
  conn->snd_mr = ibv_reg_mr(conn->id->pd, buf, conn->transfer_size, 
      IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  

  for (int i = conn->rcv_mr_next; i < conn->rcv_mr_count; i++){
    char* buf = (char*)malloc(conn->transfer_size);
    struct ibv_mr * mr = ibv_reg_mr(conn->id->pd, buf, conn->transfer_size, 
        IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  

    struct ibv_sge sge;
    sge.addr = (uintptr_t)mr->addr;
    sge.length = conn->transfer_size;
    sge.lkey = mr->lkey;

    struct ibv_recv_wr wr, *bad;
    wr.wr_id = 1;
    wr.next = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    ibv_post_recv(conn->qp, &wr, &bad);

    conn->rcv_mrs[i] = mr;
  }
  return 0;
}

int sr_conn_write(SendRcvConn conn, void *buf, size_t len){
  struct ibv_mr *mr = conn->snd_mr;

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
    // ToDo(fischi): Add bursting
  }
  return len;
}


int sr_conn_read(SendRcvConn conn, void *buf, size_t len){
  struct ibv_wc wc;
  while(ibv_poll_cq(conn->qp->recv_cq, 1, &wc) == 0){}

  if (wc.status) {
    return 0 - wc.status;
  }
  struct ibv_mr *mr = conn->rcv_mrs[conn->rcv_mr_next];
  memcpy(buf, mr->addr, wc.byte_len);

  // Repost MR
  struct ibv_sge sge;
  sge.addr = (uintptr_t)mr->addr;
  sge.length = len;
  sge.lkey = mr->lkey;

  struct ibv_recv_wr wr, *bad;
  wr.wr_id = 1;
  wr.next = NULL;
  wr.sg_list = &sge;
  wr.num_sge = 1;

  ibv_post_recv(conn->qp, &wr, &bad);

  // Update mr index
  conn->rcv_mr_next = (conn->rcv_mr_next + 1) % conn->rcv_mr_count;

  return wc.byte_len;
}


