#include <infiniband/verbs.h>
#include <stdio.h>
#include <cstdlib>

void init(){
    int status;
    // Apparently I need to call fork before everything, otherwise forking is unsafe
    status = ibv_fork_init();
    if (status != 0){
        fprintf(stderr, "Error in initializing the RDMA environment. Updated kernel required.\n");
        exit(1);
    }

    // Get all the available RDMA devices
    struct ibv_device **device_list = ibv_get_device_list(NULL);
    if (device_list == NULL || device_list != 0){
        fprintf(stderr, "Error in finding RDMA devices. Are there any available?\n");
        exit(1);
    }
    struct ibv_context *ctx;

    // Open the device
    ctx = ibv_open_device(device_list[0]);
    if (!ctx){
        fprintf(stderr, "Error, failed to open the device '%s'\n",
                ibv_get_device_name(device_list[0]));
        exit(1);
    }
    // Free the device list since we are done using it
    ibv_free_device_list(device_list);

    printf("The device '%s' was opened\n", ibv_get_device_name(ctx->device));
}