#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0
#define AUTO 2
#define VoltoGal 7.048052
#define MaxTankGal 2240. // based on current float height
#define PI 3.1415926535897932384626433832795
#define Tank_Radius_sqd 16 // radius of tank in Feet Squared - Example my tank is 8ft diameter so 4ft radius = 16 when sqd

#define GREEN 0
#define BLUE 1
#define ORANGE 2
#define RED 3

//#define ADDRESS "192.168.1.250:1883" // Local RaspberryPI RaspiCM4 Production MQTT Server
//#define ADDRESS "192.168.1.249:1883" // Local RaspberryPI 400 Development MQTT Server

const char ssid[] = "ATT9LCV8fL_2.4";
const char password[] = "6jhz7ai7pqy5";

/* Define IP Address for MQTT for both
 * a Production Server and a Development Server
 */
#define PROD_MQTT_IP "192.168.1.250"
#define PROD_MQTT_PORT 1883
#define DEV_MQTT_IP "192.168.1.249"
#define DEV_MQTT_PORT 1883

#define QOS 0
#define TIMEOUT 10000L

#define datafile "/home/pi/MWPLogData/datafile.txt"
#define pumpdata "/home/pi/MWPLogData/pumpdata.txt"
#define flowdata "/home/pi/MWPLogData/flowdata.txt"
#define flowfile "/home/pi/MWPLogData/flowfile.txt"

/*
 *  Rainbird command and response data
 */
#define MAX_ZONES 24

typedef struct {
    int station;
    int status;
} Station;

typedef struct {
    int controller;
    Station states[MAX_ZONES];
} Controller;

Controller Front_Controller = {0};
Controller Back_Controller = {0};

char rainbird_payload[255];
char rainbird_command1[] = "check1";
char rainbird_command2[] = "check2";

/* Library function prototypes */

void log_message(const char *format, ...);
void log_test(int verbose, int log_level, int msg_level, const char *format, ...);
void parse_message(char* message, Controller* controller) ;

/*
 * Tank Test is for Automated Testing 
 */

#define T_CLIENTID "Tank Test"

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

#define TANK_MONID "Tank Monitor"
#define TANK_TOPIC "Tank Data"
#define TANK_DATA  25
/* payload[0] =	Tank Gallons Per Minute
* payload[1] =	Water Height
* payload[2] =	Tank Gallons
* payload[3] =	Tank Percent Full
* payload[4] =	Tank Total Gallons (24hrs)
* payload[5] =	 spare
* payload[6] =	 spare
* payload[7] =	 spare
* payload[8] =	 spare
* payload[9] =	 spare
* payload[10] =	Cycle Count
* payload[11] =	Air Temperature
* payload[12] =	Float State 1
* payload[13] =	Float State 2
* payload[14] =	Float State 3
* payload[15] =	Float State 4
* payload[16] =	 spare
* payload[17] =	 spare
* payload[18] =	 spare
* payload[19] =	 spare
* payload[20] =	 spare
*/	

float	tank_sensor_payload[TANK_DATA];

struct TankMonitorData {		
float	tank_gallons_per_minute	;
float	water_height	;
float	tank_gallons	;
float	tank_per_full	;
float	tank_total_gallons_24	;
float	spare1	;
float	spare2	;
float	spare3	;
float	spare4	;
float	spare5	;
float	cycle_count	;
float	air_temp	;
float	float_state_1	;
float	float_state_2	;
float	float_state_3	;
float	float_state_4	;
float	spare6	;
float	spare7	;
float	spare8	;
float	spare9	;
float	spare10	;
};

char* TankMonitorData_var_name[] = {
"tank_gallons_per_minute",
"water_height",
"tank_gallons",
"tank_per_full",
"tank_total_gallons_24",
"spare",
"spare",
"spare",
"spare",
"spare",
"cycle_count",
"air_temp",
"float_state_1",
"float_state_2",
"float_state_3",
"float_state_4",
"spare",
"spare",
"spare",
"spare",
"spare"
};

#define WELL_MONID	 "Well Monitor"
#define WELL_TOPIC    "Well Data" 
#define WELL_DATA 25
/* payload[0] =	Well Pump 1 On
* payload[1] =	Well Pump 2 On
* payload[2] =	Well Pump 3 On
* payload[3] =	Irrigation Pump On
* payload[4] =	House Water Pressure
* payload[5] =	House Tank Pressure Switch On
* payload[6] =	Septic Alert On
* payload[7] =	spare
* payload[8] =	spare
* payload[9] =	spare
* payload[10] =	System Temp
* payload[11] =	spare
* payload[12] =	Cycle Count
* payload[13] =	spare
* payload[14] =	spare
* payload[15] =	spare
* payload[16] =	spare
* payload[17] =	spare
* payload[18] =	spare
* payload[19] =	spare
* payload[20] =	spare
*/	
	
