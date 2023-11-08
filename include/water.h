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

int ESP_payload[4];

int firmware = 0;
int SubFirmware = 0x80FF;

//#define MilanoWaterProject\IrrigationHeader.h  (created and maintained in Excel)
/*********************************************************************************
 *********************************************************************************
 **** |||           |||           |||           |||          |||          ||| ****
 **** vvv           vvv           vvv           vvv          vvv          vvv ****
 **** Data Below This Line is Auto Generated from the Excel Spreadsheet Above ****
 ****                                                                         ****
 *********************************************************************************
 ********************************************************************************/

/*
* Block ID: S001D
* Block Name: irrigationSens
* Description: irrigation Flow Data
* From: irrigation
* To: irrigation Flow Monitor
* Rate: 1 hz
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: mwp/data/sensor/S001D/flow/irrigation
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            millisecnods                Number of milliseconds in Time Window        0        10000        2000
*  2        int            new_data_flag                Flag 1=new data 0=stale data        0        1        1
*  3        int            adc_sensor                ADC Raw Sensor value: Bit 0-9 (0-1023)        0        1023        512
*  4        int            gpio_sensor                GPIO Sensor Data: Bit 1: GPIO 4, Bit 2: GPIO 5        0        3        3
*  5        int            temp                Temp f (int)        -32        150        80
*  6        int            temp_w1                Temperature in F Float Bytes 1&2                        
*  7        int            temp_w2                Temperature in F Float Bytes 3&4                        
*  8        int            cycle_count                 Cycle Counter        0        28800        
*  9        int            fw_version                 FW Version 4 Hex                         
*/

const char IRRIGATIONSENS_CLIENTID[] =    "Irrigation Flow Client" ;
const char IRRIGATIONSENS_TOPICID[] =  "mwp/data/sensor/S001D/flow/irrigation";
#define IRRIGATIONSENS_LEN 10

union   IRRIGATIONSENS_  {
   int     data_payload[IRRIGATIONSENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   millisecnods    ;
      int   new_data_flag    ;
      int   adc_sensor    ;
      int   gpio_sensor    ;
      int   temp    ;
      int   temp_w1    ;
      int   temp_w2    ;
      int   cycle_count    ;
      int   fw_version    ;
   }  irrigation  ;
}  ;
union  IRRIGATIONSENS_  irrigationSens_  ;

char* irrigationsens_ClientData_var_name [] = { 
    "irrigationSens_pulse_count",
    "irrigationSens_millisecnods",
    "irrigationSens_new_data_flag",
    "irrigationSens_adc_sensor",
    "irrigationSens_gpio_sensor",
    "irrigationSens_temp",
    "irrigationSens_temp_w1",
    "irrigationSens_temp_w2",
    "irrigationSens_cycle_count",
    "irrigationSens_fw_version",
}  ;

/*
* Block ID: S002D
* Block Name: tankSens
* Description: tank Flow Data
* From: tank
* To: tank Flow Monitor
* Rate: 1 hz
* MQTT Client ID: Tank Flow Client
* MQTT Topic ID: mwp/data/sensor/S002D/flow/tank
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            millisecnods                Number of milliseconds in Time Window        0        10000        2000
*  2        int            new_data_flag                Flag 1=new data 0=stale data        0        1        1
*  3        int            adc_sensor                ADC Raw Sensor value: Bit 0-9 (0-1023)        0        1023        512
*  4        int            gpio_sensor                GPIO Sensor Data: Bit 1: GPIO 4, Bit 2: GPIO 5        0        3        3
*  5        int            temp                Temp f (int)        -32        150        80
*  6        int            temp_w1                Temperature in F Float Bytes 1&2                        
*  7        int            temp_w2                Temperature in F Float Bytes 3&4                        
*  8        int            cycle_count                 Cycle Counter        0        28800        
*  9        int            fw_version                 FW Version 4 Hex                         
*/

const char TANKSENS_CLIENTID[] =    "Tank Flow Client" ;
const char TANKSENS_TOPICID[] =  "mwp/data/sensor/S002D/flow/tank";
#define TANKSENS_LEN 10

