#include <infiniband/verbs.h>
#include <stdio.h>
#include <cstdlib>

struct ibv_qp_init_attr init_qp_attr(struct ibv_cq* cq){
    struct ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.send_cq = cq;
    qp_init_attr.sq_sig_all = 1; // Alternate with 1 and IBV_SEND_SIGNALED (0 here if flag passed)
    qp_init_attr.recv_cq = cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr  = 1;
    qp_init_attr.cap.max_recv_wr  = 1;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;
    return qp_init_attr;
}

struct ibv_qp_attr init_qp_attr(){
    struct ibv_qp_attr qp_attr;
    memset(&qp_attr, 0, sizeof(qp_attr));
    return qp_attr;
}

ibv_qp_attr DefaultQpAttr() {
    ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                           IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
    attr.max_dest_rd_atomic = 8;
    attr.path_mtu = IBV_MTU_4096;
    attr.min_rnr_timer = 12;
    attr.rq_psn = 0;
    attr.sq_psn = 0;
    attr.timeout = 12;
    attr.retry_cnt = 7;
    attr.rnr_retry = 1;
    attr.max_rd_atomic = 8;
    return attr;
  }


// OLD MODIFY QP CODE 
    /*struct ibv_qp_attr qp_attr = init_qp_attr();
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
    qp_attr.dest_qp_num = ddqp_num;
    qp_attr.rq_psn = 0;
    qp_attr.max_dest_rd_atomic = 1;
    qp_attr.min_rnr_timer = 12;
    qp_attr.ah_attr.is_global = 0;
    qp_attr.ah_attr.sl = 0;
    qp_attr.ah_attr.src_path_bits = 0;
    qp_attr.ah_attr.port_num = 1;
    qp_attr.ah_attr.dlid = dlid;
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
    */
