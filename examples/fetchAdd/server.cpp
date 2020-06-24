#include "verbsEP.hpp"
#include "connectRDMA.hpp"
#include "cxxopts.hpp"
#include <bits/stdint-uintn.h>
#include <cstdio>
#include <infiniband/verbs.h>
#include <iostream>
#include <ostream>
#include <vector>
#include <thread>
#include <arpa/inet.h>
 
cxxopts::ParseResult
parse(int argc, char* argv[])
{
    cxxopts::Options options(argv[0], "Server for the QP test. It accepts connections");
    options
      .positional_help("[optional args]")
      .show_positional_help();
 
  try
  {
 
    options.add_options()
      ("address", "IP address", cxxopts::value<std::string>(), "IP")
      ("help", "Print help")
     ;
 
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    if (result.count("address") == 0)
    {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    return result;

  } catch (const cxxopts::OptionException& e)
  {
    std::cout << "error parsing options: " << e.what() << std::endl;
    std::cout << options.help({""}) << std::endl;
    exit(1);
  }
}

void dump(char* p, size_t len){
  for (int i = 0; i  < len; i++){
    printf("%02x ", p[i]);
  }
}

int main(int argc, char* argv[]){
  auto allparams = parse(argc,argv);

  std::string ip = allparams["address"].as<std::string>();  
 
  int port = 9999;

  ServerRDMA * server = new ServerRDMA(const_cast<char*>(ip.c_str()),port);
  struct ibv_qp_init_attr attr;
  struct rdma_conn_param conn_param;
 
 
  memset(&attr, 0, sizeof(attr));
  attr.cap.max_send_wr = 1;  // The maximum number of outstanding Work Requests that can be posted to the Send Queue in that Queue Pair
  attr.cap.max_recv_wr = 5;  // The maximum number of outstanding Work Requests that can be posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_send_sge = 1; // The maximum number of scatter/gather elements in any Work Request that can be posted to the Send Queue in that Queue Pair.
  attr.cap.max_recv_sge = 1; // The maximum number of scatter/gather elements in any Work Request that can be posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_inline_data = 0;
  attr.qp_type = IBV_QPT_RC; // Connection Type IBV_QPT_RC	Reliable Connection IBV_QPT_UC	Unreliable Connection IBV_QPT_UD	Unreliable Datagram

  memset(&conn_param, 0 , sizeof(conn_param));
  conn_param.responder_resources = 5;  // The maximum number of outstanding RDMA read and atomic operations that the local side will accept from the remote side.
  conn_param.initiator_depth =  0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will have to the remote side.
  conn_param.retry_count = 3; 
  conn_param.rnr_retry_count = 3;  
 
  struct ibv_pd *pd = server->create_pd();
  VerbsEP* ep = server->acceptEP(&attr,&conn_param,pd);
  struct ibv_qp* qp = ep->qp;

  size_t buf_size = 4096;
  char* buf = (char*)calloc(buf_size, sizeof(char));

  struct ibv_mr * mr = ibv_reg_mr(pd, buf, buf_size, IBV_ACCESS_REMOTE_ATOMIC | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  
  
  printf("Local Mem: %p lkey %u\n",(void *)(buf),mr->lkey);
  
  {
    struct ibv_sge * send = (struct ibv_sge *)buf;
    send->addr = (uintptr_t)buf;
    send->length = buf_size;
    send->lkey = mr->rkey;
  }

  {
    struct ibv_sge sge;
    sge.addr = (uintptr_t)buf;
    sge.length = sizeof(struct ibv_sge);
    sge.lkey =  mr->lkey ;
    struct ibv_send_wr wr, *bad;

    wr.wr_id = 0;
    wr.next = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_SEND;

    wr.send_flags = IBV_SEND_SIGNALED;  

    ibv_post_send(qp, &wr, &bad);
  }


  struct ibv_wc wc;
  while(ibv_poll_cq(qp->send_cq, 1, &wc) == 0){}
  printf("sent with status %d\n",wc.status);

  memset(buf, 0 , buf_size);
  uint64_t *counter = (uint64_t *)buf;

  while(true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // some delay
    std::cout << "Counter: " << *counter << std::endl;
  }
  return 0;
}
