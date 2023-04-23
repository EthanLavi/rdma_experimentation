#include <infiniband/verbs.h>
#include <stdio.h>

int main(int argc, char** argv){
    int rc = ibv_close_device(ctx);
    if (rc) {
        fprintf(stderr, "Error, failed to close the device '%s'\n",
                ibv_get_device_name(ctx->device));
        return 1;
    }

    return 0;
}
