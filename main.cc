#include "safe_verbs.h"
#include "states.h"
#include "tcp.h"

#include <stdlib.h>
#include <infiniband/verbs.h>
#include <stdio.h>
#include <unistd.h>
#include <thread>

// Alternate building :: g++ -omain main.cc safe_verbs.h states.h tcp.h -libverbs -g
static uint32_t BLOCK_SIZE = 256;

struct { 
  union {
    struct {
      uint64_t a;
      uint64_t b;
      uint64_t c;
    } ints;
    char data[24];
  } content;
  char padding = '\0';
} message;

int main(int argc, char **argv){
    char hostname[100];
    gethostname(hostname, 100);
    bool is_server = hostname[4] == '0';

    // Create a connection between the nodes
    int sockfd;
    if (is_server){
      sockfd = link(is_server, "127.0.0.1");
    } else {
      // Give some time for the server to start
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      sockfd = link(is_server, "10.10.1.1");
    }

    // STEP 1: Create an infiniband context ------------------
    // Apparently I need to call fork before everything, otherwise forking is unsafe
    Ibv_fork_init();
    // Get all the available RDMA devices
    struct ibv_device **device_list = Ibv_get_device_list();
    // Open a single device
    struct ibv_context *ctx = Ibv_open_device(device_list[0]);
    // Free the device list since we are done using it
    ibv_free_device_list(device_list);
    printf("The device '%s' was opened\n", ibv_get_device_name(ctx->device));

    // STEP 2: Create a protection domain ------------------------
    ibv_pd *pd = Ibv_alloc_pd(ctx);
    printf("The protection domain was created for the device\n");

    // STEP 3: Create a completion queue ------------------------
    struct ibv_cq *cq = Ibv_create_cq(ctx, 100, NULL, NULL, 0);
    printf("Opening a completion queue\n");

    // STEP 4: Create a queue pair ----------------
    struct ibv_qp_init_attr qp_init_attr = init_qp_attr(cq);
    struct ibv_qp *qp = Ibv_create_qp(pd, &qp_init_attr);
    printf("Creating a queue pair\n");

    // STEP 5: Exchange identifier information to establish a connection --------
    // If we have >1 node, we exchange this information over a TCP socket (or something else) in order for the neighbors to know enough to connect with w/ me
    struct ibv_port_attr port_attr;
    ibv_query_port(ctx, 1, &port_attr);
    uint16_t lid = port_attr.lid;
    printf("Normal LID: %d\n", lid);
    uint32_t destination_qp_number = qp->qp_num;
    message m;
    message rec_buf;
    m.content.ints.a = lid;
    m.content.ints.b = destination_qp_number; 
    if (is_server){
      Write(sockfd, m.content.data);
      Read(sockfd, rec_buf.content.data);
    } else {
      Read(sockfd, rec_buf.content.data);
      Write(sockfd, m.content.data);
    }
    uint16_t dlid = rec_buf.content.ints.a;
    uint32_t ddqp_num = rec_buf.content.ints.b;

    struct ibv_qp_attr q_attr;
    struct ibv_qp_init_attr q_init_attr;
    ibv_query_qp(qp, &q_attr, IBV_QP_STATE, &q_init_attr);
    printf("1 STATUS:: %d %d\n", q_attr.qp_state, q_attr.cur_qp_state);

    // STEP 6: Change the queue pair state -----------------
    ibv_qp_attr attr;
    int attr_mask;
    attr = DefaultQpAttr();
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = 1;
     attr_mask =
      IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    printf("Loopback: IBV_QPS_INIT\n");
    Ibv_modify_qp(qp, &attr, attr_mask);

    ibv_query_qp(qp, &q_attr, IBV_QP_STATE, &q_init_attr);
    printf("2 STATUS:: %d %d\n", q_attr.qp_state, q_attr.cur_qp_state);

    attr.ah_attr.dlid = dlid;
    attr.qp_state = IBV_QPS_RTR;
    attr.dest_qp_num = ddqp_num;
    attr.ah_attr.port_num = 1;
    attr_mask =
      (IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
     IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);
    printf("Loopback: IBV_QPS_RTR\n");
    Ibv_modify_qp(qp, &attr, attr_mask);

    ibv_query_qp(qp, &q_attr, IBV_QP_STATE, &q_init_attr);
    printf("3 STATUS:: %d %d\n", q_attr.qp_state, q_attr.cur_qp_state);

    attr.qp_state = IBV_QPS_RTS;
    attr_mask = (IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT |
               IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC);
    printf("Loopback: IBV_QPS_RTS\n");
    Ibv_modify_qp(qp, &attr, attr_mask);
    ibv_query_qp(qp, &q_attr, IBV_QP_STATE, &q_init_attr);
    printf("4 STATUS:: %d %d\n", q_attr.qp_state, q_attr.cur_qp_state);
    printf("Modified the queue pair\n");

    // STEP 7: Register a memory region --------------
    void* mr_buffer = malloc(BLOCK_SIZE);
    memset(mr_buffer, 0, BLOCK_SIZE);
    struct ibv_mr *mr = Ibv_reg_mr(pd, mr_buffer, BLOCK_SIZE);

    // STEP 8: Exchange memory region information to handle operations -----------------
    // This step requires nothing when we are a local node
    char* data_buffer = (char*) mr_buffer;
    
    // Set the buffer
    data_buffer[8] = 1;
    data_buffer[9] = 1;
    data_buffer[10] = 1;
    data_buffer[11] = 1;
    
    message mr_send;
    message mr_recv;
    mr_send.content.ints.a = mr->rkey;
    mr_send.content.ints.b = (uint64_t) mr->addr;
    if (is_server){
      Write(sockfd, mr_send.content.data);
      Read(sockfd, mr_recv.content.data);
    } else {
      Read(sockfd, mr_recv.content.data);
      Write(sockfd, mr_send.content.data);
    }
    uint32_t rkey = mr_recv.content.ints.a;
    uint64_t addy = mr_recv.content.ints.b;

    // STEP 9: Test communication ----------------
    printf("Before: ");
    for (int i = 0; i < 20; i++){
        printf("%d", data_buffer[i]);
    }
    printf("\n");

    if (is_server){ // server guard (or client guard)
    struct ibv_send_wr rdma_wr;
    struct ibv_send_wr *bad_wr;
    struct ibv_sge op;
    memset(&op, 0, sizeof(op));

    op.addr = (uint64_t) mr->addr;
    op.length = 4;
    op.lkey = mr->lkey;

    rdma_wr.wr.rdma.remote_addr = addy + 8;
    rdma_wr.wr.rdma.rkey = rkey;
    rdma_wr.opcode = IBV_WR_RDMA_WRITE;
    rdma_wr.send_flags = IBV_SEND_FENCE; // IBV_SEND_SIGNALED;
    rdma_wr.sg_list = &op;
    rdma_wr.num_sge = 1;

    Ibv_post_send(qp, &rdma_wr, &bad_wr);
    // Blocks until write is finished?
    Ibv_poll_cq(cq);
    }

    // Give some time for the work requests to be completed on both sides before dies
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    printf("After: ");
    for (int i = 0; i < 20; i++){
        printf("%d", data_buffer[i]);
    }

    // CLEANUP STEP --------------------------- (Must be done in reverse, order matters!)
    free(mr_buffer);
    // Close communication sockets
    close(sockfd);
    // Deregister a memory region
    Ibv_dereg_mr(mr);
    // Close queue pair
    Ibv_destroy_qp(qp);
    // Close completion queue
    Ibv_destroy_cq(cq);
    // Deallocate protection domain
    Ibv_dealloc_pd(pd);
    // Close device
    Ibv_close_device(ctx);
    return 0;
}
