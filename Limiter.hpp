/*******************************************************************************
 *
 * This code implements the limiter design in the following paper:
 *
 * Sanfilippo, D. (2022). Envelope following via cascaded exponential smoothers 
 * for low-distortion peak-limiting and maximisation. In Proceedings of the
 * International Faust Conference, Saint-Étienne, France.
 *
 * Specifically, the class implements a look-ahead peak-limiter based on an 
 * IIR design. The envelope profiling combines cascaded peak-holders and 
 * cascaded one-pole smoothers, which allow for smooth amplitude following
 * and very low total harmonic distortion.
 *
 * Copyright (c) 2022 Dario Sanfilippo - sanfilippodario@gmail.com
 *
 * ****************************************************************************/

#pragma once

#include <cmath>
#include <cstdint>
#include "DelaySmooth.hpp"
#include "PeakHoldCascade.hpp"
#include "ExpSmootherCascade.hpp"

template<typename real>
class Limiter {
    private:
        real SR = 48000.0; // Samplerate as a float variable for later calculations.
        real T = 1.0 / SR; // Sampling period.
        const real twoPi = 2.0 * M_PI;
        real attack = .01; // Attack time in seconds.
        real hold = .0; // Hold time in seconds, useful to improve THD at lower frequencies.
        real release = .05; // Release time in seconds.
        real dBThreshold = -.3; // Threshold in dB.
        real linThreshold = std::pow(10.0, dBThreshold * .05); // Linear threshold value.

        real dBPreGain = .0; // Input gain before processing in dB.
        real linPreGain = 1.0; // Linear gain.
        real smoothPreGain = .0; // Smoothed out linear gain for click-free variations.
        real smoothThreshold = .0; // Smoothed out limiting threshold for click-free variations.
    
        /* Coefficient for a 20 Hz one-pole low-pass filter. */
        real smoothParamCoeff = std::exp(-twoPi * 20.0 * T);
    
        size_t lookaheadDelay = 0;
        DelaySmooth<uint16_t, real> delayLeft;
        DelaySmooth<uint16_t, real> delayRight;
        const static size_t numberOfPeakHoldSections = 8;
        const static size_t numberOfSmoothSections = 4;
        const real oneOverPeakSections = 1.0 / real(numberOfPeakHoldSections);
        PeakHoldCascade<numberOfPeakHoldSections, real> peakHolder;
        ExpSmootherCascade<numberOfSmoothSections, real> expSmoother;
    
    public:
        void SetSR(real _SR);
        void SetAttTime(real _attack);
        void SetHoldTime(real _hold);
        void SetRelTime(real _release);
        void SetThreshold(real _threshold);
        void SetPreGain(real _preGain);
        void Reset();
        void Process(real** xVec, real** yVec, size_t vecLen);
        Limiter() { };
        Limiter(real _SR, real _dBPreGain, real _attack, real _hold, real _release, real _dBThreshold);
};

template<typename real>
void Limiter<real>::SetSR(real _SR) {
    SR = _SR;
    T = 1.0 / SR;
    smoothParamCoeff = std::exp(-twoPi * 20.0 * T);
    peakHolder.SetSR(SR);
    expSmoother.SetSR(SR);
}

template<typename real>
void Limiter<real>::SetAttTime(real _attack) {
    attack = _attack;
    
    /* We compute the delay so that it matches the hold time of the
     * peak-holder section for correct input-attenuation synchronisation.
     * Both hold and delay times are dependent on the attack time. */
    lookaheadDelay =
        rint(attack * oneOverPeakSections * SR) * numberOfPeakHoldSections;
    
    /* We set the interpolation time equal to the delay for minimum
     * overshooting during attack variations. */
    delayLeft.SetDelay(lookaheadDelay);
    delayLeft.SetInterpolationTime(lookaheadDelay);
    delayRight.SetDelay(lookaheadDelay);
    delayRight.SetInterpolationTime(lookaheadDelay);
    
    expSmoother.SetAttTime(attack);
    peakHolder.SetHoldTime(attack + hold);
}

template<typename real>
void Limiter<real>::SetHoldTime(real _hold) {
    hold = _hold;
    
    /* The hold time is simply an extension of the peak-holder period
     * that allows for better convergence to the target amplitude. The
     * parameter is particularly useful to reduce THD at low frequencies. */
    peakHolder.SetHoldTime(attack + hold);
}

