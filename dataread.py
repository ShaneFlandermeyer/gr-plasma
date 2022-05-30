import numpy as np
import matplotlib.pyplot as plt

x = np.fromfile('/home/shane/data.dat', dtype=np.complex64)
plt.plot(np.real(x))
plt.show()