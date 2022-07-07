# gr-plasma: A GNU Radio module for radar signal processing

The gr-plasma module implements a number of radar signal processing functions in GNU Radio, including waveform generation, transmission and reception in UHD-compatible software-defined radios, and pulse-doppler processing. The module pulls much of its signal processing from [plasma-dsp](https://github.com/ShaneFlandermeyer/plasma-dsp), which is a more general-purpose radar signal processing library intended for use outside GNU Radio.

## Installation
To install gr-plasma system-wide, plasma-dsp and its dependencies must be installed through conan, as described in its readme. Once plasma-dsp is installed, the following commands must be executed from the top-level folder in this repository

```[bash]
mkdir build
cd build
conan install ..
cmake ..
make
sudo make install
sudo ldconfig
```

The module can similarly be uninstalled from the top-level directory as shown below

```[bash]
cd build
sudo make uninstall
sudo ldconfig
```
