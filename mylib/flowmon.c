#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "unistd.h"

#define PULSE_COUNT_THRESHOLD 100
#define MAX_SAMPLES 30
#define NO_PULSE_RESET_TIME 60

static float dailyGallons = 0;
static float flowRateValueArray[MAX_SAMPLES] = {0};
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
    float sum = 0;
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
            //printf("No pulse detected for 1 minute. Resetting flow data.\n");
            resetFlowData();
            return;
        }
    }

    if (newDataFlag == 1 && pulseCount <= PULSE_COUNT_THRESHOLD && milliseconds < 5000 && milliseconds != 0) {
        //printf("Pulse count: %d, Elapsed time: %d\n", pulseCount, milliseconds);

        float flowRate = ((pulseCount / (milliseconds / 1000.0)) / 0.5) / calibrationFactor;
        flowRate = ((flowRate * 0.00026417) / (milliseconds / 1000.0)) * 60; // GPM

        //printf("Flow rate: %.2f gallons\n", flowRate);

        float flowRateGPM = flowRate * 30;
        dailyGallons += flowRate;
        *pDailyGallons = dailyGallons;

        //printf("Flow rate: %.2f GPM\n", flowRateGPM);
        //printf("Daily Gallons: %.2f\n", dailyGallons);

        if (flowRateGPM > 2.0) {
            updateFlowRate(flowRateGPM);
            *pAvgflowRateGPM = calculateAverageFlowRate();
            //printf("Average flow rate: %.2f GPM\n", *pAvgflowRateGPM);
        }
    } else {
        // Reset pulse count and elapsed time if no new data
        pulseCount = 0;
        milliseconds = 0;
    }
}
