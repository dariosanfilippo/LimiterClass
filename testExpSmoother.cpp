#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <chrono>
#include "Generators.hpp"
#include "ExpSmootherCascade.hpp"

int main() {
    typedef double real;
    
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::microseconds;

    std::ofstream csvFile("ExpSmootherCascade.csv", std::ofstream::trunc);
    csvFile << std::fixed << std::setprecision(17);
    std::cout << std::fixed << std::setprecision(17);

    const int vecLen = 4096;
    real inVec[vecLen] = { 0 };
    real outVec[vecLen] = { 0 };

    real SR = 48000.0;
    real attTime = .01;
    real relTime = .1;

    Generators<real> generators;
    ExpSmootherCascade<4, real> expSmoother;
   
    /* Setup smoother. */
    expSmoother.SetSR(SR);
    expSmoother.SetAttTime(attTime);
    expSmoother.SetRelTime(relTime);

    /* Fill input and output vectors to generate a CSV file. */
    generators.ProcessNoise(inVec, vecLen);
    expSmoother.Process(inVec, outVec, vecLen);
    for (size_t i = 0; i < vecLen; i++) {
		csvFile << i << "," << inVec[i] << "," << outVec[i] << "\n";
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
        expSmoother.Process(inVec, outVec, vecLen);
        auto t1 = high_resolution_clock::now();
        duration<double, std::micro> timeDuration = t1 - t0;
        times[i] = timeDuration.count();
        averageTime += timeDuration.count();

        /* Regenerate the input vector at each run. */
        generators.ProcessNoise(inVec, vecLen);
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
    std::cout << "The program has generated the file ExpSmootherCascade.csv containing one vector of input and output samples." << std::endl;

    csvFile.close();
    return 0;
}