union   TANKSENS_  {
   int     data_payload[TANKSENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   millisecnods    ;
      int   new_data_flag    ;
      int   adc_sensor    ;
      int   gpio_sensor    ;
      int   temp    ;
      int   temp_w1    ;
      int   temp_w2    ;
      int   cycle_count    ;
      int   fw_version    ;
   }  tank  ;
}  ;
union  TANKSENS_  tankSens_  ;

char* tanksens_ClientData_var_name [] = { 
    "tankSens_pulse_count",
    "tankSens_millisecnods",
    "tankSens_new_data_flag",
    "tankSens_adc_sensor",
    "tankSens_gpio_sensor",
    "tankSens_temp",
    "tankSens_temp_w1",
    "tankSens_temp_w2",
    "tankSens_cycle_count",
    "tankSens_fw_version",
}  ;

/*
* Block ID: S003D
* Block Name: houseSens
* Description: house Flow Data
* From: house
* To: house Flow Monitor
* Rate: 1 hz
* MQTT Client ID: House Flow Client
* MQTT Topic ID: mwp/data/sensor/S002D/flow/house
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            millisecnods                Number of milliseconds in Time Window        0        10000        2000
*  2        int            new_data_flag                Flag 1=new data 0=stale data        0        1        1
*  3        int            adc_sensor                ADC Raw Sensor value: Bit 0-9 (0-1023)        0        1023        512
*  4        int            gpio_sensor                GPIO Sensor Data: Bit 1: GPIO 4, Bit 2: GPIO 5        0        3        3
*  5        int            temp                Temp f (int)        -32        150        80
*  6        int            temp_w1                Temperature in F Float Bytes 1&2                        
*  7        int            temp_w2                Temperature in F Float Bytes 3&4                        
*  8        int            cycle_count                 Cycle Counter        0        28800        
*  9        int            fw_version                 FW Version 4 Hex                         
*/

const char HOUSESENS_CLIENTID[] =    "House Flow Client" ;
const char HOUSESENS_TOPICID[] =  "mwp/data/sensor/S002D/flow/house";
#define HOUSESENS_LEN 10

union   HOUSESENS_  {
   int     data_payload[HOUSESENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   millisecnods    ;
      int   new_data_flag    ;
      int   adc_sensor    ;
      int   gpio_sensor    ;
      int   temp    ;
      int   temp_w1    ;
      int   temp_w2    ;
      int   cycle_count    ;
      int   fw_version    ;
   }  house  ;
}  ;
union  HOUSESENS_  houseSens_  ;

char* housesens_ClientData_var_name [] = { 
    "houseSens_pulse_count",
    "houseSens_millisecnods",
    "houseSens_new_data_flag",
    "houseSens_adc_sensor",
    "houseSens_gpio_sensor",
    "houseSens_temp",
    "houseSens_temp_w1",
    "houseSens_temp_w2",
    "houseSens_cycle_count",
    "houseSens_fw_version",
}  ;

/*
* Block ID: S004D
* Block Name: spareSense
* Description: spare Flow Data
* From: spare
* To: spare Flow Monitor
* Rate: 1 hz
* MQTT Client ID: Spare Flow Client
* MQTT Topic ID: mwp/data/sensor/S002D/flow/spare
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            millisecnods                Number of milliseconds in Time Window        0        10000        2000
*  2        int            new_data_flag                Flag 1=new data 0=stale data        0        1        1
*  3        int            adc_sensor                ADC Raw Sensor value: Bit 0-9 (0-1023)        0        1023        512
*  4        int            gpio_sensor                GPIO Sensor Data: Bit 1: GPIO 4, Bit 2: GPIO 5        0        3        3
*  5        int            temp                Temp f (int)        -32        150        80
*  6        int            temp_w1                Temperature in F Float Bytes 1&2                        
*  7        int            temp_w2                Temperature in F Float Bytes 3&4                        
*  8        int            cycle_count                 Cycle Counter        0        28800        
*  9        int            fw_version                 FW Version 4 Hex                         
*/

const char SPARESENSE_CLIENTID[] =    "Spare Flow Client" ;
const char SPARESENSE_TOPICID[] =  "mwp/data/sensor/S002D/flow/spare";
#define SPARESENSE_LEN 10

union   SPARESENSE_  {
   int     data_payload[SPARESENSE_LEN] ;

