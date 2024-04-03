#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "unistd.h"

extern int verbose;

#define LITRES_TO_GALLONS 0.264172 // 1 litre = 0.264172 gallons
#define PULSE_COUNT_THRESHOLD 100
#define MAX_SAMPLES 20
#define NO_PULSE_RESET_TIME 60

static double dailyGallons = 0;
static double flowRateValueArray[MAX_SAMPLES] = {0};
static int flowIndex = 0;
static int timerNoPulse = 0;

void resetFlowData() {
    timerNoPulse = 0;
    memset(flowRateValueArray, 0, sizeof(flowRateValueArray));
    dailyGallons = 0;
}

void updateFlowRate(float flowRateGPM) {
    flowRateValueArray[flowIndex] = flowRateGPM;
    flowIndex = (flowIndex + 1) % MAX_SAMPLES; // Corrected increment operation
}

float calculateAverageFlowRate() {
    double sum = 0;
    int count = 0;

    for (int i = 0; i < MAX_SAMPLES; ++i) {
        if (flowRateValueArray[i] != 0) {
            sum += flowRateValueArray[i];
            count++;
        }
    }

    return (count > 0) ? sum / count : 0;
}

void flowmon(int newDataFlag, int milliseconds, int pulseCount, float *pAvgflowRateGPM, float *pDailyGallons, float calibrationFactor) {
    if (newDataFlag == 1) {
        timerNoPulse = 0;
    } else {
        timerNoPulse++;
        if (timerNoPulse > NO_PULSE_RESET_TIME) {
            if (verbose) printf("No pulse detected for 1 minute. Resetting flow data.\n");
            resetFlowData();
            return;
        }
    }

    if (newDataFlag == 1 && pulseCount <= PULSE_COUNT_THRESHOLD && milliseconds < 5000 && milliseconds != 0) {
      
        double elapsedTimeSec = milliseconds / 1000.0;
        double frequency = pulseCount / elapsedTimeSec;
        double flowRateLPM = 2 * frequency;
        double flowRateLPMCal = flowRateLPM * calibrationFactor;
        double flowRateGPM = flowRateLPMCal * LITRES_TO_GALLONS;
    
    // Update total gallons flowed
        double flowThisInterval = flowRateGPM * (elapsedTimeSec / 60.0); // Convert GPM to gallons for the current interval
        dailyGallons += flowThisInterval;

        *pDailyGallons = dailyGallons;

        if (verbose) {
            printf("********************************************************\n") ;
            printf("Pulse count: %d, Elapsed time: %d\n", pulseCount, milliseconds);
            printf("Elapsed Time in Seconds: %.2f\n", elapsedTimeSec);
            printf("Frequency: %.2f Hz\n", frequency);  
            printf("Flow rate: %.2f LPM\n", flowRateLPM);
            printf("Cal Flow rate: %.2f LPMCal\n", flowRateLPMCal);
            printf("Flow rate: %.2f GPM\n", flowRateGPM);
            printf(" \n");
            printf("Flow this interval: %.2f gallons\n", flowThisInterval);
            printf("Daily Gallons: %.2f\n", dailyGallons);
            printf("********************************************************\n") ;  
        }
        if (flowRateGPM > 2.0) {
            updateFlowRate(flowRateGPM);
            *pAvgflowRateGPM = calculateAverageFlowRate();
            if (verbose) printf("Average flow rate: %.2f GPM\n", *pAvgflowRateGPM);
        }
    } else {
        // Reset pulse count and elapsed time if no new data
        pulseCount = 0;
        milliseconds = 0;
    }
}