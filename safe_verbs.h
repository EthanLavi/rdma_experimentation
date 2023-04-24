#include <infiniband/verbs.h>
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <unistd.h>

//  To make code cleaner with no error handling required.

const char* get_error_message(int err_val){
    switch(err_val){
        case EINVAL:
            return "Error: Invalid value";
        case ENOMEM:
            return "Error: Not enough memory";
        case ENOSYS:
            return "Error: Not supported by RDMA device";
        case EPERM:
            return "Error: Invalid permissions";
        case EBUSY:
            return "Error: Busy because in the middle of processing";
        default:
            return "Error: Unknown";
    }
}

/**
 * ibv_fork_init - Prepare data structures so that fork() may be used
 * safely.  If this function is not called or returns a non-zero
 * status, then libibverbs data structures are not fork()-safe and the
 * effect of an application calling fork() is undefined.
 */
inline void Ibv_fork_init(){
    int err = ibv_fork_init();
    if (err != 0){
        fprintf(stderr, "Error in initializing the RDMA environment. Updated kernel required. (%s)\n", get_error_message(err));
        exit(1);
    }
}

/**
 * ibv_get_device_list - Get list of IB devices currently available
 * @num_devices: optional.  if non-NULL, set to the number of devices
 * returned in the array.
 *
 * Return a NULL-terminated array of IB devices.  The array can be
 * released with ibv_free_device_list().
 */
inline struct ibv_device** Ibv_get_device_list(){
    int length = 0;
    struct ibv_device **device_list = ibv_get_device_list(&length);
    if (device_list == nullptr || length == 0){
        fprintf(stderr, "Error in finding RDMA devices. Are there any available?\n");
        exit(1);
    }
    return device_list;
}

/**
 * ibv_open_device - Initialize device for use
 */
inline struct ibv_context* Ibv_open_device(ibv_device* device){
    struct ibv_context* ctx = ibv_open_device(device);
    if (ctx == nullptr){
        fprintf(stderr, "Error, failed to open the device '%s'\n", ibv_get_device_name(device));
        exit(1);
    }
    return ctx;
}  

/**
 * ibv_close_device - Release device
 */
inline void Ibv_close_device(struct ibv_context *context){
    int err = ibv_close_device(context);
    if (err != 0) {
        fprintf(stderr, "Error, failed to close the device '%s' with %s\n", ibv_get_device_name(context->device), get_error_message(err));
        exit(1);
    }
}

/**
 * ibv_alloc_pd - Allocate a protection domain
 */
inline struct ibv_pd* Ibv_alloc_pd(struct ibv_context* context){
    struct ibv_pd* pd = ibv_alloc_pd(context);
    if (pd == nullptr){
        fprintf(stderr, "Error, failed to allocate protection domain\n");
        exit(1);
    }
    return pd;
}

/**
 * ibv_dealloc_pd - Free a protection domain
 */
inline void Ibv_dealloc_pd(struct ibv_pd* pd){
    int err = ibv_dealloc_pd(pd);
    if (err != 0){
        fprintf(stderr, "Error, failed to de-allocate protection domain with %s\n", get_error_message(err));
        exit(1);
    }
}

/**
 * ibv_create_cq - Create a completion queue
 * @context - Context CQ will be attached to
 * @cqe - Minimum number of entries required for CQ
 * @cq_context - Consumer-supplied context returned for completion events
 * @channel - Completion channel where completion events will be queued.
 *     May be NULL if completion events will not be used.
 * @comp_vector - Completion vector used to signal completion events.
 *     Must be >= 0 and < context->num_comp_vectors.
 */
inline struct ibv_cq* Ibv_create_cq(struct ibv_context *context, int cqe, void *cq_context, struct ibv_comp_channel* channel, int comp_vector){
    struct ibv_cq *cq = ibv_create_cq(context, cqe, cq_context, channel, comp_vector);
    if (cq == nullptr){
        fprintf(stderr, "Error, failed to create a completion queue with %s\n", get_error_message(errno));
        exit(1);
    }
    return cq;
}

/**
 * ibv_destroy_cq - Destroy a completion queue
 */
inline void Ibv_destroy_cq(struct ibv_cq* cq){
    int err = ibv_destroy_cq(cq);
    if (err != 0){
        fprintf(stderr, "Error, cannot destroy completion queue with %s\n", get_error_message(err));
        exit(1);
    }
}

/**
 * ibv_create_qp - Create a queue pair.
 */
struct ibv_qp* Ibv_create_qp(struct ibv_pd* pd, struct ibv_qp_init_attr* qp_init_attr){
    ibv_qp* qp = ibv_create_qp(pd, qp_init_attr);
    if (qp == nullptr){
        fprintf(stderr, "Error, cannot create queue pair with %s\n", get_error_message(errno));
        exit(1);
    }
    return qp;
}

/**
 * ibv_destroy_qp - Destroy a queue pair.
 */
inline void Ibv_destroy_qp(struct ibv_qp* qp){
    int err = ibv_destroy_qp(qp);
    if (err != 0){
        fprintf(stderr, "Error, cannot destroy queue pair with %s\n", get_error_message(err));
        exit(1);
    }
}

/**
 * ibv_modify_qp - Modify a queue pair.
 */
inline void Ibv_modify_qp(struct ibv_qp* qp, struct ibv_qp_attr* qp_attr, int attr_mask){
    int status = ibv_modify_qp(qp, qp_attr, attr_mask);
    if (status != 0){
        fprintf(stderr, "Cannot modify the queue pair with %d:%s\n", errno, get_error_message(errno));
        exit(1);
    }
}  

/**
 * Register a memory region
*/
inline struct ibv_mr* Ibv_reg_mr(struct ibv_pd *pd, void *addr, size_t length){
    struct ibv_mr* mr = ibv_reg_mr(pd, addr, length, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC);
    if (mr == nullptr){
        fprintf(stderr, "Cannot register a memory region %d:%s\n", errno, get_error_message(errno));
        exit(1);
    }
    return mr;
}

/**
 * Unregister a memory region
*/
inline void Ibv_dereg_mr(struct ibv_mr* mr){
    int err = ibv_dereg_mr(mr);
    if (err != 0){
        fprintf(stderr, "Cannot de-register a memory region %d:%s\n", err, get_error_message(err));
        exit(1);
    }
}

/**
 * Send a work request
*/
inline void Ibv_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr){
    int err = ibv_post_send(qp, wr, bad_wr);
    if (err != 0){
        fprintf(stderr, "Cannot send a work request %d:%s\n", err, get_error_message(err));
        exit(1);
    }
}

/**
 * Poll a completion queue
*/
void Ibv_poll_cq(struct ibv_cq* cq) {
  struct ibv_wc wc;
  int result;
  int count = 0;
  do {
    result = ibv_poll_cq(cq, 1, &wc);
    if (count == 1000){
        result = 1;
    } else {
        count++;
    }
  } while (result == 0);

  if (result > 0 && wc.status == ibv_wc_status::IBV_WC_SUCCESS) {
    return;
  }

  // You can identify which WR failed with wc.wr_id.
  fprintf(stderr, "Poll failed with status %s (work request ID: %lu)\n", ibv_wc_status_str(wc.status), wc.wr_id);
  exit(1);
}


