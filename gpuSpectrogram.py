import cupy as cp
import numpy as np
import pyqtgraph as pg
import time
import uhd

from collections import deque
from pyqtgraph.Qt import QtGui, QtCore
from PyQt5.QtWidgets import QApplication

SAMPLERATE = 3e6
CENTERFREQ = 2.5e9
DURATION   = 10
BUFFERSIZE = 8192
GAIN = 30

serial = ["32C1EC6", "32C1EDB"]

def chunkRecv(ringSize=100):
    """Starts USRP receiver collecting chunks of IQ and calls spectrogramGPU

    Args:
        ringSize (int): Max number of IQ chunks to hold
    Returns:
        ringBuffer [np.ndarray]: Double ended queue holds chunks of IQ
    """
    usrp = uhd.usrp.MultiUSRP(f"serial={serial[0]}")

    # USRP set params
    usrp.set_rx_rate(SAMPLERATE)
    usrp.set_rx_freq(CENTERFREQ)
    usrp.set_rx_gain(GAIN)

    # Floating point & streamer (RX streamer object)
    st_args = uhd.usrp.StreamArgs("fc32", "fc32")
    streamer = usrp.get_rx_stream(st_args)
    metadata = uhd.types.RXMetadata()

    # Issue start streaming command
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    stream_cmd.stream_now = True
    streamer.issue_stream_cmd(stream_cmd)

    # Buffer and ring size
    buffer = np.zeros(BUFFERSIZE, dtype=np.complex64)
    ringBuffer = deque(maxlen=ringSize)

    print(f"D> Start receiver")

    # Init spectrogram window (parameter is shape of spectrogram arrays)
    img, win, app = initSpectroWindow((30, 256))

    try:
        while True:
            QtCore.QCoreApplication.processEvents()
            numRx = streamer.recv(buffer, metadata, timeout=1.0)

            if numRx > 0:
                ringBuffer.append(buffer.copy())
                spectrogram = spectrogramGPU(buffer)
                updateSpectroWindow(img, spectrogram)
                print("D> Spectrogram shape:", spectrogram.shape)
            else:
                print("D> CODE: TIMEOUT")

    except KeyboardInterrupt:
        print("D> Stop receiver")
        stop_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
        streamer.issue_stream_cmd(stop_cmd)

    return ringBuffer

def spectrogramGPU(chunkIQ, nfft=512, overlap=0.75):
    """Generate spectrogram from IQ chunk based on cuPy FFT

    Args:
        chunkIQ [np.ndarray]: 1D array of complex64 IQ samples
        nfft (int): FFT window size
        overlap (float): Overlap ratio of window segments

    Returns:
        cp.ndarray: 2D array representing spectrogram (time x frequency)
     """
    gpuIQ = cp.asarray(chunkIQ)

    step = int(nfft * (1 - overlap))
    numSegments = (len(chunkIQ) - nfft) // step

    spectrogram = []
    for i in range(numSegments):
        start = i * step
        segment = gpuIQ[start:start+nfft]
        windowed = segment * cp.hanning(nfft)
        fftOut = cp.fft.fftshift(cp.fft.fft(windowed))
        power = 10 * cp.log10(cp.abs(fftOut) ** 2 + 1e-12)
        spectrogram.append(power)

    return cp.stack(spectrogram)

def initSpectroWindow(shape=(30, 256)):
    """Init real-time spectrogram window

    Args:
        shape (tuple): Shape of spectrogram image

    Returns:
        tuple: PyQtGraph and Qt window
    """
    app = QApplication([])
    win = pg.GraphicsLayoutWidget(title="Real-Time Spectrogram")
    view = win.addViewBox()
    view.setAspectLocked(False)

    img = pg.ImageItem()
    view.addItem(img)
    img.setImage(np.zeros(shape))

    freq_range = np.linspace(CENTERFREQ - SAMPLERATE / 2, CENTERFREQ +
                             SAMPLERATE / 2, shape[1])
    img.setTransform(pg.QtGui.QTransform().scale((freq_range[-1] -
                                                  freq_range[0]) / shape[1], 1))


    win.resize(800, 400)
    win.show()
    return img, win, app

def updateSpectroWindow(img, specCp):
    """Update spectrogram window in near real-time

    Args:
        img (pyqtgraph.ImageItem): Image handle
        specCp (np.ndarray): cuPy spectrogram
    """
    specNp = cp.asnumpy(specCp)
    img.setImage(specNp.T, levels=(specNp.min(), specNp.max()))


if __name__ == "__main__":
    chunkRecv()
    QtGui.QApplication.instance().exec_()