float	well_sensor_payload[WELL_DATA];
	
struct WellMonitorData {		
float	well_pump_1_on	;
float	well_pump_2_on	;
float	well_pump_3_on	;
float	irrigation_pump_on	;
float	house_water_pressure	;
float	House_tank_pressure_switch_on	;
float	septic_alert_on	;
float	spare7	;
float	spare8	;
float	spare9	;
float	system_temp	;
float	spare11	;
float	cycle_count	;
float	spare13	;
float	spare14	;
float	spare15	;
float	spare16	;
float	spare17	;
float	spare18	;
float	spare19	;
float	spare20	;
};		

char* WellMonitorData_var_name[] = {
"well_pump_1_on",
"well_pump_2_on",
"well_pump_3_on",
"irrigation_pump_on",
"house_water_pressure",
"House_tank_pressure_switch_on",
"septic_alert_on",
"spare7",
"spare8",
"spare9",
"system_temp",
"spare11",
"cycle_count",
"spare13",
"spare14",
"spare15",
"spare16",
"spare17",
"spare18",
"spare19",
"spare20"
};

#define FLOW_MONID  "Flow Monitor"
#define FLOW_TOPIC  "Flow Data"
#define FLOW_DATA 25
/* payload[0] =	Gallons Per Minute
* payload[1] =	Total Gallons (24 Hrs)
* payload[2] =	Irrigation Pressure
* payload[3] =	Pump Temperature
* payload[4] =	Front Rainbird On
* payload[5] =	Front Active Zone
* payload[6] =	Back Rainbird On
* payload[7] =	Back Active Zone
* payload[8] =	spare
* payload[9] =	spare
* payload[10] =	Cycle Count
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

float	flow_sensor_payload[FLOW_DATA];


struct FlowMonitorData {		
float	irrigationFlowPerMin	;
float	irrigationTotalFlow	;
float	irrigationPressure	;
float	irrigationPumpTemp	;
float	frontRainbirdOn	;
float	frontActiveZone	;
float	backRainbirdOn	;
float	backActiveZone	;
float	spare8	;
float	spare9	;
float	cycle_count	;
float	spare11	;
float	spare12	;
float	spare13	;
float	spare14	;
float	spare15	;
float	spare16	;
float	spare17	;
float	spare18	;
float	spare19	;
float	spare20	;
};		

char* FlowMonitorData_var_name[] = {
"irrigationFlowPerMin",
"irrigationTotalFlow",
"irrigationPressure",
"irrigationPumpTemp",
"frontRainbirdOn",
"frontActiveZone",
"backRainbirdOn",
"backActiveZone",
"spare8",
"spare9",
"cycle count",
"spare11",
"spare12",
"spare13",
"spare14",
"spare15",
"spare16",
"spare17",
"spare18",
"spare19",
"spare20"
};

#define MON_ID	  "Monitor"  
#define M_TOPIC  "Monitor Data" 
#define M_LEN 25

/* payload[0] =    PumpCurrentSense[1];
 * payload[1] =    PumpCurrentSense[2];
 * payload[2] =    PumpCurrentSense[3];
 * payload[3] =    PumpCurrentSense[4];
 * payload[4] =    PumpLedColor[1];
 * payload[5] =    PumpLedColor[2];
 * payload[6] =    PumpLedColor[3];
 * payload[7] =    PumpLedColor[4];
 * payload[8] =    PumpRunCount;  //byte4-pump4;byte3-pump3;byte2-pump2;byte1-pump1
 * payload[9] =   PumpRunTime{1] ; //Seconds
 * payload[10] =   PumpRunTime{2] ; //Seconds
 * payload[11] =   PumpRunTime{3] ; //Seconds
 * payload[12] =   PumpRunTime{4] ; //Seconds
 * payload[13] =    43floatState;  //byte34-float4;byte123-float3
 * payload[14] =    21floatState;  //bytes34-float2;byte12-float1
 * payload[15] =    AllfloatLedcolor;  //byte4-color4;byte3-color3;byte2-color2;byte1-color1
 * payload[16] =   Septic Relay Alert
 * payload[17] =   Septic Relay Alert Color
 * payload[18] =   Pressure Relay Sense
 * payload[19] =   Pressure LED Color
 * payload[20] =   spare
 */

int monitor_payload[M_LEN];

