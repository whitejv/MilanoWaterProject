
//#include "json.h"

/* Use below for free Cloud MQTT Server */
//#define ADDRESS     "tcp://soldier.cloudmqtt.com:15599"
//#define mqttPort 15599
//#define mqttUser "zerlcpdf"
//#define mqttPassword  "OyHBShF_g9ya"

#define TRUE 1  //now defined in json.h
#define FALSE 0 //now defined in json.h
#define ON 1
#define OFF 0
#define AUTO 2
#define VoltoGal 7.048052
#define MaxTankGal  2290.  //based on current float height
#define PI 3.1415926535897932384626433832795


#define GREEN   0
#define BLUE    1
#define ORANGE  2
#define RED     3

#define ADDRESS     "192.168.1.154:1883" //Local RaspberryPI as MQTT Server
#define QOS         1
#define TIMEOUT     10000L

/*
 * Data Block Interface Control
 */

/*
 * Data Word 0:  CH1: 4-20 mA Raw Sensor HydroStatic Pressure 16bit
 * Data Word 1:  CH2: Raw Sensor Current Sense Well 1 16bit
 * Data Word 2:  CH3: Raw Sensor Current Sense Well 2 16bit
 * Data Word 3:  CH4: Raw Sensor Current Sense Well 3 16bit
 * Data Word 4:  GPIO 8 bits Hex (8 bits: 0-3 floats, 4 Pump Relay Command)
 * Data Word 5:  Raw Temp Celcius
 * Data Word 6:  Flow Sensor Count 16bit Int
 * Data Word 7:  Flow Sensor Period 16bit Int
 * Data Word 8:  CH1: Spare 4-20 mA input
 * Data Word 9:  CH2: Raw Sensor House Water Pressure 16bit ADC 0-5v
 * Data Word 10: CH3: Unused 2 16bit
 * Data Word 11: CH4: Raw Sensor Current Sense Irrigation pump 4 (16bit)
 * Data Word 12: Cycle Counter 16bit Int
 * Data Word 13: spare
 * Data Word 14: spare
 * Data Word 15: spare
 * Data Word 16: I2C Panic Count 16bit Int
 * Data Word 17: TMP100 I2C Error
 * Data Word 18: MCP23008 I2C Error
 * Data Word 19: MCP3428 I2C Error
 * Data Word 20: FW Version 4 Hex
 */
/*
 * Data Word 21: Last Data Word is Control Word From User
 */

float Tank_Radius_sqd = 16;    //internal radius of tank in Ft squared - Example my tank is 8ft Diameter = 4ft radius = 16 when squared
unsigned short int data_payload[22] ;
unsigned short int ESP_payload[4] ;
float formatted_sensor_payload [21];
unsigned short int monitor_sensor_payload[21];
unsigned short int alert_sensor_data[21];

int firmware = 0;
int SubFirmware = 0x80FF;


/*
 *  Float Management
 */

int DebounceFloatInProgress = FALSE;
int HighFloat = FALSE;
int LowFloat = FALSE;
int commandAlarmOn = 0;
int readFloatHi = 0;
int readFloatLow = 0;

/*
 *  Pump Status and Control
 */

#define NPumps 4                             //Number of active pumps
#define PumpInterval 4000                   //4000 millisecs is 4 sec interval to run routine
#define PumpCalcInterval 60000/RunInterval  //Number of cycles per minute
#define PumpSwitchInterval PumpCalcInterval*PumpCycleTime
#define PumpCycleTime 30                    //Each pump runs in a 30 minute cycle
#define Pump1ScaleFactor 5.618144
#define Pump2ScaleFactor 6.154596
#define Pump3ScaleFactor 5.0
#define Pump12ScaleFactor 11.28648


struct pump
{

   int RunCommanded;      //on/off
   int RunStatus;         //on/off
   int PumpPower;         //on/off
   int RunTimeCommanded;  //seconds
   int RunTimeActual;     //seconds
   int CycleTime;         //seconds
   float PumpScaleFactor; //GPM Measured Scale Factor
   float FlowRate;        //GPM
   float TotalFlow;       //gallons
   int TimestampOn;       //timestamp
};
