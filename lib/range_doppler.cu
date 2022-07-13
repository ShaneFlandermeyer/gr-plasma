// #include <gnuradio/plasma/ra
// #include <gnuradio/plasma/range_doppler.h>
#include <complex>
#include <iostream>

#include <cufft.h>


__global__ void
rdComplexMultiply(cuFloatComplex* s, cuFloatComplex* w, long int M, long int N)
{
    long int i = threadIdx.x + blockDim.x * blockIdx.x;

    if (i < N * M) {
        long int n = i % N;

        s[i] = cuCmulf(s[i], w[n]);
        // s[i] = make_cuFloatComplex(10.0f,0.0f);
    }
}

__global__ void
rdComplexDivide(cuFloatComplex* s, long int M, long int N)
{
    long int i = threadIdx.x + blockDim.x * blockIdx.x;

    if (i < N * M) {
        long int n = i % N;
        s[i] = cuCdivf(s[i],make_cuFloatComplex((float)N,0.0f);
    }
}


void cudaMatchedFilter(std::complex<float>* H,
                       std::complex<float>* x,
                       std::complex<float>* out,
                       long int M,
                       long int N)
{
    long int num_thread_block = 256;
    long int num_block_grid;

    cuFloatComplex *d_rx, *d_mf_freq;
    cudaMalloc((void**)&d_rx, sizeof(cuFloatComplex) * M * N);
    cudaMalloc((void**)&d_mf_freq, sizeof(cuFloatComplex) * N);
    cudaMemcpy(d_rx, x, sizeof(cuFloatComplex) * M * N, cudaMemcpyHostToDevice);
    cudaMemcpy(d_mf_freq, H, sizeof(cuFloatComplex) * N, cudaMemcpyHostToDevice);

    cufftHandle plan;
    cufftPlan1d(&plan, N, CUFFT_C2C, M);

    cufftExecC2C(plan, (cufftComplex*)d_rx, (cufftComplex*)d_rx, CUFFT_FORWARD);

    // Multiplying signal with waveform in fourier domain
    num_block_grid = (N * M + num_thread_block - 1) / num_thread_block;
    rdComplexMultiply<<<num_block_grid, num_thread_block>>>(d_rx, d_mf_freq, M, N);
    cudaDeviceSynchronize();
    cufftExecC2C(plan, (cufftComplex*)d_rx, (cufftComplex*)d_rx, CUFFT_INVERSE);
    rdComplexDivide<<<num_block_grid, num_thread_block>>>(d_rx, M, N);
    cudaMemcpy(out, d_rx, sizeof(cuFloatComplex) * M * N, cudaMemcpyDeviceToHost);

    cufftDestroy(plan);
    cudaFree(d_rx);
    cudaFree(d_mf_freq);
}

// void matchedFilterProcessingCUDA_gpu(std::complex<float>* signal,
//                                      std::complex<float>* waveform,
//                                      long int M,
//                                      long int N)
// {
// size_t mem_size = sizeof(cuFloatComplex) * M * N;
// long int threadsPerBlock = 256;
// long int blocksPerGrid;

// cuFloatComplex *d_signal, *d_waveform;
// cudaMalloc((void**)&d_signal, mem_size);
// cudaMalloc((void**)&d_waveform, (mem_size / M));
// cudaMemcpy(d_signal, signal, mem_size, cudaMemcpyHostToDevice);
// cudaMemcpy(d_waveform, waveform, (mem_size / M), cudaMemcpyHostToDevice);

// cufftHandle plan;
// int nCol[1] = { N };
// if (cufftPlan1d(&plan, N, CUFFT_C2C, M) !=
//     CUFFT_SUCCESS) {
//     std::cout << "Problem creating plan" << std::endl;
// }
// // if (cufftPlan1d(&plan, N, CUFFT_C2C, M) != CUFFT_SUCCESS) {
// //     std::cout << "Problem creating plan" << std::endl;
// // }

// cufftExecC2C(plan, (cufftComplex*)d_signal, (cufftComplex*)d_signal,
// CUFFT_FORWARD);

// // Multiplying signal with waveform in fourier domain
// blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
// rdComplexMultiply<<<blocksPerGrid, threadsPerBlock>>>(d_signal, d_waveform, M, N);

// cufftExecC2C(plan, (cufftComplex*)d_signal, (cufftComplex*)d_signal,
// CUFFT_INVERSE);

// cudaMemcpy(signal, d_signal, mem_size, cudaMemcpyDeviceToHost);

// cufftDestroy(plan);
// cudaFree(d_signal);
// cudaFree(d_waveform);
// }