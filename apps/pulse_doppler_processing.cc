#include <plasma_dsp/linear_fm_waveform.h>
#include <iostream>
#include <fstream>

std::vector<std::complex<float>> read(const std::string& filename, size_t offset = 0) {
  std::ifstream infile(filename, std::ios::in | std::ios::binary);
  infile.seekg(0, std::ios::end);
  std::size_t size = infile.tellg();
  infile.seekg(offset, std::ios::beg);
  
  std::vector<std::complex<float>> data(size / sizeof(std::complex<float>));
  infile.read(reinterpret_cast<char*>(data.data()), size * sizeof(float));
  infile.close();
  return data;
}

int main(int argc, char *argv[]) {
  // Generate the reference waveform and matched filter
  double bandwidth = 75e6;
  double pulse_width = 1e-3;
  double samp_rate = 100e6;
  plasma::LinearFMWaveform waveform(bandwidth, pulse_width, 0, samp_rate);
  Eigen::ArrayXcf mf = waveform.MatchedFilter().cast<std::complex<float>>();
  std::vector<std::complex<float>> data = read("/home/shane/pdu_file_sink.dat");
  std::cout << data[0] << std::endl;
  std::cout << data.size() << std::endl;
  // for (auto e : data) {
  //   std::cout << e << std::endl;
  // }

  return EXIT_SUCCESS;

}