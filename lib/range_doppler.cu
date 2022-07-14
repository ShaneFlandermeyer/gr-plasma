#include <gnuradio/plasma/range_doppler.h>
#include <complex>
#include <iostream>

/**
 * @brief Element-wise multiply of each column of matrix x1 (stored as a contiguous data
 vector) by x2
 *
 * @param x1 Input matrix data
 * @param x2 Input coefficient data
 * @param ncol Number of columns in x1
 * @param nrow Number of rows in x1
 */
__global__ void
colwiseHadamard(cuFloatComplex* x1, cuFloatComplex* x2, long int nrow, long int ncol)
{
    long int i = threadIdx.x + blockDim.x * blockIdx.x;
    if (i < nrow * ncol) {
        long int n = i % nrow;
        x1[i] = cuCmulf(x1[i], x2[n]);
    }
}

__global__ void divide(cuFloatComplex* x, long int N, cuFloatComplex c)
{
    long int i = threadIdx.x + blockDim.x * blockIdx.x;
    if (i < N) {
        x[i] = cuCdivf(x[i], c);
    }
}

template <typename T>
__global__ void cufftShift_1D_kernel(T* data, int N)
{
    int index = ((blockIdx.x * blockDim.x) + threadIdx.x);
    if (index < N / 2) {
        // Save the first value
        T regTemp = data[index];
        // Swap the first element
        data[index] = (T)data[index + (N / 2)];
        // Swap the second one
        data[index + (N / 2)] = (T)regTemp;
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
    int rank = 1;
    int n[1] = { (int)nfft };
    int* inembed = NULL;
    int istride = num_pulse;
    int idist = 1;
    int* onembed = NULL;
    int ostride = num_pulse;
    int odist = 1;
    int batch = num_pulse;
    cufftHandle plan;
    cufftPlanMany(&plan,
                  rank,
                  n,
                  inembed,
                  istride,
                  idist,
                  onembed,
                  ostride,
                  odist,
                  CUFFT_C2C,
                  batch);
    // cufftPlanMany(
    //     &plan, 1, ncol, NULL, num_pulse, 1, NULL, num_pulse, 1, CUFFT_C2C, num_pulse);
    cufftExecC2C(plan, (cufftComplex*)d_x, (cufftComplex*)d_x, CUFFT_FORWARD);

    // Multiplying signal with waveform in fourier domain and scale by the fft
    // size to account for the 1/n factor in the IFFT
    num_block_grid = (nfft * num_pulse + num_thread_block - 1) / num_thread_block;
    colwiseHadamard<<<num_block_grid, num_thread_block>>>(d_x, d_H, nfft, num_pulse);

    // IFFT input array
    cufftExecC2C(plan, (cufftComplex*)d_x, (cufftComplex*)d_x, CUFFT_INVERSE);
    divide<<<num_block_grid, num_thread_block>>>(
        d_x, nfft * num_pulse, make_cuFloatComplex(nfft, 0));
    // FIXME: Segfaulting here
    // Copy data back from the device and clean up
    cudaMemcpy(
        out, d_x, sizeof(cuFloatComplex) * num_pulse * nfft, cudaMemcpyDeviceToHost);


    cufftDestroy(plan);
    cudaFree(d_x);
    cudaFree(d_H);
}

void cudaDopplerProcessing(std::complex<float>* out,
                           std::complex<float>* x,
                           long int nrow,
                           long int ncol)
{
    long int num_thread_block = 256;
    long int num_block_grid = (nrow * nrow + num_thread_block - 1) / num_thread_block;
    ;
    // Move the input data to the device
    cuFloatComplex* d_x;
    cudaMalloc((void**)&d_x, nrow * ncol * sizeof(cuFloatComplex));
    cudaMemcpy(d_x, x, nrow * ncol * sizeof(cuFloatComplex), cudaMemcpyHostToDevice);

    // FFT input array
    cufftHandle plan;
    int rank = 1;
    int n[1] = { (int)ncol };
    int idist = 1;
    int odist = 1;
    int inembed[1] = { (int)ncol };
    int onembed[1] = { (int)ncol };
    int istride = nrow;
    int ostride = nrow;
    int batch = nrow;
    cufftPlanMany(&plan,
                  rank,
                  n,
                  inembed,
                  istride,
                  idist,
                  onembed,
                  ostride,
                  odist,
                  CUFFT_C2C,
                  batch);


    cufftExecC2C(plan, (cufftComplex*)d_x, (cufftComplex*)d_x, CUFFT_FORWARD);
    cufftShift_1D_kernel<<<num_block_grid, num_thread_block>>>(d_x, nrow * ncol);
    cudaMemcpy(out, d_x, sizeof(cuFloatComplex) * nrow * ncol, cudaMemcpyDeviceToHost);

    cufftDestroy(plan);
    cudaFree(d_x);
}
