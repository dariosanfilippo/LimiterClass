/*******************************************************************************
 *
 * Mono-input, mono-output exponential smoother via cascaded one-pole filters
 * with 2π*tau time constant.
 *
 * Copyright (c) 2022 Dario Sanfilippo - sanfilippodario@gmail.com
 *
 * ****************************************************************************/

#pragma once

#include <cmath>

template<size_t stages, typename real>
class ExpSmootherCascade {
    private:
        real SR = 48000.0; // Samplerate as a float variable for later calculations.
        real T = 1.0 / SR; // Sampling period.
        const real twoPi = 2.0 * M_PI;
        real twoPiT = twoPi * T;
        real attTime = .001; // Attack time in seconds.
        real relTime = .01; // Release time in seconds.
    
        /* Coefficient correction factor to maintain consistent attack and
         * decay rates when cascading multiple one-pole sections. */
        const real coeffCorrection =
            1.0 / std::sqrt(std::pow(2.0, 1.0 / real(stages)) - 1.0);
    
        real attCoeff = std::exp((-twoPiT * coeffCorrection) / attTime);
        real relCoeff = std::exp((-twoPiT * coeffCorrection) / relTime);
    
        /* We store the coefficients in an array for efficient Boolean
         * fetching without branching. */
        real coeff[2] = {
            relCoeff,
            attCoeff
        };
        real output[stages] = { .0 };

    public:
        void SetSR(real _SR);
        void SetAttTime(real _attTime);
        void SetRelTime(real _relTime);
        void Reset() { memset(output, 0, stages); };
        void Process(real* xVec, real* yVec, size_t vecLen);
        ExpSmootherCascade() { };
        ExpSmootherCascade(real _SR, real _attTime, real _relTime);
};

template<size_t stages, typename real>
void ExpSmootherCascade<stages, real>::SetSR(real _SR) {
    SR = _SR;
    T = 1.0 / SR;
    twoPiT = twoPi * T;
}

/* We store attack and relelease phases coefficients in an array for look-up 
 * table selection using a Boolean index, which is faster than two 
 * multiplications by bools. */
template<size_t stages, typename real>
void ExpSmootherCascade<stages, real>::SetAttTime(real _attTime) {
    attTime = _attTime;
    attCoeff = std::exp((-twoPiT * coeffCorrection) / attTime);
    coeff[1] = attCoeff;
}

template<size_t stages, typename real>
void ExpSmootherCascade<stages, real>::SetRelTime(real _relTime) {
    relTime = _relTime;
    relCoeff = std::exp((-twoPiT * coeffCorrection) / relTime);
    coeff[0] = relCoeff;
}

/* Given input and output vectors, the function processes a block of vecLen
 * samples of the input signal and stores it in the output vector. */
template<size_t stages, typename real>
void ExpSmootherCascade<stages, real>::Process(real* xVec, real* yVec, size_t vecLen) {
    for (size_t n = 0; n < vecLen; n++) { // level-0 for-loop
        
        /* Outside of the inner for-loop, we assign the input vector sample 
         * to an auxiliary variable, which will be the input to the first 
         * section. */
        real input = xVec[n];

        /* Compute M series exponential smoothers in an inner for-loop. */
        for (size_t stage = 0; stage < stages; stage++) { // level-1 for-loop

            /* Determine whether the system is in the attack or release 
             * phase, namely checking if the input is greater than the 
             * output. */
            bool isAttackPhase = input > output[stage];

            /* Compute the output of the one-pole section "stage" using the
             * corresponding attack or release coefficient. */
            output[stage] =
                input + coeff[isAttackPhase] * (output[stage] - input);

            /* We can now update the input to the next section with the 
             * output of the current one. */
            input = output[stage];

        } // end of level-1 for-loop

        /* Finally, we assign the output of the last section to the output
         * vector sample. */
        yVec[n] = output[stages - 1];

    } // end of level-0 for-loop
}

/* We store attack and relelease phases coefficients in an array for look-up
 * table selection using a Boolean index, which is faster than two
 * multiplications by bools. */
template<size_t stages, typename real>
ExpSmootherCascade<stages, real>::ExpSmootherCascade(real _SR, real _attTime, real _relTime) {
    SR = _SR;
    T = 1.0 / SR;
    twoPiT = twoPi * T;
    attTime = _attTime;
    attCoeff = std::exp((-twoPiT * coeffCorrection) / attTime);
    relTime = _relTime;
    relCoeff = std::exp((-twoPiT * coeffCorrection) / relTime);
    coeff[0] = relCoeff;
    coeff[1] = attCoeff;
}