struct MonitorData {		
int	pump_current_sense_1	;
int	pump_current_sense_2	;
int	pump_current_sense_3	;
int	pump_current_sense_4	;
int	pump_led_color_1	;
int	pump_led_color_2	;
int	pump_led_color_3	;
int	pump_led_color_4	;
int	pump_run_count	;
int	pump_run_time_1	;
int	pump_run_time_2	;
int	pump_run_time_3	;
int	pump_run_time_4	;
int	float_state_43	;
int	float_state_21	;
int	all_float_led_colors	;
int	septic_relay_alert	;
int	septic_relay_alert_color	;
int	press_relay_sense	;
int	press_led_color	;
int	spare	;
};	

char* MonData_var_names[] = {
"pump_current_sense_1",
"pump_current_sense_2",
"pump_current_sense_3",
"pump_current_sense_4",
"pump_led_color_1",
"pump_led_color_2",
"pump_led_color_3",
"pump_led_color_4",
"pump_run_count",
"pump_run_time_1",
"pump_run_time_2",
"pump_run_time_3",
"pump_run_time_4",
"float_state_43",
"float_state_21",
"all_float_led_colors",
"septic_relay_alert",
"septic_relay_alert_color",
"press_relay_sense",
"press_led_color",
"spare"
};

#define ALERT_ID	 "Sys Alert"  
#define A_TOPIC    "Alert Data" 
#define A_LEN 25
/* payload[0] =   spare
 * payload[1] =   spare
 * payload[2] =   spare
 * payload[3] =   spare
 * payload[4] =   spare
 * payload[5] =   spare
 * payload[6] =   spare
 * payload[7] =   spare
 * payload[8] =   spare
 * payload[9] =   spare
 * payload[10] =   spare
 * payload[11] =   spare
 * payload[12] =   spare
 * payload[13] =   spare
 * payload[14] =   spare
 * payload[15] =   spare
 * payload[16] =   spare
 * payload[17] =   spare
 * payload[18] =   spare
 * payload[19] =   spare
 * payload[20] =   spare
 */

int alert_payload[A_LEN];

struct AlertData {		
int	alarm1	;
int	alarm2	;
int	alarm3	;
int	alarm4	;
int	alarm5	;
int	alarm6	;
int	alarm7	;
int	alarm8	;
int	alarm9	;
int	alarm10	;
int	alarm11	;
int	alarm12	;
int	alarm13	;
int	alarm14	;
int	alarm15	;
int	alarm16	;
int	alarm17	;
int	alarm18	;
int	alarm19	;
int	alarm20	;
};	

char *AlertData_var_names[] = {
"spare1",
"spare2",
"spare3",
"spare4",
"spare5",
"spare6",
"spare7",
"spare8",
"spare9",
"spare10",
"spare11",
"spare12",
"spare13",
"spare14",
"spare15",
"spare16",
"spare17",
"spare18",
"spare19",
"spare20"
};

/*************************************************
 ************************************************* 
 * IOT Micro-Controller Input Devices Start Here *
 ************************************************* 
 *************************************************/

/* Generic FLow Client to Support 3 Flow Sensors */

#define FLOWSENSOR 13
#define TEMPSENSOR 2
#define CONFIGPIN1  12 //GPIO 12
#define CONFIGPIN2  3  //GPIO 3
#define DISCINPUT1  4  //GPIO 4 Input with Pullup
#define DISCINPUT2  5  //GPIO 5 Input with Pullup
#define GenericFLowMSGSize 10
#define IRRIGATION_CLIENTID	 "Irrigation Flow Client"
#define IRRIGATION_CLIENT   "Irrigation Flow Payload"
#define IRRIGATION_LEN GenericFLowMSGSize
#define TANK_CLIENTID	 "Tank Flow Client"
#define TANK_CLIENT   "TankFlow  Payload"
#define TANK_LEN GenericFLowMSGSize
#define HOUSE_CLIENTID	 "House Flow Client"
#define HOUSE_CLIENT   "House Flow Payload"
#define HOUSE_LEN GenericFLowMSGSize
#define SPARE_CLIENTID	 "Spare Flow  Client"
#define SPARE_CLIENT   "Spare Flow  Payload"
#define SPARE_LEN GenericFLowMSGSize

struct flowSensorConfigTable
{
   int  sensorID;         // Sensor ID
   char sensorName[35];    // Sensor Name  
   char clientid[35] ;     // Client ID
   char messageid[35];     // Message ID
   int  messagelen ;       // Message Length
};

