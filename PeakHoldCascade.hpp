/*******************************************************************************
 *
 * Mono-input, mono-output cascaded peak-holder sections.
 *
 * This function inspects the inputs absolute peaks and it holds them
 * for "holdtime" seconds (approximately) if they are smaller than the 
 * currently held peak, otherwise, the inputs absolute peak is output.
 *
 * This architecture cascades M peak-hold sections, each of them with a hold
 * time that is 1 / M of the full hold period. This allows for secondary peaks 
 * occurring after holdTime / M to also be detected.
 *
 * Copyright (c) 2022 Dario Sanfilippo - sanfilippodario@gmail.com
 *
 * ****************************************************************************/

#pragma once

#include <cmath>

template<size_t stages, typename real>
class PeakHoldCascade {
    private:
        real SR = 48000.0; // Samplerate as a float variable for later calculations.
        real holdTime = .0; // Hold time in seconds.
        const real oneOverStages = 1.0 / real(stages);

        /* We approximate the given hold time in seconds by rounding the samples
         * conversion to the nearest int. Note that the hold time variations are 
         * constrained to steps of "stages" samples, which is the number of cascaded
         * sections. */
        size_t holdTimeSamples = std::rint(holdTime * oneOverStages * SR);
        
        size_t timer[stages] = { 0 };
        real output[stages] = { .0 };

    public:
        void SetSR(real _SR);
        void SetHoldTime(real _holdTime);
        void Reset() { memset(timer, 0, stages); memset(output, 0, stages); };
        void Process(real* xVec, real* yVec, size_t vecLen);
        PeakHoldCascade() { };
        PeakHoldCascade(real _SR, real _holdTime);
};

template<size_t stages, typename real>
void PeakHoldCascade<stages, real>::SetSR(real _SR) {
    SR = _SR;
    holdTimeSamples = std::rint(holdTime * oneOverStages * SR);
}

template<size_t stages, typename real>
void PeakHoldCascade<stages, real>::SetHoldTime(real _holdTime) {
    holdTime = _holdTime;
    holdTimeSamples = std::rint(holdTime * oneOverStages * SR);
}

/* This function computes a peak-holder with a given period P as a combination
 * of "stages" series peak-holder sections with an hold period of P / stages.
 * Given input and output vectors, the function processes a block of vecLen
 * samples of the input signal and stores it in the output vector. */
template<size_t stages, typename real>
void PeakHoldCascade<stages, real>::Process(real* xVec, real* yVec, size_t vecLen) {
    for (size_t n = 0; n < vecLen; n++) { // level-0 for-loop

        /* Outside of th einner for-loop, we assign the absolute value 
         * of the input vector sample to an auxiliary variable, which 
         * will be the input to the first section. */
        real input = std::fabs(xVec[n]);

        /* Compute a series of peak-holders in an inner for-loop. */
        for (size_t stage = 0; stage < stages; stage++) { // level-1 for-loop

            /* Compute the necessary Boolean conditions to determine whether 
             * the system is in a hold or release phase. A new peak is detected
             * when the input absolute value is >= to the output of the 
             * peak-holder. Each time a new peak is detected, a timer is reset. 
             * The peak-holder is in a release phase when a new peak is 
             * detected or the time is out, in which case the input absolute
             * value becomes the input of the system. Otherwise, the system
             * will hold the out value until a release phase occurs. */
            bool isNewPeak = input >= output[stage];
            bool isTimeOut = timer[stage] >= holdTimeSamples;
            bool release = isNewPeak || isTimeOut;

            /* Following a branchless programming paradigm, we compute the paths 
             * for the variables with bifurcation and assign the results using
             * Boolean array-fetching, which is faster than two multiplications 
             * by bools. */
            size_t timerPaths[2] = {
                timer[stage] + 1,
                0
            };
            timer[stage] = timerPaths[release];
            real outPaths[2] = {
                output[stage],
                input
            };
            output[stage] = outPaths[release];

            /* We can now update the auxiliary variable with the output of
             * the current peak-holder section, which will be the input for
             * the next section. */
            input = output[stage];

        } // end of level-1 for-loop

        /* Finally, we can assign the output of the last stage to the output 
         * vector sample. */
        yVec[n] = output[stages - 1];

    } // end of level-0 for-loop
}

template<size_t stages, typename real>
PeakHoldCascade<stages, real>::PeakHoldCascade(real _SR, real _holdTime) {
    SR = _SR;
    holdTime = _holdTime;
    holdTimeSamples = std::rint(holdTime * oneOverStages * SR);
}
