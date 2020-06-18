#include "verbsEP.hpp"
#include "connectRDMA.hpp"
#include "cxxopts.hpp"
#include <bits/stdint-uintn.h>
#include <cstdio>
#include <iostream>
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
  attr.qp_type = IBV_QPT_RC;

  memset(&conn_param, 0 , sizeof(conn_param));
  conn_param.responder_resources = 0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will accept from the remote side.
  conn_param.initiator_depth =  0;  // The maximum number of outstanding RDMA read and atomic operations that the local side will have to the remote side.
  conn_param.retry_count = 3; 
  conn_param.rnr_retry_count = 3;  
 
  struct ibv_pd *pd = server->create_pd();
  VerbsEP* ep = server->acceptEP(&attr,&conn_param,pd);
  struct ibv_qp* qp = ep->qp;

  size_t buf_size = 4096;
  char* buf = (char*)calloc(buf_size, sizeof(char));

  struct ibv_mr * mr = ibv_reg_mr(pd, buf, buf_size, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);  // konst: for Fabian 
  while(true) {
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

    std::cout << "Buf: ";
    dump(buf, 16);
    std::cout << "\n";
  }
  return 0;
}