struct flowSensorConfigTable flowSensorConfig[4] = {
    {0, "SPARE",         SPARE_CLIENTID, SPARE_CLIENT,           GenericFLowMSGSize},
    {1, "TANK",          TANK_CLIENTID,     TANK_CLIENT,         GenericFLowMSGSize},
    {2, "IRRIGATION",    IRRIGATION_CLIENTID, IRRIGATION_CLIENT, GenericFLowMSGSize},
    {3, "HOUSE",         HOUSE_CLIENTID, HOUSE_CLIENT,           GenericFLowMSGSize}
};
	
int	flow_data_payload[GenericFLowMSGSize] ;
int	irrigation_data_payload[IRRIGATION_LEN] ;
int	tank_data_payload[TANK_LEN] ;
int	house_data_payload[HOUSE_LEN] ;
int	spare_data_payload[SPARE_LEN] ;

/* 
* payload 0	Pulses Counted in Time Window
* payload 1	Number of milliseconds in Time Window
* payload 2	Flag 1=new data 0=stale data
* payload 3	ADC Raw Sensor value (int/hex)
* payload 4	GPIO Sensor Data valuse (int/hex)
* payload 5	Temp f (int)
* payload 6	Temperature in F Float Bytes 1&2
* payload 7	Temperature in F Float Bytes 3&4
* payload 8	 Cycle Counter
* payload 9	 FW Version 4 Hex 
*/	
		
 	

struct FlowClientData {		
int	pulse_count	;
int	millisecnods	;
int	new_data_flag	;
int adc_sensor;
int gpio_sensor;
int	temp	    ;
int	temp_f_1	;
int	temp_f_2	;
int	cycle_count	;
int	fw_version	;
}	;	

char* FlowClientData_var_name [] = {
"pulse_count",
"millisecnods",
"new_data_flag",
"temperature",
"temp_float_1",
"temp_float_2",
"cycle_count",
"fw_version"
};
char* IrrigationClientData_var_name [] = {	
    "irrigation_pulse_count",
    "irrigation_millisecnods",
    "irrigation_new_data_flag",
    "irrigation_adc_sensor",
    "irrigation_gpio_sensor",
    "irrigation_temp",
    "irrigation_temp_w1",
    "irrigation_temp_w2",
    "irrigation_cycle_count",
    "irrigation_fw_version"};
char* TankClientData_var_name [] = {	
    "tank_pulse_count",
    "tank_millisecnods",
    "tank_new_data_flag",
    "tank_adc_sensor",
    "tank_gpio_sensor",
    "tank_temp",
    "tank_temp_w1",
    "tank_temp_w2",
    "tank_cycle_count",
    "tank_fw_version"};
char* HouseClientData_var_name [] = {	
    "house_pulse_count",
    "house_millisecnods",
    "house_new_data_flag",
    "house_adc_sensor",
    "house_gpio_sensor",
    "house_temp",
    "house_temp_w1",
    "house_temp_w2",
    "house_cycle_count",
    "house_fw_version"};

#define WELL_CLIENTID  "Well Client" 
#define WELL_CLIENT    "Well Payload"
#define WELL_LEN 25
/* payload 0	 A0 Raw Sensor Current Sense Well 1 16bit
* payload 1	 A1 Raw Sensor Current Sense Well 2 16bit
* payload 2	 A2 Raw Sensor Current Sense Well 3 16bit
* payload 3	 A3 Raw Sensor Current Sense Irrigation Pump 16bit
* payload 4	 A7 Sensor House Water Pressure 16bit ADC 0-5v
* payload 5	D2 House Tank Pressure Switch (0=active)
* payload 6	D3 Septic Alert (0=active)
* payload 7	 unused
* payload 8	 unused
* payload 9	 unused
* payload 10	 Raw Temp Celcius
* payload 11	 unused
* payload 12	 Cycle Counter 16bit Int
* payload 13	 spare
* payload 14	 spare
* payload 15	 spare
* payload 16	 spare
* payload 17	 spare
* payload 18	 spare
* payload 19	 spare
* payload 20	 FW Version 4 Hex 
*/	

int	well_data_payload[WELL_LEN] ;
 	

struct WellClientDatat {		
int	raw_current_sense_well1	;
int	raw_current_sense_well2	;
int	raw_current_sense_well3	;
int	raw_current_sense_irrigation_pump	;
int	house_water_pressure	;
int	house_tank_pressure_switch_on	;
int	septic_alert_on	;
int	spare_1	;
int	spare_2	;
int	spare_3	;
int	raw_temp_celcius	;
int	spare	;
int	cycle_count	;
int	spare_4	;
int	spare_5	;
int	spare_6	;
int	spare_7	;
int	spare_8	;
int	spare_9	;
int	spare_10	;
int	 FW_version_4_hex 	;
};

