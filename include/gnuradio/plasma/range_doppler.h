#ifndef A12C37D2_39C4_4E19_8B04_F963CB4B7A0D
#define A12C37D2_39C4_4E19_8B04_F963CB4B7A0D

#include <cuComplex.h>
#include <cuda_runtime.h>
#include <cufft.h>
#include <cufftXt.h>
#include <device_launch_parameters.h>
#include <complex>

void cudaMatchedFilter(std::complex<float>* mf_freq,
                       std::complex<float>* rx,
                       std::complex<float>* out,
                       long int M,
                       long int N);
void matchedFilterProcessingCUDA_gpu(std::complex<float>* signal,
                                     std::complex<float>* waveform,
                                     long int M,
                                     long int N);

#endif /* A12C37D2_39C4_4E19_8B04_F963CB4B7A0D */