   struct  {
      int   pulse_count    ;
      int   millisecnods    ;
      int   new_data_flag    ;
      int   adc_sensor    ;
      int   gpio_sensor    ;
      int   temp    ;
      int   temp_w1    ;
      int   temp_w2    ;
      int   cycle_count    ;
      int   fw_version    ;
   }  spare  ;
}  ;
union  SPARESENSE_  spareSense_  ;

char* sparesense_ClientData_var_name [] = { 
    "spareSense_pulse_count",
    "spareSense_millisecnods",
    "spareSense_new_data_flag",
    "spareSense_adc_sensor",
    "spareSense_gpio_sensor",
    "spareSense_temp",
    "spareSense_temp_w1",
    "spareSense_temp_w2",
    "spareSense_cycle_count",
    "spareSense_fw_version",
}  ;

/*
* Block ID: S005D
* Block Name: wellSens
* Description: well power data
* From: well
* To: well Flow Monitor
* Rate: 1 hz
* MQTT Client ID: Well Flow Client
* MQTT Topic ID: mwp/data/sensor/S002D/flow/well
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            raw_current_sense_well1                Current Sense for Well #1        0        5000        20
*  1        int            raw_current_sense_well2                Current Sense for Well #2        0        10000        2000
*  2        int            raw_current_sense_well3                Current Sense for Well #3        0        1        1
*  3        int            raw_current_sense_irrigation_pump                Current Sense for Irrigation        0        1023        512
*  4        int            spare                Spare        0        0        0
*  5        int            house_tank_pressure_switch_on                GPIO Discrete #        0        1        0
*  6        int            septic_alert_on                GPIO Discrete #        0        1        0
*  7        int            raw_temp_celcius                Temperature (celsius)        0        55        20
*  8        int            cycle_count                 Cycle Counter        0        28800        
*  9        int            fw_version                 FW Version 4 Hex                         
*/

const char WELLSENS_CLIENTID[] =    "Well Flow Client" ;
const char WELLSENS_TOPICID[] =  "mwp/data/sensor/S002D/flow/well";
#define WELLSENS_LEN 10

union   WELLSENS_  {
   int     data_payload[WELLSENS_LEN] ;

   struct  {
      int   raw_current_sense_well1    ;
      int   raw_current_sense_well2    ;
      int   raw_current_sense_well3    ;
      int   raw_current_sense_irrigation_pump    ;
      int   spare    ;
      int   house_tank_pressure_switch_on    ;
      int   septic_alert_on    ;
      int   raw_temp_celcius    ;
      int   cycle_count    ;
      int   fw_version    ;
   }  well  ;
}  ;
union  WELLSENS_  wellSens_  ;

char* wellsens_ClientData_var_name [] = { 
    "wellSens_raw_current_sense_well1",
    "wellSens_raw_current_sense_well2",
    "wellSens_raw_current_sense_well3",
    "wellSens_raw_current_sense_irrigation_pump",
    "wellSens_spare",
    "wellSens_house_tank_pressure_switch_on",
    "wellSens_septic_alert_on",
    "wellSens_raw_temp_celcius",
    "wellSens_cycle_count",
    "wellSens_fw_version",
}  ;

/*
* Block ID: S001C
* Block Name: irrigationCommand
* Description: irrigation Flow Command
* From: Irrigation
* To: irrigation Flow Sensor
* Rate: As-Req
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: mwp/command/sensor/S001C/flow/irrigation
* MSG Length: 2
*  word #        data type            variable                description        min        max        nominal
*  0        int            command                Command Word        0        4        1
*  1        int            command_data_w1                Command Data Word 2        0        8        4
*/

const char IRRIGATIONCOMMAND_CLIENTID[] =    "Irrigation Flow Client" ;
const char IRRIGATIONCOMMAND_TOPICID[] =  "mwp/command/sensor/S001C/flow/irrigation";
#define IRRIGATIONCOMMAND_LEN 2

union   IRRIGATIONCOMMAND_  {
   int     data_payload[IRRIGATIONCOMMAND_LEN] ;

   struct  {
      int   command    ;
      int   command_data_w1    ;
   }  irrigation  ;
}  ;
union  IRRIGATIONCOMMAND_  irrigationCommand_  ;

char* irrigationcommand_ClientData_var_name [] = { 
    "irrigationCommand_command",
    "irrigationCommand_command_data_w1",
}  ;

