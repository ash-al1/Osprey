Goal:
    + GPU accelerated signal classification in real-time
    + I don't care about how good a trained model is, just that we perform
      signal classification in real time.
    + cuPy, cuFFT
    + Spectrogram + raw IQ, maybe FFT & power spectral density too? Not sure

Tasks:
    + Create dataset of simulated signals in spectrogram form
    + Simple train a CNN/LSTM model
    + Save trained model for inference
    + Develop how we're going to send the signals for inference in real-time
    + Develop smoe GUI objects to be displayed on current QtGui spectrogram
    + Fix the timeout problem where spectrogram is buffering

Questions:
    + Do we care about performance of model?
    + Besides spectrogram and IQ should we do other forms?
    + How the fuck is signal classification done in real time ... we don't know
      any prior parameters like SNR how are we supposed to validate in real time
      too? Think about this, or ask someone smarter than me

Work done:
    + Real time collect signals with UHD and USRP board
    + Init QtGui spectrogram window
    + Update spectrogram window in near real-time
