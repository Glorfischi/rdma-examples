#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <infiniband/verbs.h>
#include <string.h>

#include "conn/sendsrcv.h"

struct sharedRcvQueue {
  struct ibv_srq *srq;
  struct ibv_pd *pd;

  struct ibv_srq_init_attr srq_init_attr;

  size_t transfer_size; // The maximum size of a message

  struct ibv_mr **rcv_mrs; // List of memory regions to receive into
  size_t rcv_mr_count; // Number of memory regions to receive into
  size_t rcv_mr_next; // Memory region index we will read from next.
};

struct sendSharedRcvConn{
  struct rdma_addrinfo *addrinfo;

  struct rdma_cm_id* id;
  struct ibv_qp * qp;

  struct ibv_qp_init_attr attr;
  struct rdma_conn_param conn_param;

  size_t transfer_size; // The maximum size of a message

  // ToDo(fabian): To support send bursts we would need to add multiple MRs
  struct ibv_mr *snd_mr; // memory region for sending. 

  struct sharedRcvQueue *srq;
  
};

struct sendSharedRcvListener{
  struct rdma_cm_id *id;
  struct sharedRcvQueue *srq;
};

SendSharedRcvListener ssr_listener_init(){
  SendSharedRcvListener listener = calloc(1, sizeof(struct sendSharedRcvListener));
  return listener;
}

int ssr_listener_listen(SendSharedRcvListener listener,  char *ip, int port) {
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

  struct sharedRcvQueue *srq = calloc(1, sizeof(struct sharedRcvQueue));
  srq->transfer_size = 4086;
  srq->rcv_mr_count = 10;
  srq->rcv_mr_next = 0;
  srq->rcv_mrs = calloc(srq->rcv_mr_count, sizeof(struct ibv_mr*));
  srq->pd = listener->id->pd;

  // ToDo(fabian): Parameterize
  srq->srq_init_attr.attr.max_wr  = srq->rcv_mr_count;
  srq->srq_init_attr.attr.max_sge = 1;

   
  srq->srq = ibv_create_srq(listener->id->pd, &srq->srq_init_attr);
  for (int i = srq->rcv_mr_next; i < srq->rcv_mr_count; i++){
    char* buf = (char*)malloc(srq->transfer_size);
    struct ibv_mr * mr = ibv_reg_mr(srq->pd, buf, srq->transfer_size, 
        IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  

    struct ibv_sge sge;
    sge.addr = (uintptr_t)mr->addr;
    sge.length = srq->transfer_size;
    sge.lkey = mr->lkey;

    struct ibv_recv_wr wr, *bad;
    wr.wr_id = 1;
    wr.next = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    ibv_post_srq_recv(srq->srq, &wr, &bad);

    srq->rcv_mrs[i] = mr;
  }
  listener->srq = srq;
  
  return rdma_listen(listener->id, 2);
}


int ssr_listener_accept(SendSharedRcvListener listener, SendSharedRcvConn conn){
  assert(listener != NULL);
  assert(conn != NULL);

  int ret;
  struct rdma_cm_id *conn_id;

  ret = rdma_get_request(listener->id, &conn_id);
  if(ret){
    return ret;
  }

  conn->srq = listener->srq;
  conn->attr.srq = listener->srq->srq;
  
  ret = rdma_create_qp(conn_id, listener->id->pd, &conn->attr);
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
  return 0;
}




/*
 *  Constructor 
 */
SendSharedRcvConn ssr_conn_init(){
  SendSharedRcvConn conn = calloc(1, sizeof(struct sendSharedRcvConn));

  // ToDo: Parameterize
  conn->transfer_size = 4096;

  memset(&conn->attr, 0, sizeof(conn->attr));
  conn->attr.cap.max_send_wr = 1;  // The maximum number of outstanding Work Requests that can be
  // posted to the Send Queue in that Queue Pair
  conn->attr.cap.max_recv_wr = 2;  // The maximum number of outstanding Work Requests that can be
  // posted to the Receive Queue in that Queue Pair. // ToDo(fischi): Cannot be 0. Why? Applies to srq? Why segfault?
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

int ssr_conn_read(SendSharedRcvConn conn, void *buf, size_t len){
  struct ibv_wc wc;
  while(ibv_poll_cq(conn->qp->recv_cq, 1, &wc) == 0){}

  if (wc.status) {
    return 0 - wc.status;
  }
  // ToDo(fischi): Not thread safe
  struct ibv_mr *mr = conn->srq->rcv_mrs[conn->srq->rcv_mr_next];
  // Update mrindex
  conn->srq->rcv_mr_next = (conn->srq->rcv_mr_next + 1) % conn->srq->rcv_mr_count;
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

  ibv_post_srq_recv(conn->srq->srq, &wr, &bad);

  return wc.byte_len;
}


