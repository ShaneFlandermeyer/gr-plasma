#include <gnuradio/plasma/range_doppler.h>
#include <complex>
#include <iostream>

/**
 * @brief Multiply each column of matrix x1 (stored as a contiguous data vector)
   by x2, then scale the result by
 *
 * @param x1 Input matrix data
 * @param x2 Input coefficient data
 * @param ncol Number of columns in x1
 * @param nrow Number of rows in x1
 */
__global__ void multiplyAndScale(cuFloatComplex* x1,
                                      cuFloatComplex* x2,
                                      long int nrow,
                                      long int ncol,
                                      cuFloatComplex divScale)
{
    long int i = threadIdx.x + blockDim.x * blockIdx.x;
    if (i < nrow * ncol) {
        long int n = i % nrow;
        x1[i] = cuCmulf(x1[i], x2[n]);
        x1[i] = cuCdivf(x1[i], divScale);
    }
}


void cudaMatchedFilter(std::complex<float>* H,
                       std::complex<float>* x,
                       std::complex<float>* out,
                       long int num_pulse,
                       long int nfft)
{
    long int num_thread_block = 256;
    long int num_block_grid;

    // Copy input arrays to the device
    cuFloatComplex *d_x, *d_H;
    cudaMalloc((void**)&d_x, sizeof(cuFloatComplex) * num_pulse * nfft);
    cudaMalloc((void**)&d_H, sizeof(cuFloatComplex) * nfft);
    cudaMemcpy(d_x, x, sizeof(cuFloatComplex) * num_pulse * nfft, cudaMemcpyHostToDevice);
    cudaMemcpy(d_H, H, sizeof(cuFloatComplex) * nfft, cudaMemcpyHostToDevice);

    // FFT input array
    cufftHandle plan;
    cufftPlan1d(&plan, nfft, CUFFT_C2C, num_pulse);
    cufftExecC2C(plan, (cufftComplex*)d_x, (cufftComplex*)d_x, CUFFT_FORWARD);

    // Multiplying signal with waveform in fourier domain and 
    num_block_grid = (nfft * num_pulse + num_thread_block - 1) / num_thread_block;
    multiplyAndScale<<<num_block_grid, num_thread_block>>>(
        d_x, d_H, nfft, num_pulse, nfft);

    // IFFT input array
    cufftExecC2C(plan, (cufftComplex*)d_x, (cufftComplex*)d_x, CUFFT_INVERSE);
    // Copy data back from the device and clean up
    cudaMemcpy(
        out, d_x, sizeof(cuFloatComplex) * num_pulse * nfft, cudaMemcpyDeviceToHost);
    cufftDestroy(plan);
    cudaFree(d_x);
    cudaFree(d_H);
}