/*
* Block ID: S001CR
* Block Name: irrigationResponse
* Description: irrigation Flow Response
* From: irrigation Flow Sensor
* To: Irrigation
* Rate: As-Req
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: mwp/command/sensor/S001CR/flow/irrigation
* MSG Length: 2
*  word #        data type            variable                description        min        max        nominal
*  0        int            command_response_w1                Command Response Word        0        4        1
*  1        int            command_response_w2                Command Response Word 1        0        8        4
*/

const char IRRIGATIONRESPONSE_CLIENTID[] =    "Irrigation Flow Client" ;
const char IRRIGATIONRESPONSE_TOPICID[] =  "mwp/command/sensor/S001CR/flow/irrigation";
#define IRRIGATIONRESPONSE_LEN 2

union   IRRIGATIONRESPONSE_  {
   int     data_payload[IRRIGATIONRESPONSE_LEN] ;

   struct  {
      int   command_response_w1    ;
      int   command_response_w2    ;
   }  irrigation ;
}  ;
union  IRRIGATIONRESPONSE_  irrigationResponse_  ;

char* irrigationresponse_ClientData_var_name [] = { 
    "irrigationResponse_command_response_w1",
    "irrigationResponse_command_response_w2",
}  ;

/*
* Block ID: I001D
* Block Name: irrigationMon
* Description: Irrigation Monitor
* From: Irrigation
* To: yyy
* Rate: 1 hz
* MQTT Client ID: Irrigation Monitor Client
* MQTT Topic ID: mwp/data/monitor/I001D
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        float            FlowPerMin                Gallons Per Minute        0        50        10
*  1        float            TotalFlow                Total Gallons (24 Hrs)        0        5000        2000
*  2        float            Pressure                Irrigation Pressure        0        100        50
*  3        float            PumpTemp                Pump Temperature        -5        125        80
*  4        float            activeController                Active Controller        1        2        1
*  5        float            activeZone                Active Zone        1        22        1
*  6        float            spare1                spare1        0        0        0
*  7        float            spare2                spare2        0        0        0
*  8        float            cycle_count                 Cycle Counter        0        28800        
*  9        float            fw_version                 FW Version 4 Hex                         
*/

const char IRRIGATIONMON_CLIENTID[] =    "Irrigation Monitor Client" ;
const char IRRIGATIONMON_TOPICID[] =  "mwp/data/monitor/I001D";
#define IRRIGATIONMON_LEN 10

union   IRRIGATIONMON_  {
   float     data_payload[IRRIGATIONMON_LEN] ;

   struct  {
      float   FlowPerMin    ;
      float   TotalFlow    ;
      float   Pressure    ;
      float   PumpTemp    ;
      float   activeController    ;
      float   activeZone    ;
      float   spare1    ;
      float   spare2    ;
      float   cycle_count    ;
      float   fw_version    ;
   }  irrigation  ;
}  ;
union  IRRIGATIONMON_  irrigationMon_  ;

char* irrigationmon_ClientData_var_name [] = { 
    "irrigationMon_FlowPerMin",
    "irrigationMon_TotalFlow",
    "irrigationMon_Pressure",
    "irrigationMon_PumpTemp",
    "irrigationMon_activeController",
    "irrigationMon_activeZone",
    "irrigationMon_spare1",
    "irrigationMon_spare2",
    "irrigationMon_cycle_count",
    "irrigationMon_fw_version",
}  ;

/*
* Block ID: T001D
* Block Name: tankMon
* Description: Tank Monitor
* From: tank
* To: blynk
* Rate: 1 hz
* MQTT Client ID: Tank Monitor Client
* MQTT Topic ID: mwp/data/monitor/T001D
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        float            tank_gallons_per_minute                Tank Gallons Per Minute                        
*  1        float            water_height                Water Height                        
*  2        float            tank_gallons                Tank Gallons                        
*  3        float            tank_per_full                Tank Percent Full                        
*  4        float            tank_total_gallons_24                Tank Total Gallons (24hrs)                        
*  5        float            air_temp                Air Temperature                        
*  6        float            float1                Overfill Float                        
*  7        float            float2                Tank Low Float                        
*  8        float            cycle_count                Cycle Count        0        28800        
*  9        float            fw_version                 FW Version 4 Hex                         
*/

