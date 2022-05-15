/*******************************************************************************
 * Signal generators for testing.
 * ****************************************************************************/

#pragma once

#include <cmath>

template<typename real>
class Generators {
    private:
        real SR = 48000.0;
        real T = 1.0 / SR;
        real freq = 1000.0;
        real phasor = .0;
        real incr = freq / SR;
        real twoPi = 2.0 * M_PI;
    public:
        void SetSR(real _SR);
        void SetFreq(real _freq);
        void ProcessSine(real* vec, int vecLen);
        void ProcessNoise(real* vec, int vecLen);
};

template<typename real>
void Generators<real>::SetSR(real _SR) {
    SR = _SR;
    T = 1.0 / SR;
    incr = freq / SR;
}

template<typename real>
void Generators<real>::SetFreq(real _freq) {
    freq = _freq;
    incr = freq / SR;
}

template<typename real>
void Generators<real>::ProcessSine(real* vec, int vecLen) {
    for (int i = 0; i < vecLen; i++) {
        phasor += incr;
        phasor = phasor - std::floor(phasor);
        vec[i] = std::sin(twoPi * phasor);
    }
}

template<typename real>
void Generators<real>::ProcessNoise(real* vec, int vecLen) {
    int32_t state = 0;
    int32_t seed = 12345;
    const int32_t MAX = 2147483648 - 1;
    for (int i = 0; i < vecLen; i++) {
        state = state * 1103515245 + seed;
        vec[i] = state / real(MAX);
    }
}
