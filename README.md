# LimiterClass
C++ classes for an IIR look-ahead limiter implementing cascaded exponential smoothers for low-distortion peak-limiting and maximisation.

Reference:

 Sanfilippo, D. (2022). Envelope following via cascaded exponential smoothers 
 for low-distortion peak limiting and maximisation. In Proceedings of the
 International Faust Conference, Saint-Étienne, France. (https://www.dropbox.com/s/pwujls02aqo4uo0/Sanfilippo-IFC22-EnvelopeFollowing.pdf?dl=0)

The repository contains the header-only classes DelaySmooth.hpp, PeakHoldCascade.hpp, ExpSmootherCascade.hpp, and Limiter.hpp. The first class implements a click-free and Doppler-free delay line, which we use to set the limiter's lookahead delay. The second class implements a peak-holder system as a set of cascaded peak-holders, which allow for secondary peaks to be detected. The third class implements an exponential smoother based on cascaded one-pole filters and the cut-off correction formula proposed by (Zavalishin 2012). The last class is the actual limiter implementation where these classes are combined.

The limiter parameters are: Pre Gain, Attack Time, Hold Time, Release Time, and Threshold. The pre gain is an amplification factor in dB applied to the input signal before processing. The attack time, in seconds, sets the limiter's attack rate and lookahead delay. The hold time, in seconds, allows to hold peaks for an extra period and it can be particularly useful to improve THD at low frequencies without affecting the release time. The release time, in seconds, sets the release rate of the limiter. Finally, the threshold parameter, in dB, sets the limiter's ceiling.

The repository also includes test programs to test the correctness of the processing of the individial classes, as well as performance measurements providing average execution times and relative standard deviation to determine the significance of each test run.