const char TANKMON_CLIENTID[] =    "Tank Monitor Client" ;
const char TANKMON_TOPICID[] =  "mwp/data/monitor/T001D";
#define TANKMON_LEN 10

union   TANKMON_  {
   float     data_payload[TANKMON_LEN] ;

   struct  {
      float   tank_gallons_per_minute    ;
      float   water_height    ;
      float   tank_gallons    ;
      float   tank_per_full    ;
      float   tank_total_gallons_24    ;
      float   air_temp    ;
      float   float1    ;
      float   float2    ;
      float   cycle_count    ;
      float   fw_version    ;
   }  tank  ;
}  ;
union  TANKMON_  tankMon_  ;

char* tankmon_ClientData_var_name [] = { 
    "tankMon_tank_gallons_per_minute",
    "tankMon_water_height",
    "tankMon_tank_gallons",
    "tankMon_tank_per_full",
    "tankMon_tank_total_gallons_24",
    "tankMon_air_temp",
    "tankMon_float1",
    "tankMon_float2",
    "tankMon_cycle_count",
    "tankMon_fw_version",
}  ;

/*
* Block ID: H001D
* Block Name: houseMon
* Description: House Monitor
* From: house
* To: blynk
* Rate: 1 hz
* MQTT Client ID: House Monitor Client
* MQTT Topic ID: mwp/data/monitor/H001D
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        float            tank_gallons_per_minute                Tank Gallons Per Minute                        
*  1        float            water_height                Water Height                        
*  2        float            tank_gallons                Tank Gallons                        
*  3        float            tank_per_full                Tank Percent Full                        
*  4        float            tank_total_gallons_24                Tank Total Gallons (24hrs)                        
*  5        float            air_temp                Air Temperature                        
*  6        float            float1                Overfill Float                        
*  7        float            float2                Tank Low Float                        
*  8        float            cycle_count                Cycle Count        0        28800        
*  9        float            fw_version                 FW Version 4 Hex                         
*/

const char HOUSEMON_CLIENTID[] =    "House Monitor Client" ;
const char HOUSEMON_TOPICID[] =  "mwp/data/monitor/H001D";
#define HOUSEMON_LEN 10

union   HOUSEMON_  {
   float     data_payload[HOUSEMON_LEN] ;

   struct  {
      float   tank_gallons_per_minute    ;
      float   water_height    ;
      float   tank_gallons    ;
      float   tank_per_full    ;
      float   tank_total_gallons_24    ;
      float   air_temp    ;
      float   float1    ;
      float   float2    ;
      float   cycle_count    ;
      float   fw_version    ;
   }  house  ;
}  ;
union  HOUSEMON_  houseMon_  ;

char* housemon_ClientData_var_name [] = { 
    "houseMon_tank_gallons_per_minute",
    "houseMon_water_height",
    "houseMon_tank_gallons",
    "houseMon_tank_per_full",
    "houseMon_tank_total_gallons_24",
    "houseMon_air_temp",
    "houseMon_float1",
    "houseMon_float2",
    "houseMon_cycle_count",
    "houseMon_fw_version",
}  ;

/*
* Block ID: W001D
* Block Name: wellMon
* Description: Well Monitor
* From: well
* To: blynk
* Rate: 1 hz
* MQTT Client ID: Well Monitor Client
* MQTT Topic ID: mwp/data/monitor/W001D
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        float            well_pump_1_on                Well Pump 1 On                        
*  1        float            well_pump_2_on                Well Pump 2 On                        
*  2        float            well_pump_3_on                Well Pump 3 On                        
*  3        float            irrigation_pump_on                Irrigation Pump On                        
*  4        float            house_water_pressure                House Water Pressure                        
*  5        float            system_temp                System Temp                        
*  6        float            House_tank_pressure_switch_on                House Tank Pressure Switch On                        
*  7        float            septic_alert_on                Septic Alert On                        
*  8        float            cycle_count                Cycle Count        0        28800        
*  9        float            fw_version                 FW Version 4 Hex                         
*/

const char WELLMON_CLIENTID[] =    "Well Monitor Client" ;
const char WELLMON_TOPICID[] =  "mwp/data/monitor/W001D";
#define WELLMON_LEN 10

union   WELLMON_  {
   float     data_payload[WELLMON_LEN] ;

