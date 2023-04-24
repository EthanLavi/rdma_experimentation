#include "safe_verbs.h"
#include "states.h"

#include <infiniband/verbs.h>
#include <stdio.h>
#include <unistd.h>

// Alternate building :: g++ -omain main.cc safe_verbs.h states.h -libverbs -g
static uint32_t BLOCK_SIZE = 256;

int main(int argc, char **argv){
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
    uint32_t destination_qp_number = qp->qp_num;

    // STEP 6: Change the queue pair state -----------------
    struct ibv_qp_attr qp_attr = init_qp_attr();
    // Setting the fields
    qp_attr.qp_state = ibv_qp_state::IBV_QPS_INIT;
    qp_attr.pkey_index = 0;
    qp_attr.port_num = 1;
    qp_attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
    // Modify into INIT state
    Ibv_modify_qp(qp, &qp_attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

    qp_attr = init_qp_attr();
    // Setting the fields
    qp_attr.qp_state = ibv_qp_state::IBV_QPS_RTR;
    qp_attr.path_mtu = IBV_MTU_4096;
    qp_attr.dest_qp_num = destination_qp_number;
    qp_attr.rq_psn = 0;
    qp_attr.max_dest_rd_atomic = 1;
    qp_attr.min_rnr_timer = 12;
    qp_attr.ah_attr.is_global = 0;
    qp_attr.ah_attr.sl = 0;
    qp_attr.ah_attr.src_path_bits = 0;
    qp_attr.ah_attr.port_num = 1;
    qp_attr.ah_attr.dlid = lid;
    // Modifying into Ready to Receive (RTR) state
    Ibv_modify_qp(qp, &qp_attr, IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);

    qp_attr = init_qp_attr();
    // Setting fields
    qp_attr.qp_state = ibv_qp_state::IBV_QPS_RTS;
    qp_attr.sq_psn = 0;
    qp_attr.timeout = 14;
    qp_attr.retry_cnt = 7;
    qp_attr.rnr_retry = 7;
    qp_attr.max_rd_atomic = 1;
    // Modifying into Ready to Send (RTS) state

    Ibv_modify_qp(qp, &qp_attr, IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC);
    printf("Modified the queue pair\n");

    // STEP 7: Register a memory region --------------
    void* mr_buffer = malloc(BLOCK_SIZE);
    memset(mr_buffer, 0, BLOCK_SIZE);
    struct ibv_mr *mr = Ibv_reg_mr(pd, mr_buffer, BLOCK_SIZE);

    // STEP 8: Exchange memory region information to handle operations -----------------
    // This step requires nothing because we are a local node

    // STEP 9: Test communication ----------------
    struct ibv_send_wr rdma_wr;
    struct ibv_send_wr *bad_wr;
    struct ibv_sge op;
    char* data_buffer = (char*) mr_buffer;
    memset(&op, 0, sizeof(op));
    data_buffer[8] = 1;
    data_buffer[9] = 1;
    data_buffer[10] = 1;
    data_buffer[11] = 1;

    op.addr = (uint64_t) mr->addr;
    op.length = 4;
    op.lkey = mr->lkey;

    rdma_wr.wr.rdma.remote_addr = (uint64_t) mr->addr + 8;
    rdma_wr.wr.rdma.rkey = mr->rkey;
    rdma_wr.opcode = IBV_WR_RDMA_READ;
    rdma_wr.sg_list = &op;
    rdma_wr.num_sge = 1;
    Ibv_post_send(qp, &rdma_wr, &bad_wr);
    // Blocks until write is finished?
    Ibv_poll_cq(cq);

    for (int i = 0; i < 20; i++){
        printf("%d", data_buffer[i]);
    }

    // CLEANUP STEP ---------------------------
    free(mr_buffer);
    // Deregister a memory region
    Ibv_dereg_mr(mr);
    // Close queue pair
    Ibv_destroy_qp(qp);
    // Close completion queue
    Ibv_destroy_cq(cq);
    // Deallocate protection domain
    Ibv_dealloc_pd(pd); // this must be done before close device. Order appears to matter in cleanup!
    // Close device
    Ibv_close_device(ctx);
    return 0;
}
