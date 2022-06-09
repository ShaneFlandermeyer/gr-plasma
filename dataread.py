import numpy as np
import matplotlib.pyplot as plt

x = np.fromfile('/home/shane/pdu_file_sink.dat', dtype=np.complex64)
plt.plot(np.abs(x[:int(1e7)]))
plt.show()