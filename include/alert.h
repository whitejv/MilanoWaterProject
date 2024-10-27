#define ALARM_COUNT 20

enum AlarmInternalState
{
   inactive = 0,
   trigger = 1,
   active = 2,
   reset = 3,
   timeout = 4
};

typedef enum {Critical, Warn, Info} AlarmType;

typedef enum {CONDITION_EQUAL, CONDITION_NOT_EQUAL, CONDITION_GREATER_THAN, CONDITION_LESS_THAN} TriggerCondition;

typedef struct {
    int alarmID;
    AlarmType type;
    char label[256];
    TriggerCondition conditional1;
    int trigValue1;
    TriggerCondition conditional2;
    int trigValue2;
    char eventMessage[256];

} AlarmInfo;


AlarmInfo alarmInfo[] = {

    {0, Info, "spare", 0, 0, 0, 0, "spare"},
    {1, Critical, "Tank Level Critically Low", CONDITION_EQUAL, TRUE, 0, 0, "Water Level in Tank is Critically Low"},
    {2, Warn, "Tank Overfill Condition", CONDITION_GREATER_THAN, 2000, 0, 0, "Tank Overfill Detected"},
    {3, Critical, "Irrigation Pump Temp Low", CONDITION_LESS_THAN, 55, 0, 0, "Irrigation Pump Temperature Exceeding Low Limit"},
    {4, Critical, "House Supply Temp Low", CONDITION_LESS_THAN, 55, 0, 0, "House Supply Temperature Exceeding Low Limit"},
    {5, Critical, "House Water Pressure Low", CONDITION_LESS_THAN, 40, 0, 0, "Household Water Pressure Critically Low"},
    {6, Critical, "Irrigation Water Pressure Low", CONDITION_LESS_THAN, 40, 0, 0, "Household Water Pressure Critically Low"},
    {7, Critical, "Septic System Alert", CONDITION_EQUAL, TRUE, 0, 0, "Septic System Malfunction"},
    {8, Critical, "Irrigation Pump Run Away", CONDITION_EQUAL, TRUE, CONDITION_LESS_THAN, CONDITION_LESS_THAN, "Irrigation Pump Running with No Flow"},
    {9, Critical, "Well 3 Pump Run Away", CONDITION_EQUAL, TRUE, CONDITION_LESS_THAN, CONDITION_LESS_THAN, "Well 3 Pump Running with No Flow"},
    {10, Warn, "Well 1 Not Starting", CONDITION_EQUAL, TRUE, CONDITION_EQUAL, CONDITION_EQUAL, "Water Well Pumps Failed to Start"},
    {11, Warn, "Well 2 Not Starting", CONDITION_EQUAL, TRUE, CONDITION_EQUAL, CONDITION_EQUAL, "Water Well Pumps Failed to Start"},
    {12, Warn, "Well 3 Not Starting", CONDITION_EQUAL, TRUE, CONDITION_LESS_THAN, CONDITION_LESS_THAN, "Water Well #3 Pump ON but Flow is Less Than 2 Gal per Minute"},
    {13, Warn, "Well 1 Runtime Exceeded", CONDITION_GREATER_THAN, 25, 0, 0, "Water Well Run Time Exceeds 25 Minutes Total"},
    {14, Info, "Well 1 Cycles Excessive", CONDITION_GREATER_THAN, 15, 0, 0, "Water Well Cycles Exceeds 15 per Day"},
    {15, Warn, "Well 2 Runtime Exceeded", CONDITION_GREATER_THAN, 25, 0, 0, "Water Well Run Time Exceeds 25 Minutes Total"},
    {16, Info, "Well 2 Cycles Excessive", CONDITION_GREATER_THAN, 15, 0, 0, "Water Well Cycles Exceeds 15 per Day"},
    {17, Warn, "Well 3 Runtime Exceeded", CONDITION_GREATER_THAN, 300, 0, 0, "Water Well Run Time Exceeds 25 Minutes Total"},
    {18, Info, "Well 3 Cycles Excessive", CONDITION_GREATER_THAN, 5, 0, 0, "Water Well Cycles Exceeds 15 per Day"},
    {19, Warn, "Irrigation Runtime Exceeded", CONDITION_GREATER_THAN, 300, 0, 0, "Water Well Run Time Exceeds 25 Minutes Total"},
    {20, Info, "Irrigation Cycles Excessive", CONDITION_GREATER_THAN, 20, 0, 0, "Water Well Cycles Exceeds 15 per Day"},
};

typedef struct {
    int alarmState;      // Alarm state
    int alarmStatePrior; // Last Alarm State
    int internalState;   // field for internal state
    int timer;           // field for timer
    int timeOut;         // field for timeOut
    int triggerDelay ;   // field for triggerDelay
    int eventSend;       // send event to Blynk Flag
    int eventSent;       // event has been sent to Blynk
    int occurences;      // Occurences since last reset normally at midnight
    const int TIMEOUT_RESET_VALUE ;
    const int TIMER_INCREMENT ;
    const int TRIGGER_VALUE ;
    const int TRIGGER_DELAY ;
} AlarmStructure;