template<typename real>
void Limiter<real>::SetRelTime(real _release) {
    release = _release;
    expSmoother.SetRelTime(release);
}

template<typename real>
void Limiter<real>::SetThreshold(real _threshold) {
    dBThreshold = _threshold;
    linThreshold = std::pow(10.0, dBThreshold * .05);
}

template<typename real>
void Limiter<real>::SetPreGain(real _preGain) {
    dBPreGain = _preGain;
    linPreGain = std::pow(10.0, dBPreGain * .05);
}

template<typename real>
void Limiter<real>::Reset() {
    delayLeft.Reset();
    delayRight.Reset();
    peakHolder.Reset();
    expSmoother.Reset();
}

/* This function computes a lookahead limiting process deploying eight cascaded
 * peak-holder sections as approximation of a max filter, and four cascaded
 * one-pole smoothers for envelope following. Note that the process introduces
 * a delay in the input signal equal to the attack time. Given input and output
 * vectors, the function processes a block of vecLen samples of the input signal
 * and stores it in the output vector. */
template<typename real>
void Limiter<real>::Process(real** xVec, real** yVec, size_t vecLen) {
    real* xLeft = xVec[0];
    real* xRight = xVec[1];
    real* yLeft = yVec[0];
    real* yRight = yVec[1];
    
    /* Apply the pre gain to the input samples. */
    for (size_t n = 0; n < vecLen; n++) {
        smoothPreGain =
            linPreGain + smoothParamCoeff * (smoothPreGain - linPreGain);
        xLeft[n] *= smoothPreGain;
        xRight[n] *= smoothPreGain;
    }

    /* Compute the max between inputs absolute values for stereo
     * processing and store it in the left output vector. */
    for (size_t n = 0; n < vecLen; n++) {
        yLeft[n] = std::max<real>(std::fabs(xLeft[n]), std::fabs(xRight[n]));
    }
    
    /* Compute the peak-hold envelope of the left and right input vectors and
     * store it in the corresponding output vector. */
    peakHolder.Process(yLeft, yLeft, vecLen);

    /* We clip the resulting vector to the threshold value so that input
     * signals below this value are unaltered. We store the resulting
     * values in the left output vector. Similarly, we store the
     * smoothed out threshold parameter in the right output vector for
     * later use. yLeft now contains the clipped peak-hold envelope,
     * while yRight contains the smoothed out threshold parameter. */
    for (size_t n = 0; n < vecLen; n++) {
        smoothThreshold =
            linThreshold + smoothParamCoeff * (smoothThreshold - linThreshold);
        yLeft[n] = std::max<real>(yLeft[n], smoothThreshold);
        yRight[n] = smoothThreshold;
    }

    /* We smooth out the clipped peak envelope using four cascaded one-pole
     * branching sections with independent attack and release times.
     * yLeft now contains a smooth envelope profile of the input signal. */
    expSmoother.Process(yLeft, yLeft, vecLen);

    /* We compute the attenuation gain as the ratio between the limiting
     * threshold and the envelope profile. Finally, we copy the resulting
     * vector to both output vectors as the attenuation gain will be the
     * same for both inputs. */
    for (size_t n = 0; n < vecLen; n++) {
        yLeft[n] = yRight[n] / yLeft[n];
        yRight[n] = yLeft[n];
    }

    /* We apply the look-ahead delay to synchronise the input signals and the
     * attenuation gain. */
    delayLeft.Process(xLeft, xLeft, vecLen);
    delayRight.Process(xRight, xRight, vecLen);

    /* Lastly, we apply the attenuation gain to the delayed inputs and store
     * the result in the output vectors. */
    for (size_t n = 0; n < vecLen; n++) {
        yLeft[n] *= xLeft[n];
        yRight[n] *= xRight[n];
    }
}

template<typename real>
Limiter<real>::Limiter(real _SR, real _dBPreGain, real _attack, real _hold, real _release, real _dBThreshold) {
    SR = _SR;
    dBPreGain = _dBPreGain;
    attack = _attack;
    hold = _hold;
    release = _release;
    dBThreshold = _dBThreshold;
}
