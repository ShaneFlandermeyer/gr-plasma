
// #include <cuComplex.h>
// #include <cuda_runtime.h>
// #include <cufft.h>
// #include <cufftXt.h>
#include <Eigen/Dense>
// #include <device_launch_parameters.h>
#include <complex>
#include <iostream>
#define NX 256
#define BATCH 10

int main()
{
    std::complex<float> data[9];
    for (int i = 0; i < 9; i++) {
        data[i] = i;
    }

    Eigen::Map<Eigen::Array<std::complex<float>, 3, 3, Eigen::RowMajor>> mat(data, 3, 3);
    Eigen::ArrayXXcf mat2 = Eigen::ArrayXXcf::Zero(5, 3);
    mat2.block(0, 0, 3, 3) =
        Eigen::Map<Eigen::Array<std::complex<float>, 3, 3, Eigen::ColMajor>>(
            mat.data(), 3, 3);
    std::cout << mat2 << std::endl;

    return EXIT_SUCCESS;
}