   struct  {
      float   well_pump_1_on    ;
      float   well_pump_2_on    ;
      float   well_pump_3_on    ;
      float   irrigation_pump_on    ;
      float   house_water_pressure    ;
      float   system_temp    ;
      float   House_tank_pressure_switch_on    ;
      float   septic_alert_on    ;
      float   cycle_count    ;
      float   fw_version    ;
   }  well  ;
}  ;
union  WELLMON_  wellMon_  ;

char* wellmon_ClientData_var_name [] = { 
    "wellMon_well_pump_1_on",
    "wellMon_well_pump_2_on",
    "wellMon_well_pump_3_on",
    "wellMon_irrigation_pump_on",
    "wellMon_house_water_pressure",
    "wellMon_system_temp",
    "wellMon_House_tank_pressure_switch_on",
    "wellMon_septic_alert_on",
    "wellMon_cycle_count",
    "wellMon_fw_version",
}  ;

/*
* Block ID: M001D
* Block Name: Monitor
* Description: Monitor Data for Blynk
* From: monitor
* To: Blynk
* Rate: 1 hz
* MQTT Client ID: Monitor Client
* MQTT Topic ID: mwp/data/monitor/M001D
* MSG Length: 20
*  word #        data type            variable                description        min        max        nominal
*  0        int            pump_current_sense_1                 PumpCurrentSense[1];                        
*  1        int            pump_current_sense_2                 PumpCurrentSense[2];                        
*  2        int            pump_current_sense_3                 PumpCurrentSense[3];                        
*  3        int            pump_current_sense_4                 PumpCurrentSense[4];                        
*  4        int            pump_led_color_1                 PumpLedColor[1];                        
*  5        int            pump_led_color_2                 PumpLedColor[2];                        
*  6        int            pump_led_color_3                 PumpLedColor[3];                        
*  7        int            pump_led_color_4                 PumpLedColor[4];                        
*  8        int            pump_run_count                 PumpRunCount;  //byte4-pump4;byte3-pump3;byte2-pump2;byte1-pump1        0        28800        
*  9        int            pump_run_time_1                PumpRunTime{1] ; //Seconds                        
*  10        int            pump_run_time_2                PumpRunTime{2] ; //Seconds                        
*  11        int            pump_run_time_3                PumpRunTime{3] ; //Seconds                        
*  12        int            pump_run_time_4                PumpRunTime{4] ; //Seconds                        
*  13        int            float_state_43                 43floatState;  //byte34-float4;byte123-float3                        
*  14        int            float_state_21                 21floatState;  //bytes34-float2;byte12-float1                        
*  15        int            all_float_led_colors                 AllfloatLedcolor;  //byte4-color4;byte3-color3;byte2-color2;byte1-color1                        
*  16        int            septic_relay_alert                Septic Relay Alert                        
*  17        int            septic_relay_alert_color                Septic Relay Alert Color                        
*  18        int            press_relay_sense                Pressure Relay Sense                        
*  19        int            press_led_color                Pressure LED Color                        
*/

const char MONITOR_CLIENTID[] =    "Monitor Client" ;
const char MONITOR_TOPICID[] =  "mwp/data/monitor/M001D";
#define MONITOR_LEN 20

union   MONITOR_  {
   int     data_payload[MONITOR_LEN] ;

   struct  {
      int   pump_current_sense_1    ;
      int   pump_current_sense_2    ;
      int   pump_current_sense_3    ;
      int   pump_current_sense_4    ;
      int   pump_led_color_1    ;
      int   pump_led_color_2    ;
      int   pump_led_color_3    ;
      int   pump_led_color_4    ;
      int   pump_run_count    ;
      int   pump_run_time_1    ;
      int   pump_run_time_2    ;
      int   pump_run_time_3    ;
      int   pump_run_time_4    ;
      int   float_state_43    ;
      int   float_state_21    ;
      int   all_float_led_colors    ;
      int   septic_relay_alert    ;
      int   septic_relay_alert_color    ;
      int   press_relay_sense    ;
      int   press_led_color    ;
   }  monitor  ;
}  ;
union  MONITOR_  monitor_  ;

