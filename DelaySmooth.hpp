/*******************************************************************************
 *
 * Mono-input, mono-output delay line with an efficient wrap-around method.
 * The reading and writing heads use fixed-width unsigned int types, 
 * which can overflow safely. The limitation of this design is that buffers 
 * allocation is limited to 2^8, 2^16, 2^32, or 2^64 elements. Of course,
 * only the first two sizes are feasible, hence this technique is convenient
 * for delays that are 256 or 65536 samples maximum.
 *
 * The delay line uses two parallel delay lines among which crossfade takes 
 * place for click-free and Doppler-free delay variations.
 *
 * Copyright (c) 2022 Dario Sanfilippo - sanfilippodario@gmail.com
 *
 * ****************************************************************************/

#pragma once

#include <cstdint>
#include <cmath>
#include <vector>

template<typename head, typename real>
class DelaySmooth {
    private:

        /* Compute the buffer size as the largest representable 
         * int with a head-type variable. */
        const size_t bufferLen = 2l << (8 * sizeof(head) - 1);

        size_t delay = 0; // system output delay in samples
        size_t interpolationTime = 1024; // interpolation time in samples
        size_t lowerDelay = 0; // lower head delay
        size_t upperDelay = 0; // upper head delay
        real interpolation = 0.0; // interpolation coefficient in the range [0; 1]
        real interpolationStep = 1.0 / real(interpolationTime); // interpolation segment slope
        real increment = interpolationStep; // helper var for interpolation coefficient calculation
        
        /* Reading and writing heads. These will cycle continuously from 0
         * to bufferLen - 1. The reading heads will be offset appropriately
         * to set the delay. */
        head lowerReadPtr = 0;
        head upperReadPtr = 0;
        head writePtr = 0;
    
        std::vector<real> buffer;

    public:
        void SetDelay(size_t _delay) { delay = _delay; };
        void SetInterpolationTime(size_t _interpolationTime) {
            interpolationTime = _interpolationTime;
            interpolationStep = 1.0 / real(interpolationTime);
        };
        void Reset() { memset(buffer, 0, bufferLen); };
        void Process(real* xVec, real* yVec, size_t vecLen);
        DelaySmooth() { buffer.resize(bufferLen); };
        DelaySmooth(size_t _delay, size_t _interpolationTime);
};

/* This function computes a click-free and Doppler-free variable delay line
 * by linearly crossfading between two independent integer delay lines.
 * Once a crossfade has been completed, the inactive delay line can be
 * set with a new delay and a new crossafed can start. During the crossfade,
 * neither the delay or interpolation time can be changed. */
template<typename head, typename real>
void DelaySmooth<head, real>::Process(real* xVec, real* yVec, size_t vecLen) {
    for (size_t n = 0; n < vecLen; n++) { // level-0 for-loop

        /* Fill the delay buffer with the input signal;
         * the buffer will be shared by the two delay lines to reduce 
         * memory costs. */
        buffer[writePtr] = xVec[n];

        /* Compute the necessary Boolean conditions to trigger a new
         * interpolation and set a new delay or interpolation time. 
         * The mechanism for smooth delay variations works by linearly
         * interpolating from the currently active delay line to the
         * inactive one, which is set with the new delay. Note that a new delay 
         * or interpolation time can be set only after the transition has been 
         * completed. */
        bool lowerReach = interpolation == 0.0;
        bool upperReach = interpolation == 1.0;
        bool lowerDelayChanged = delay != lowerDelay;
        bool upperDelayChanged = delay != upperDelay;
        bool startDownwardInterp = upperReach && upperDelayChanged;
        bool startUpwardInterp = lowerReach && lowerDelayChanged;

        /* Following a branchless programming paradigm, we compute the paths 
         * for the variables with bifurcation and assign the results using
         * Boolean array-fetching, which is faster than two multiplications 
         * by bools. 
         * 
         * If we have completed an upward interpolation and the delay has 
         * changed, we assign a negative incremental step to trigger a
         * downward interpolation; alternatively, if we are at the bottom 
         * edge and the delay has changed, we assign a positive incremental 
         * step; otherwise, we leave the incremental step unaltered. 
         *
         * The delay is set to whatever delay line becomes inactive after
         * completing the interpolation. Otherwise, it stays unaltered
         * during the transition. */
        real incrementPathsUp[2] = {
            increment, 
            interpolationStep
        };
        real incrementPathsDown[2] = {
            incrementPathsUp[startUpwardInterp], 
            -interpolationStep
        };
        increment = incrementPathsDown[startDownwardInterp];
        size_t lowerDelayPaths[2] = {
            lowerDelay, 
            delay
        };
        size_t upperDelayPaths[2] = {
            upperDelay, 
            delay
        };
        lowerDelay = lowerDelayPaths[upperReach];
        upperDelay = upperDelayPaths[lowerReach];

        /* Compute the delays reading heads and increment the writing head. */
        lowerReadPtr = writePtr - lowerDelay;
        upperReadPtr = writePtr - upperDelay;
        writePtr++;

        /* Compute the interpolation and assign the result to the output. */
        interpolation = 
            std::max<real>(0.0, std::min<real>(1.0, interpolation + increment));
        yVec[n] =
            interpolation * (buffer[upperReadPtr] - buffer[lowerReadPtr]) +
                buffer[lowerReadPtr];

    } // end of level-0 for-loop
}

template<typename head, typename real>
DelaySmooth<head, real>::DelaySmooth(size_t _delay, size_t _interpolationTime) {
    buffer.resize(bufferLen);
    delay = _delay;
    interpolationTime = _interpolationTime;
    interpolationStep = 1.0 / real(interpolationTime);
}
