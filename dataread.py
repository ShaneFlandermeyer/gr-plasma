from matplotlib.colors import Colormap
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import animation
import scipy.signal
import scipy.constants as sc

fc = 5e9
lam = sc.c/fc
samp_rate = 100e6
prf = 5e3
pri = 1 / prf
npulse_cpi = 128
nsamp_cpi = int(npulse_cpi*pri*samp_rate)

# Format the rx data as pulses
# x = np.fromfile('/media/shane/Tony/long_collect.dat',
#                 dtype=np.complex64, offset=8*(82+100*int(nsamp_cpi)), count=int(nsamp_cpi))
x = np.fromfile('/home/shane/pdu_file_sink.dat',
                dtype=np.complex64, offset=8*(82+0*int(nsamp_cpi)), count=int(nsamp_cpi))
x = np.reshape(x, (-1, npulse_cpi), order='F')
mf = np.fromfile('/home/shane/mf.dat', dtype=np.complex64)

nconv = x.shape[0] + mf.shape[0] - 1
range_pulse_map = np.zeros((nconv, npulse_cpi), dtype=np.complex64)
for i in range(x.shape[1]):
    range_pulse_map[:, i] = scipy.signal.convolve(x[:, i], mf)


range_dopp_map = np.fft.fftshift(np.fft.fft(range_pulse_map, axis=1), axes=1)
maxval = np.max(np.abs(range_dopp_map.flatten()))
dyrange = -80
range_dopp_map_db = 20*np.log10(np.abs(range_dopp_map)/maxval)
range_dopp_map_db = np.where(
    range_dopp_map_db < dyrange, dyrange, range_dopp_map_db)

range_axis = (sc.c/2) * (np.arange(nconv) - mf.shape[0]) / samp_rate
dopp_axis = np.arange(-prf/2, prf/2, prf/npulse_cpi)
vel_axis = (lam/2)*dopp_axis

# plt.figure()
fig = plt.figure()
im = plt.imshow(range_dopp_map_db, extent=(np.min(vel_axis), np.max(vel_axis),
                                           np.min(range_axis), np.max(range_axis)), aspect='auto', origin='lower', cmap='plasma')
plt.colorbar()
plt.show()


def init():
    im.set_data(range_dopp_map_db)
    return [im]


def animate(iframe):

    # a = im.get_array()
    # a = a*np.exp(-0.001*i)    # exponential decay of the values
    # im.set_array(a)
    # return [im]

    # Format the rx data as pulses
    x = np.fromfile('/media/shane/Tony/long_collect.dat',
                    dtype=np.complex64, offset=8*(82+(2*iframe+50)*nsamp_cpi), count=nsamp_cpi)
    x = np.reshape(x, (-1, npulse_cpi), order='F')
    # nconv = x.shape[0] + mf.shape[0] - 1
    # range_pulse_map = np.zeros((nconv, npulse_cpi), dtype=np.complex64)
    for ipulse in range(x.shape[1]):
        range_pulse_map[:, ipulse] = scipy.signal.convolve(x[:, ipulse], mf)

    range_dopp_map = np.fft.fftshift(
        np.fft.fft(range_pulse_map, axis=1), axes=1)
    maxval = np.max(np.abs(range_dopp_map.flatten()))
    # dyrange = -80
    range_dopp_map_db = 20*np.log10(np.abs(range_dopp_map)/maxval)
    range_dopp_map_db = np.where(
        range_dopp_map_db < dyrange, dyrange, range_dopp_map_db)

    # plt.figure()
    # range_axis = (sc.c/2) * (np.arange(nconv) - mf.shape[0]) / samp_rate
    # dopp_axis = np.arange(-prf/2, prf/2, prf/npulse_cpi)
    # vel_axis = (lam/2)*dopp_axis
    im.set_array(range_dopp_map_db)
    plt.xlim([-20,20])
    plt.ylim([-200,200])
    plt.title('Frame: ' + str(iframe))
    print("Frame: ", iframe+1)
    return [im]


# anim = animation.FuncAnimation(fig, animate, init_func=init,
#                                frames=200, interval=20)
# anim.save('/home/shane/basic_animation.gif', fps=30)
