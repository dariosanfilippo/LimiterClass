#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <chrono>
#include "Generators.hpp"
#include "Limiter.hpp"

int main() {
    typedef double real;
    
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::microseconds;

    std::ofstream csvFile("Limiter.csv", std::ofstream::trunc);
    csvFile << std::fixed << std::setprecision(17);
    std::cout << std::fixed << std::setprecision(17);

    const int vecLen = 4096;
    
    real** inVec = new real*[2];
    real** outVec = new real*[2];
    for (size_t i = 0; i < 2; i++) {
        inVec[i] = new real[vecLen];
        outVec[i] = new real[vecLen];
    }

    real SR = 48000.0;
    real attTime = .01;
    real holdTime = .01;
    real relTime = .1;
    real preGain = 60.0;
    real threshold = -.3;

    Generators<real> generators;
    Limiter<real> limiter(SR, preGain, attTime, holdTime, relTime, threshold);
   
    /* Setup limiter. */
    limiter.SetSR(SR);
    limiter.SetAttTime(attTime);
    limiter.SetHoldTime(attTime);
    limiter.SetRelTime(relTime);
    limiter.SetPreGain(preGain);
    limiter.SetThreshold(threshold);
    limiter.Reset();

    /* Fill input and output vectors to generate a CSV file. */
    generators.ProcessNoise(inVec[0], vecLen);
    generators.ProcessNoise(inVec[1], vecLen);
    limiter.Process(inVec, outVec, vecLen);
    for (size_t i = 0; i < vecLen; i++) {
		csvFile << i << "," << inVec[0][i] << "," << inVec[1][i] << "," << outVec[0][i] << "," << outVec[1][i] << "\n";
	}

    /* Execution time measurement variables. */
    double averageTime = 0;
    double standardDeviation = 0;
    const size_t iterations = 100000;
    double times[iterations];

    for (size_t i = 0; i < iterations; i++) {
        
        /* We run the process function "iterations" times
         * measuring the execution time at each run. We then accumulate
         * the results and store the single times in an array for later 
         * use. */
        auto t0 = high_resolution_clock::now();
        limiter.Process(inVec, outVec, vecLen);
        auto t1 = high_resolution_clock::now();
        duration<double, std::micro> timeDuration = t1 - t0;
        times[i] = timeDuration.count();
        averageTime += timeDuration.count();

        /* Regenerate the input vector at each run. */
        generators.ProcessNoise(inVec[0], vecLen);
        generators.ProcessNoise(inVec[1], vecLen);
    }
    
    /* Compute the execution time average. */
    averageTime /= double(iterations);

    /* Compute the relative standard deviation. Note that for a 
     * measurement to be significant, the standard deviation percentage
     * should be low. */
    for (size_t i = 0; i < iterations; i++) {
        standardDeviation = standardDeviation + 
            std::pow((times[i] - averageTime), 2.0);
    }
    standardDeviation /= double(iterations);
    standardDeviation = std::sqrt(standardDeviation);
    standardDeviation /= averageTime;

    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << "Average execution time (microsecond): " << averageTime << std::endl;
    std::cout << "Relative standard deviation (%): " << (standardDeviation * 100.0) << std::endl;
    std::cout << "The program has generated the file Limiter.csv containing one vector of input and output samples." << std::endl;

    csvFile.close();
    return 0;
}
