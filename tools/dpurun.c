#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <dpu.h>
#include <dpu_debug.h>
#include <dpu_memory.h>

void* load_file_complete(const char* path, size_t* out_size) {
    FILE* fp = fopen(path, "rb");
    assert(fp != NULL);

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    void* res = malloc(size);
    assert(res != NULL);

    fread(res, size, 1, fp);
    fclose(fp);

    *out_size = size;
    return res;
}

void buf_to_stdout(size_t sz, const uint64_t* buf) {
    FILE* p = popen("xxd -e -g 8", "w");
    assert(p != NULL);

    fwrite(buf, 1, sz * sizeof(buf[0]), p);
    pclose(p);
}

int main(int argc, char** argv) {
    struct dpu_set_t set, dpu;

    if (argc != 3) {
        printf("Usage: dpurun <core loader> <mram image>\n");
        return EXIT_FAILURE;
    }

    size_t buf_sz = 0;
    void* buf = load_file_complete(argv[2], &buf_sz);

    DPU_ASSERT(dpu_alloc_ranks(1, "backend=simulator", &set));
    DPU_ASSERT(dpu_load(set, argv[1], NULL));

    DPU_FOREACH(set, dpu) {
        DPU_ASSERT(dpu_copy_to_mram(dpu.dpu, 63 << 20, buf, buf_sz));
    }

    free(buf);

    dpu_error_t err;

    if ((err = dpu_launch(set, DPU_SYNCHRONOUS)) == DPU_ERR_DPU_FAULT) {
        struct dpu_context_t* ctx = calloc(MAX_NR_DPUS_PER_RANK, sizeof(*ctx));
        size_t i = 0;

        assert(ctx != NULL);

        DPU_FOREACH(set, dpu, i) {
            DPU_ASSERT(dpu_context_fill_from_rank(&ctx[i], set.list.ranks[0]));
            DPU_ASSERT(dpu_initialize_fault_process_for_dpu(dpu.dpu, &ctx[i], 0x1000));
            DPU_ASSERT(dpu_extract_pcs_for_dpu(dpu.dpu, &ctx[i]));
            DPU_ASSERT(dpu_extract_context_for_dpu(dpu.dpu, &ctx[i]));

            dpu_thread_t tid = ctx[i].bkp_fault_thread_index;
            printf("Thread [%d] faulted [code = %06x] at pc = 0x8%07lx (caller = 0x8%07lx)\n", tid, ctx[i].bkp_fault_id, ctx[i].pcs[tid] * 8, (ctx[i].registers[23 + 24 * tid] - 1) * 8);
        }

        free(ctx);
    }

    DPU_FOREACH(set, dpu) {
        uint64_t output[8];

        if (dpu_log_read(dpu, stdout) != DPU_OK) {
            printf("No logging symbols found.\n");
        }

        DPU_ASSERT(dpu_copy_from_mram(dpu.dpu, (const uint8_t*) &output[0], (64 << 20) - sizeof(output), sizeof(output)));
        buf_to_stdout(8, output);
    }

    DPU_ASSERT(dpu_free(set));
    return EXIT_SUCCESS;
}
