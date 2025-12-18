#define VEC_SIZE 64

#include <mram.h>
#include "mutex.h"

#define UNDEFINED_VAL (-1)
volatile int shared_variable = UNDEFINED_VAL;
MUTEX_INIT(my_mutex);

__mram uint64_t a[VEC_SIZE];
__mram uint64_t b[VEC_SIZE];
__mram uint64_t c[VEC_SIZE];

int main() {
  mutex_lock(my_mutex);
    for (int i = 0; i < VEC_SIZE; i++) {
        c[i] = a[i] + b[i];
    }
  mutex_unlock(my_mutex);
}
