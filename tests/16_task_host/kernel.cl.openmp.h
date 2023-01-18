#include <iris/iris_openmp.h>

static void process(int* A, int* factor, IRIS_OPENMP_KERNEL_ARGS) {
  size_t _id;
#pragma omp parallel for shared(A, factor) private(_id)
  IRIS_OPENMP_KERNEL_BEGIN
  A[_id] = _id * factor[0];
  IRIS_OPENMP_KERNEL_END
}