char* monitor_ClientData_var_name [] = { 
    "Monitor_pump_current_sense_1",
    "Monitor_pump_current_sense_2",
    "Monitor_pump_current_sense_3",
    "Monitor_pump_current_sense_4",
    "Monitor_pump_led_color_1",
    "Monitor_pump_led_color_2",
    "Monitor_pump_led_color_3",
    "Monitor_pump_led_color_4",
    "Monitor_pump_run_count",
    "Monitor_pump_run_time_1",
    "Monitor_pump_run_time_2",
    "Monitor_pump_run_time_3",
    "Monitor_pump_run_time_4",
    "Monitor_float_state_43",
    "Monitor_float_state_21",
    "Monitor_all_float_led_colors",
    "Monitor_septic_relay_alert",
    "Monitor_septic_relay_alert_color",
    "Monitor_press_relay_sense",
    "Monitor_press_led_color",
}  ;

/*
* Block ID: A001D
* Block Name: Alert
* Description: Alert Data for Blynk
* From: alert
* To: Blynk
* Rate: 1 hz
* MQTT Client ID: Alert Client
* MQTT Topic ID: mwp/data/monitor/A001D
* MSG Length: 20
*  word #        data type            variable                description        min        max        nominal
*  0        int            alert1                Alert 1                        
*  1        int            alert2                Alert 2                        
*  2        int            alert3                Alert 3                        
*  3        int            alert4                Alert 4                        
*  4        int            alert5                Alert 5                        
*  5        int            alert6                Alert 6                        
*  6        int            alert7                Alert 7                        
*  7        int            alert8                Alert 8                        
*  8        int            alert9                Alert 9        0        28800        
*  9        int            alert10                Alert 10                        
*  10        int            alert11                Alert 11                        
*  11        int            alert12                Alert 12                        
*  12        int            alert13                Alert 13                        
*  13        int            alert14                Alert 14                        
*  14        int            alert15                Alert 15                        
*  15        int            alert16                Alert 16                        
*  16        int            alert17                Alert 17                        
*  17        int            alert18                Alert 18                        
*  18        int            alert19                Alert 19                        
*  19        int            alert20                Alert 20                        
*/

const char ALERT_CLIENTID[] =    "Alert Client" ;
const char ALERT_TOPICID[] =  "mwp/data/monitor/A001D";
#define ALERT_LEN 20

union   ALERT_  {
   int     data_payload[ALERT_LEN] ;

   struct  {
      int   alert1    ;
      int   alert2    ;
      int   alert3    ;
      int   alert4    ;
      int   alert5    ;
      int   alert6    ;
      int   alert7    ;
      int   alert8    ;
      int   alert9    ;
      int   alert10    ;
      int   alert11    ;
      int   alert12    ;
      int   alert13    ;
      int   alert14    ;
      int   alert15    ;
      int   alert16    ;
      int   alert17    ;
      int   alert18    ;
      int   alert19    ;
      int   alert20    ;
   }  alert  ;
}  ;
union  ALERT_  alert_  ;

char* alert_ClientData_var_name [] = { 
    "Alert_alert1",
    "Alert_alert2",
    "Alert_alert3",
    "Alert_alert4",
    "Alert_alert5",
    "Alert_alert6",
    "Alert_alert7",
    "Alert_alert8",
    "Alert_alert9",
    "Alert_alert10",
    "Alert_alert11",
    "Alert_alert12",
    "Alert_alert13",
    "Alert_alert14",
    "Alert_alert15",
    "Alert_alert16",
    "Alert_alert17",
    "Alert_alert18",
    "Alert_alert19",
    "Alert_alert20",
}  ;
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


struct flowSensorConfigTable
{
   int  sensorID;         // Sensor ID
   char sensorName[40];    // Sensor Name  
   char clientid[40] ;     // Client ID
   char messageid[40];     // Message ID
   int  messagelen ;       // Message Length
};
/*
struct flowSensorConfigTable flowSensorConfig[4] = {
    {0, "SPARE",         SPARE_CLIENTID, SPARE_TOPICID,           SPARE_LEN},
    {1, "TANK",          TANK_CLIENTID,     TANK_TOPICID,         TANK_LEN},
    {2, "IRRIGATION",    IRRIGATION_CLIENTID, IRRIGATION_TOPICID, IRRIGATION_LEN},
    {3, "HOUSE",         HOUSE_CLIENTID, HOUSE_TOPICID,           HOUSE_LEN}
};
*/