# Automatic Modulation Recognition

Identify signal modulation scheme from raw RF signal (no demod, synch).

## Core
+ Electronic Support (ES) intercept and build an electronic order of battle
    + Detect radios, radars then demod their waveform and feed into C&C
+ Electronic Attack  (EA) friendly can jam enemy
+ Electronic Protect (EP) friendly can prevent jamming themselves

## Use case

Top level:
+ Pinpoint emitters
+ Classify asset type
+ Recommend action to operators

### Signal Intelligence:
+ Wideband receiver surveys spectrum.
+ Determine targets by identifying modulation, frequency, protocol
+ Flag and forward targets to intel
+ Multiple receivers can direction-find targets

### Attack:
+ Friendly can jam by matching targets parameters
+ Adaptive jamming: coordinating nodes to jam different bands, directions

### Spectrum Awareness:
+ Avoid interference
+ CEPTOR (Rohde and Shwarz) live map
+ Tag friendly and foreign emitters, detect unauthorized use (drones)

## References

+ [TorchSig GNU Radio Block for ML Inference](https://events.gnuradio.org/event/24/contributions/628/attachments/190/473/TorchSig_GRCon2024_paper.pdf)
+ [R&S CEPTER](https://www.rohde-schwarz.com/us/applications/r-s-ceptor-the-military-spectrum-monitoring-solution-application-card_56279-1527495.html)
+ [LATTICE](https://www.anduril.com/command-and-control/)
