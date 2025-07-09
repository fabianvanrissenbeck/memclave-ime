#define VEC_SIZE 64

#include <mram.h>

__mram uint64_t a[VEC_SIZE];
__mram uint64_t b[VEC_SIZE];
__mram uint64_t c[VEC_SIZE];

int main(void) {
    for (int i = 0; i < VEC_SIZE; i++) {
        c[i] = a[i] + b[i];
    }
}
