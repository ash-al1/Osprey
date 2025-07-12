Meta reasoning:
- Academic research that does not turn into proper products, or real life production is
somewhat (if always) a waste of time. This is called research circus.
- I need experience with systems engineering & manufacturing

Whats the problem?
- No open source ready signal classification systems (plug & play) exist
    - Only research projects, python programming code, simulated datasets, etc
    - Spectrogram wideband automatic modulation recognition is not a solved
      problem. Lots of research in narrowband (with simulated/custom data) but
      not with standard dataset (torchsig), 50+ mods, multiple signal per
      spectrogram
- No libraries for working with USRP at a high level easily, exists [WRONG]
    - Some exist but are not standards yet or have been removed
    - Besides GNU Radio, which is a GUI

What are the unknowns?
- UHD & CuPY, but lack of knowledge
- Not sure how to create a DL model prior to testing
    - Should there be a feedback loop to retrain models?
    - What modulation schemes should we focus on?
    - What frequency range is a good to choose?
- Do we need to program cuda kernels or just use CuPY?
- Whats the I/O for receiving and plotting?

Whats my plan?
0. Build GUI for plotting
    + ImGui, ImPlot
1. Build a header library to access USRP easy
    + C/C++ header library for setting up the USRP
    + Include functions to modify & get general parameters of the USRP
2. Build compute library
    + IQ -> AP (amplitude, phase) + FFT + PSD
3. Real-time data architecture
    + Decimation for visualization, not for compute?
    + Threading
    + Circular buffer system for 50 MSPS