char* WellClientData_var_name [] = {
"raw_current_sense_well1",
"raw_current_sense_well2",
"raw_current_sense_well3",
"raw_current_sense_irrigation_pump",
"house_water_pressure",
"house_tank_pressure_switch_on",
"septic_alert_on",
"spare_1",
"spare_2",
"spare_3",
"raw_temp_celcius",
"spare",
"cycle_count",
"spare_4",
"spare_5",
"spare_6",
"spare_7",
"spare_8",
"spare_9",
"spare_10",
"FW_version_4_hex"
} ;

#define TANKGAL_CLIENTID "TankGal Client"
#define TANKGAL_CLIENT   "TankGal Payload"
#define TANKGAL_LEN 25
/* payload 0	Water Surface Dist (in.)
* payload 1	Humidity
* payload 2	Temperature
* payload 3	 spare
* payload 4	 spare
* payload 5	 spare
* payload 6	 spare
* payload 7	 spare
* payload 8	 spare
* payload 9	 spare
* payload 10	Raw System Temp Celsius
* payload 11	 spare
* payload 12	cycle count
* payload 13	 spare
* payload 14	 spare
* payload 15	 spare
* payload 16	 spare
* payload 17	 spare
* payload 18	 spare
* payload 19	 spare
* payload 20	 spare
*/	

int	tankgal_data_payload[TANKGAL_LEN] ;

struct TankGalClientData {	
    int water_surface_dist	;
    int humidity	;
    int temperature	;
    int spare_1	;
    int spare_2	;
    int spare_3	;
    int spare_4	;
    int spare_5	;
    int spare_6	;
    int spare_7	;
    int system_temp_celsius	;
    int spare_9	;
    int cycle_count	;
    int spare_10	;
    int spare_11	;
    int spare_12	;
    int spare_13	;
    int spare_14	;
    int spare_15	;
    int spare_16	;
    int spare_17	;
    }	;
  
char* TankGalClientData_var_name [] = {	
"water_surface_dist"	,
"humidity"	,
"temperature"	,
"spare_1"	,
"spare_2"	,
"spare_3"	,
"spare_4"	,
"spare_5"	,
"spare_6"	,
"spare_7"	,
"system_temp_celsius"	,
"spare_9"	,
"cycle_count"	,
"spare_10"	,
"spare_11"	,
"spare_12"	,
"spare_13"	,
"spare_14"	,
"spare_15"	,
"spare_16"	,
"spare_17"	
};  

int ESP_payload[4];

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

#define NPumps 4                             // Number of active pumps
#define PumpInterval 4000                    // 4000 millisecs is 4 sec interval to run routine
#define PumpCalcInterval 60000 / RunInterval // Number of cycles per minute
#define PumpSwitchInterval PumpCalcInterval *PumpCycleTime
#define PumpCycleTime 30 // Each pump runs in a 30 minute cycle
#define Pump1ScaleFactor 5.618144
#define Pump2ScaleFactor 6.154596
#define Pump3ScaleFactor 5.0
#define Pump12ScaleFactor 11.28648

struct pump
{

   int RunCommanded;      // on/off
   int RunStatus;         // on/off
   int PumpPower;         // on/off
   int RunTimeCommanded;  // seconds
   int RunTimeActual;     // seconds
   int CycleTime;         // seconds
   float PumpScaleFactor; // GPM Measured Scale Factor
   float FlowRate;        // GPM
   float TotalFlow;       // gallons
   int TimestampOn;       // timestamp
};

struct pump Pump[NPumps + 1]; // Add 1 so that I can start at 1,2,3 not 0

/*
  *  Pump Run Count and Run Time
  */

struct PumpStats
{
   int PumpOn;          // TRUE - Pump is in On state
   int PumpOnTimeStamp; // Time Stamp Last Time Pump Was Running
   int PumpLastState;   // Pump on/off Last Time Checked
   int RunCount;        // Number of Pump Cycles in 24 hours
   int RunTime;         // Run Time in Seconds in 24 Hours
   int StartGallons;    // Tank Gallon Reading when pump starts
   int StopGallons;     // Tank Gallons Reading when pump stops
};

struct PumpStats MyPumpStats[NPumps + 1];
// = {  0,0,0,0,0,  //Add 1 so that I can refer to pumps starting with 1,2,3,4 and not 0
//      0,0,0,0,0,
//      0,0,0,0,0,
//      0,0,0,0,0,
//      0,0,0,0,0 };
