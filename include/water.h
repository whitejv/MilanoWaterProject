
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

#define F_CLIENTID "Tank Subscriber"
#define F_TOPIC   "Formatted Sensor Data"
#define F_LEN 21

/* payload[0] =	Pressure Sensor Value
* payload[1] =	Water Height
* payload[2] =	Tank Gallons
* payload[3] =	Tank Percent Full
* payload[4] =	Current Sensor  1 Value
* payload[5] =	Current Sensor  2 Value
* payload[6] =	Current Sensor  3 Value
* payload[7] =	Current Sensor  4 Value
* payload[8] =	Firmware Version of ESP
* payload[9] =	I2C Fault Count
* payload[10] =	Cycle Count
* payload[11] =	Ambient Temperature
* payload[12] =	Float State 1
* payload[13] =	Float State 2
* payload[14] =	Float State 3
* payload[15] =	Float State 4
* payload[16] =	Pressure Switch State
* payload[17] =	House Water Pressure Value
* payload[18] =	 spare
* payload[19] =	 spare
* payload[20] =	 spare
*/	
	
float	formatted_sensor_payload[F_LEN];
	
#define M_CLIENTID "Tank Monitor"
#define M_TOPIC  "Monitor Data"
#define M_LEN 21

/* payload[0] =	 PumpCurrentSense[1];
* payload[1] =	 PumpCurrentSense[2];
* payload[2] =	 PumpCurrentSense[3];
* payload[3] =	 PumpCurrentSense[4];
* payload[4] =	 PumpLedColor[1];
* payload[5] =	 PumpLedColor[2];
* payload[6] =	 PumpLedColor[3];
* payload[7] =	 PumpLedColor[4];
* payload[8] =	 floatstate[1];
* payload[9] =	 floatstate[2];
* payload[10] =	 floatstate[3];
* payload[11] =	 floatstate[4];
* payload[12] =	 floatLedcolor[1];
* payload[13] =	 floatLedcolor[2];
* payload[14] =	 floatLedcolor[3];
* payload[15] =	 floatLedcolor[4];
* payload[16] =	spare
* payload[17] =	spare
* payload[18] =	spare
* payload[19] =	Pressure LED Color
* payload[20] =	spare
*/	
	
	
unsigned int 	monitor_sensor_payload[M_LEN];
	
#define A_CLIENTID "Tank Alert"
#define A_TOPIC  "Alert Data"
#define A_LEN 21

/* payload[0] =	spare
* payload[1] =	spare
* payload[2] =	spare
* payload[3] =	spare
* payload[4] =	spare
* payload[5] =	spare
* payload[6] =	spare
* payload[7] =	spare
* payload[8] =	spare
* payload[9] =	spare
* payload[10] =	spare
* payload[11] =	spare
* payload[12] =	spare
* payload[13] =	spare
* payload[14] =	spare
* payload[15] =	spare
* payload[16] =	spare
* payload[17] =	spare
* payload[18] =	spare
* payload[19] =	spare
* payload[20] =	spare
*/	
	
	
unsigned int 	alert_sensor_payload[A_LEN];

float Tank_Radius_sqd = 16;    //internal radius of tank in Ft squared - Example my tank is 8ft Diameter = 4ft radius = 16 when squared
unsigned short int data_payload[22] ;
unsigned short int ESP_payload[4] ;

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
