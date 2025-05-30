#include <stdio.h>
#include <stdlib.h>

#include <dpu.h>

int main(int argc, char** argv) {
    struct dpu_set_t set, dpu;

    DPU_ASSERT(dpu_alloc_ranks(1, "backend=simulator", &set));
    DPU_ASSERT(dpu_load(set, argv[1], NULL));
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

    DPU_FOREACH(set, dpu) {
        if (dpu_log_read(dpu, stdout) != DPU_OK) {
            printf("No logging symbols found.\n");
        }
    }

    DPU_ASSERT(dpu_free(set));
    return EXIT_SUCCESS;
}
