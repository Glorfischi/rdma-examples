#include "verbsEP.hpp"
#include "connectRDMA.hpp"
#include <bits/stdint-uintn.h>
#include <chrono>
#include "cxxopts.hpp"
#include <cstddef>
#include <iostream>
#include <thread> 
 
cxxopts::ParseResult
parse(int argc, char* argv[])
{
    cxxopts::Options options(argv[0], "Server for the microbenchmark");
    options
      .positional_help("[optional args]")
      .show_positional_help();
 
  try
  {
 
    options.add_options()
      ("address", "IP address of the victim", cxxopts::value<std::string>(), "IP")
      ("print", "print qpnums")
      ("counter", "Number to count up to", cxxopts::value<uint64_t>())
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
 


std::vector<VerbsEP *> connections;

int main(int argc, char* argv[]){
 
  auto allparams = parse(argc,argv);

  std::string ip = allparams["address"].as<std::string>();  
  auto n = allparams["counter"].as<uint64_t>();  
 
  int port = 9999;
  ClientRDMA * client = new ClientRDMA(const_cast<char*>(ip.c_str()),port);
  struct ibv_qp_init_attr attr;
  struct rdma_conn_param conn_param;
 
  memset(&attr, 0, sizeof(attr));
  attr.cap.max_send_wr = 1;  // The maximum number of outstanding Work Requests that can be posted to the Send Queue in that Queue Pair
  attr.cap.max_recv_wr = 2;  // The maximum number of outstanding Work Requests that can be posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_send_sge = 1; // The maximum number of scatter/gather elements in any Work Request that can be posted to the Send Queue in that Queue Pair.
  attr.cap.max_recv_sge = 1; // The maximum number of scatter/gather elements in any Work Request that can be posted to the Receive Queue in that Queue Pair. 
  attr.cap.max_inline_data = 0;
  attr.qp_type = IBV_QPT_RC;

  memset(&conn_param, 0 , sizeof(conn_param));
  conn_param.responder_resources = 0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will accept from the remote side.
  conn_param.initiator_depth =  1;  // The maximum number of outstanding RDMA read and atomic operations that the local side will have to the remote side.
  conn_param.retry_count = 3;  
  conn_param.rnr_retry_count = 3; 
  
 
  VerbsEP *ep = client->connectEP(&attr,&conn_param,NULL);
  struct ibv_pd* pd = ep->pd;
  struct ibv_qp* qp = ep->qp;

  size_t buf_size = 4096;
  char* buf = (char*)malloc(buf_size);
 
  struct ibv_mr * mr = ibv_reg_mr(pd, buf, buf_size, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  // konst: for Fabian 
 
  {
    struct ibv_sge sge;
    sge.addr = (uintptr_t)buf;
    sge.length = 4096;
    sge.lkey = mr->lkey;

    struct ibv_recv_wr wr, *bad;
    wr.wr_id = 1;
    wr.next = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    ibv_post_recv(qp, &wr, &bad);
  }

  struct ibv_wc wc;
  while(ibv_poll_cq(qp->recv_cq, 1, &wc) == 0){}
  printf("Received with status %d\n",wc.status);

  struct ibv_sge* recv = (struct ibv_sge*)buf;
  printf("Received addr: %p len: %u key: %u\n", (void *)recv->addr,recv->length,recv->lkey);
  
  auto remote_address = recv->addr; // (Fischi) We received the location of the mem region
  auto remote_key = recv->lkey; // (Fischi) What exactly is that?


  for(uint64_t i=0; i<n; i++){
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // some delay
    memset(buf, 0, buf_size);
    {
      struct ibv_sge sg;
      struct ibv_send_wr wr;
      struct ibv_send_wr *bad_wr;

      memset(&sg, 0, sizeof(sg));
      sg.addr	  = (uintptr_t)buf;
      sg.length = buf_size;
      sg.lkey	  = mr->lkey;
      
      memset(&wr, 0, sizeof(wr));
      wr.wr_id      = 0;
      wr.sg_list    = &sg;
      wr.num_sge    = 1;
      wr.opcode     = IBV_WR_ATOMIC_FETCH_AND_ADD;
      wr.send_flags = IBV_SEND_SIGNALED;
      wr.wr.atomic.remote_addr = remote_address;
      wr.wr.atomic.rkey        = remote_key;
      wr.wr.atomic.compare_add = 1ULL;

      if (ibv_post_send(qp, &wr, &bad_wr)) {
        fprintf(stderr, "Error, ibv_post_send() failed\n");
        return -1;
      }

      struct ibv_wc wc;
      while(ibv_poll_cq(qp->send_cq, 1, &wc) == 0){
          // nothing
      }
      printf("sent with status %d and read %d\n",wc.status, *(uint64_t *)buf);
    }
  }
  return 0;
}

 
 
 
