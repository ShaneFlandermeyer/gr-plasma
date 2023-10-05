import uhd
import numpy as np
import argparse

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    # parser.add_argument("-o", "--output-file", type=str, required=True)
    parser.add_argument("-f", "--freq", type=float, required=True)
    parser.add_argument("-r", "--rate", default=1e6, type=float)
    parser.add_argument("-d", "--duration", default=5.0, type=float)
    parser.add_argument("-c", "--channels", default=0, nargs="+", type=int)
    parser.add_argument("-g", "--gain", type=int, default=10)
    return parser.parse_args()

def main():
    # args = parse_args()
    args = ""
    duration = 5
    rate = 1e6
    channels = [0]
    freq=2.4e9
    gain = 40
    usrp = uhd.usrp.MultiUSRP(args)
    num_samps = int(np.ceil(duration*rate))
    samps = usrp.recv_num_samps(num_samps, freq, rate, channels, gain)
    print(samps.size)
    # with open(args.output_file, 'wb') as f:
    #     np.save(f, samps, allow_pickle=False, fix_imports=False)

if __name__ == "__main__":
    main()