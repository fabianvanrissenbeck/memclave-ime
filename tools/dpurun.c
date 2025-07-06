#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <dpu.h>
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
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

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
