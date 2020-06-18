#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <infiniband/verbs.h>

 
class VerbsEP{

public:
  struct rdma_cm_id* const id;
  struct ibv_qp * const qp;
  struct ibv_pd * const pd;
  const uint32_t max_inline_data;
 
  const uint32_t max_send_size;
  const uint32_t max_recv_size;

  VerbsEP(struct rdma_cm_id* id, struct ibv_qp *qp, uint32_t max_inline_data,uint32_t max_send_size,uint32_t max_recv_size): 
          id(id),qp(qp), pd(qp->pd), max_inline_data(0),max_send_size(max_send_size),max_recv_size(max_recv_size)
  {
      // empty
  }

  ~VerbsEP(){
    // TODO
  }

  uint32_t get_qp_num() const{
    return qp->qp_num;
  }
};
