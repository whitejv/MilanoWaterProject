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

int firmware = 0;
int SubFirmware = 0x80FF;

/*************************************************
 ************************************************* 
 * IOT Micro-Controller Input Devices Start Here *
 ************************************************* 
 *************************************************/
/*****************************************************
 *** Generic Sensor to Support 8 Flow Sensors      ***
 ***                                               ***
 *** GPIO00 - //Good - Config 3
 *** GPIO01 - stops operation of board
 *** GPIO02 - //Good also lights LED - Temp Sensor
 *** GPIO03 - //Good - Config 2 (Also TX Pin If grounded board won’t load)
 *** GPIO04 - //Good - Disc Input 1
 *** GPIO05 - //Good - Disc Input 2
 *** GPIO06 - doesn’t exist
 *** GPIO12 - //Good - Config 1
 *** GPIO13 - //Good - Flow Sensor
 *** GPIO14 - //reserved SDA
 *** GPIO15 - //reserved SCL
 *** GPIO16 - //Good - Disc 3 (no pull-up)
******************************************************/

#define FLOWSENSOR  13
#define TEMPSENSOR  2
#define CONFIGPIN1  12 //GPIO 12   Input with Pullup
#define CONFIGPIN2  3  //GPIO 3 Input with Pullup
#define CONFIGPIN3  0  //GPIO 0 Input with Pullup
#define DISCINPUT1  4  //GPIO 4 Input with Pullup
#define DISCINPUT2  5  //GPIO 5 Input with Pullup


struct flowSensorConfigTable
{
   int  sensorID;        // Sensor ID
   const char  *sensorName;     // Sensor Name  
   const char  *clientid ;      // Client ID
   const char  *messageid;      // Message ID
   const char  *jsonid;         // JSON ID
   int         messagelen ;     // Message Length
};

int	flow_data_payload[10] ; //must match messagelen below

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
* Block ID: X001D
* Block Name: genericSens
* Description: Generic Sensor Data
* From: generic
* To: xxx
* Type: data
* MQTT Client ID: Generic Flow Client
* MQTT Topic ID: /sensor/X001D/flow/generic
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            milliseconds                Number of milliseconds in Time Window        0        10000        2000
*  2        int            new_data_flag                Flag 1=new data 0=stale data        0        1        1
*  3        int            adc_sensor                ADC Raw Sensor value: Bit 0-9 (0-1023)        0        1023        512
*  4        int            gpio_sensor                GPIO Sensor Data: Bit 1: GPIO 4, Bit 2: GPIO 5        0        3        3
*  5        int            temp                Temp f (int)        -32        150        80
*  6        int            temp_w1                Temperature in F Float Bytes 1&2                        
*  7        int            temp_w2                Temperature in F Float Bytes 3&4                        
*  8        int            cycle_count                 Cycle Counter        0        28800        
*  9        int            fw_version                 FW Version 4 Hex                         
*  10        int            adc_x1                extended sensor adc - 1                        
*  11        int            adc_x2                extended sensor adc - 2                        
*  12        int            adc_x3                extended sensor adc - 3                        
*  13        int            adc_x4                extended sensor adc - 4                        
*  14        int            adc_x5                extended sensor adc - 5                        
*  15        int            adc_x6                extended sensor adc - 6                        
*  16        int            adc_x7                extended sensor adc - 7                        
*  17        int            adc_x8                extended sensor adc - 8                        
*  18        int            GPIO_x1                estended sensor GPIO - 1                        
*  19        int            GPIO_x2                estended sensor GPIO - 2                        
*/

const char GENERICSENS_CLIENTID[] =    "Generic Flow Client" ;
const char GENERICSENS_TOPICID[] =  "mwp/data/sensor/X001D/flow/generic";
const char GENERICSENS_JSONID[] =  "mwp/json/data/sensor/X001D/flow/generic";
#define GENERICSENS_LEN 10

union   GENERICSENS_  {
   int     data_payload[GENERICSENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   milliseconds    ;
      int   new_data_flag    ;
      int   adc_sensor    ;
      int   gpio_sensor    ;
      int   temp    ;
      int   temp_w1    ;
      int   temp_w2    ;
      int   cycle_count    ;
      int   fw_version    ;
      int   adc_x1    ;
      int   adc_x2    ;
      int   adc_x3    ;
      int   adc_x4    ;
      int   adc_x5    ;
      int   adc_x6    ;
      int   adc_x7    ;
      int   adc_x8    ;
      int   GPIO_x1    ;
      int   GPIO_x2    ;
   }  generic  ;
}  ;
union  GENERICSENS_  genericSens_  ;

char* genericsens_ClientData_var_name [] = { 
    "X001D:pulse_count",
    "X001D:milliseconds",
    "X001D:new_data_flag",
    "X001D:adc_sensor",
    "X001D:gpio_sensor",
    "X001D:temp",
    "X001D:temp_w1",
    "X001D:temp_w2",
    "X001D:cycle_count",
    "X001D:fw_version",
    "X001D:adc_x1",
    "X001D:adc_x2",
    "X001D:adc_x3",
    "X001D:adc_x4",
    "X001D:adc_x5",
    "X001D:adc_x6",
    "X001D:adc_x7",
    "X001D:adc_x8",
    "X001D:GPIO_x1",
    "X001D:GPIO_x2",
}  ;

/*
* Block ID: S001D
* Block Name: irrigationSens
* Description: irrigation Flow Data
* From: irrigation
* To: irrigation Flow Monitor
* Type: data
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: /sensor/S001D/flow/irrigation
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            milliseconds                Number of milliseconds in Time Window        0        10000        2000
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
const char IRRIGATIONSENS_JSONID[] =  "mwp/json/data/sensor/S001D/flow/irrigation";
#define IRRIGATIONSENS_LEN 10

union   IRRIGATIONSENS_  {
   int     data_payload[IRRIGATIONSENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   milliseconds    ;
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
    "S001D:pulse_count",
    "S001D:milliseconds",
    "S001D:new_data_flag",
    "S001D:adc_sensor",
    "S001D:gpio_sensor",
    "S001D:temp",
    "S001D:temp_w1",
    "S001D:temp_w2",
    "S001D:cycle_count",
    "S001D:fw_version",
}  ;

/*
* Block ID: S002D
* Block Name: tankSens
* Description: tank Flow Data
* From: tank
* To: tank Flow Monitor
* Type: data
* MQTT Client ID: Tank Flow Client
* MQTT Topic ID: /sensor/S002D/flow/tank
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            milliseconds                Number of milliseconds in Time Window        0        10000        2000
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
const char TANKSENS_JSONID[] =  "mwp/json/data/sensor/S002D/flow/tank";
#define TANKSENS_LEN 10

union   TANKSENS_  {
   int     data_payload[TANKSENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   milliseconds    ;
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
    "S002D:pulse_count",
    "S002D:milliseconds",
    "S002D:new_data_flag",
    "S002D:adc_sensor",
    "S002D:gpio_sensor",
    "S002D:temp",
    "S002D:temp_w1",
    "S002D:temp_w2",
    "S002D:cycle_count",
    "S002D:fw_version",
}  ;

/*
* Block ID: S003D
* Block Name: houseSens
* Description: house Flow Data
* From: house
* To: house Flow Monitor
* Type: data
* MQTT Client ID: House Flow Client
* MQTT Topic ID: /sensor/S002D/flow/house
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            milliseconds                Number of milliseconds in Time Window        0        10000        2000
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
const char HOUSESENS_JSONID[] =  "mwp/json/data/sensor/S002D/flow/house";
#define HOUSESENS_LEN 10

union   HOUSESENS_  {
   int     data_payload[HOUSESENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   milliseconds    ;
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
    "S003D:pulse_count",
    "S003D:milliseconds",
    "S003D:new_data_flag",
    "S003D:adc_sensor",
    "S003D:gpio_sensor",
    "S003D:temp",
    "S003D:temp_w1",
    "S003D:temp_w2",
    "S003D:cycle_count",
    "S003D:fw_version",
}  ;

/*
* Block ID: S004D
* Block Name: spareSens
* Description: spare Flow Data
* From: spare
* To: spare Flow Monitor
* Type: data
* MQTT Client ID: Spare Flow Client
* MQTT Topic ID: /sensor/S002D/flow/spare
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pulse_count                Pulses Counted in Time Window        0        5000        20
*  1        int            milliseconds                Number of milliseconds in Time Window        0        10000        2000
*  2        int            new_data_flag                Flag 1=new data 0=stale data        0        1        1
*  3        int            adc_sensor                ADC Raw Sensor value: Bit 0-9 (0-1023)        0        1023        512
*  4        int            gpio_sensor                GPIO Sensor Data: Bit 1: GPIO 4, Bit 2: GPIO 5        0        3        3
*  5        int            temp                Temp f (int)        -32        150        80
*  6        int            temp_w1                Temperature in F Float Bytes 1&2                        
*  7        int            temp_w2                Temperature in F Float Bytes 3&4                        
*  8        int            cycle_count                 Cycle Counter        0        28800        
*  9        int            fw_version                 FW Version 4 Hex                         
*/

const char SPARESENS_CLIENTID[] =    "Spare Flow Client" ;
const char SPARESENS_TOPICID[] =  "mwp/data/sensor/S002D/flow/spare";
const char SPARESENS_JSONID[] =  "mwp/json/data/sensor/S002D/flow/spare";
#define SPARESENS_LEN 10

union   SPARESENS_  {
   int     data_payload[SPARESENS_LEN] ;

   struct  {
      int   pulse_count    ;
      int   milliseconds    ;
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
union  SPARESENS_  spareSens_  ;

char* sparesens_ClientData_var_name [] = { 
    "S004D:pulse_count",
    "S004D:milliseconds",
    "S004D:new_data_flag",
    "S004D:adc_sensor",
    "S004D:gpio_sensor",
    "S004D:temp",
    "S004D:temp_w1",
    "S004D:temp_w2",
    "S004D:cycle_count",
    "S004D:fw_version",
}  ;

/*
* Block ID: S005D
* Block Name: wellSens
* Description: well power data
* From: well
* To: well Flow Monitor
* Type: data
* MQTT Client ID: Well Flow Client
* MQTT Topic ID: /sensor/S002D/flow/well
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
const char WELLSENS_JSONID[] =  "mwp/json/data/sensor/S002D/flow/well";
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
    "S005D:raw_current_sense_well1",
    "S005D:raw_current_sense_well2",
    "S005D:raw_current_sense_well3",
    "S005D:raw_current_sense_irrigation_pump",
    "S005D:spare",
    "S005D:house_tank_pressure_switch_on",
    "S005D:septic_alert_on",
    "S005D:raw_temp_celcius",
    "S005D:cycle_count",
    "S005D:fw_version",
}  ;

/*
* Block ID: S001C
* Block Name: irrigationCommand
* Description: irrigation Flow Command
* From: Irrigation
* To: irrigation Flow Sensor
* Type: command
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: /sensor/S001C/flow/irrigation
* MSG Length: 2
*  word #        data type            variable                description        min        max        nominal
*  0        int            command                Command Word        0        4        1
*  1        int            command_data_w1                Command Data Word 2        0        8        4
*/

const char IRRIGATIONCOMMAND_CLIENTID[] =    "Irrigation Flow Client" ;
const char IRRIGATIONCOMMAND_TOPICID[] =  "mwp/command/sensor/S001C/flow/irrigation";
const char IRRIGATIONCOMMAND_JSONID[] =  "mwp/json/command/sensor/S001C/flow/irrigation";
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
    "S001C:command",
    "S001C:command_data_w1",
}  ;

/*
* Block ID: S001CR
* Block Name: irrigationResponse
* Description: irrigation Flow Response
* From: Irrigation
* To: Irrigation
* Type: response
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: /sensor/S001CR/flow/irrigation
* MSG Length: 2
*  word #        data type            variable                description        min        max        nominal
*  0        int            command_response_w1                Command Response Word        0        4        1
*  1        int            command_response_w2                Command Response Word 1        0        8        4
*/

const char IRRIGATIONRESPONSE_CLIENTID[] =    "Irrigation Flow Client" ;
const char IRRIGATIONRESPONSE_TOPICID[] =  "mwp/response/sensor/S001CR/flow/irrigation";
const char IRRIGATIONRESPONSE_JSONID[] =  "mwp/json/response/sensor/S001CR/flow/irrigation";
#define IRRIGATIONRESPONSE_LEN 2

union   IRRIGATIONRESPONSE_  {
   int     data_payload[IRRIGATIONRESPONSE_LEN] ;

   struct  {
      int   command_response_w1    ;
      int   command_response_w2    ;
   }  irrigation  ;
}  ;
union  IRRIGATIONRESPONSE_  irrigationResponse_  ;

char* irrigationresponse_ClientData_var_name [] = { 
    "S001CR:command_response_w1",
    "S001CR:command_response_w2",
}  ;

/*
* Block ID: I001D
* Block Name: irrigationMon
* Description: Irrigation Monitor
* From: Irrigation
* To: yyy
* Type: data
* MQTT Client ID: Irrigation Monitor Client
* MQTT Topic ID: /monitor/I001D
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        float            FlowPerMin                Gallons Per Minute        0        50        10
*  1        float            TotalFlow                Total Gallons (24 Hrs)        0        5000        2000
*  2        float            Pressure                Irrigation Pressure        0        100        50
*  3        float            PumpTemp                Pump Temperature        -5        125        80
*  4        float            FrontControllerActive                Front Controller Active        0        1        1
*  5        float            FrontActiveZone                Front Active Zone        1        22        1
*  6        float            BackControllerActive                Back Controller Active        0        1        1
*  7        float            BackActiveZone                Back Active Zone        1        22        1
*  8        float            cycle_count                 Cycle Counter        0        28800        
*  9        float            fw_version                 FW Version 4 Hex                         
*/

const char IRRIGATIONMON_CLIENTID[] =    "Irrigation Monitor Client" ;
const char IRRIGATIONMON_TOPICID[] =  "mwp/data/monitor/I001D";
const char IRRIGATIONMON_JSONID[] =  "mwp/json/data/monitor/I001D";
#define IRRIGATIONMON_LEN 10

union   IRRIGATIONMON_  {
   float     data_payload[IRRIGATIONMON_LEN] ;

   struct  {
      float   FlowPerMin    ;
      float   TotalFlow    ;
      float   Pressure    ;
      float   PumpTemp    ;
      float   FrontControllerActive    ;
      float   FrontActiveZone    ;
      float   BackControllerActive    ;
      float   BackActiveZone    ;
      float   cycle_count    ;
      float   fw_version    ;
   }  irrigation  ;
}  ;
union  IRRIGATIONMON_  irrigationMon_  ;

char* irrigationmon_ClientData_var_name [] = { 
    "I001D:FlowPerMin",
    "I001D:TotalFlow",
    "I001D:Pressure",
    "I001D:PumpTemp",
    "I001D:FrontControllerActive",
    "I001D:FrontActiveZone",
    "I001D:BackControllerActive",
    "I001D:BackActiveZone",
    "I001D:cycle_count",
    "I001D:fw_version",
}  ;

/*
* Block ID: R001C
* Block Name: rainbirdCommand
* Description: Rainbird Command Block
* From: rainbirdPy
* To: xxx
* Type: command
* MQTT Client ID: RainbirdPy
* MQTT Topic ID: /rainbird/command
* MSG Length: 7
*  word #        data type            variable                description        min        max        nominal
*  0        char            command[7]                Command Word                        
*/

const char RAINBIRDCOMMAND_CLIENTID[] =    "RainbirdPy" ;
const char RAINBIRDCOMMAND_TOPICID[] =  "mwp/command/rainbird/command";
const char RAINBIRDCOMMAND_JSONID[] =  "mwp/json/command/rainbird/command";
#define RAINBIRDCOMMAND_LEN 7

union   RAINBIRDCOMMAND_  {
   char     data_payload[RAINBIRDCOMMAND_LEN] ;

   struct  {
      char   command[7]    ;
   }  rainbirdpy  ;
}  ;
union  RAINBIRDCOMMAND_  rainbirdCommand_  ;

char* rainbirdcommand_ClientData_var_name [] = { 
    "R001C:command[7]",
}  ;

/*
* Block ID: R001R
* Block Name: rainbirdResponse
* Description: Rainbird Response 
* From: rainbirdPy
* To: Irrigation
* Type: response
* MQTT Client ID: RainbirdPy
* MQTT Topic ID: /rainbird/+/active_zone
* MSG Length: 255
*  word #        data type            variable                description        min        max        nominal
*  0        char            command_response[255]                Command Response Word                        
*/

const char RAINBIRDRESPONSE_CLIENTID[] =    "RainbirdPy" ;
const char RAINBIRDRESPONSE_TOPICID[] =  "mwp/response/rainbird/+/active_zone";
const char RAINBIRDRESPONSE_JSONID[] =  "mwp/json/response/rainbird/+/active_zone";
#define RAINBIRDRESPONSE_LEN 255

union   RAINBIRDRESPONSE_  {
   char     data_payload[RAINBIRDRESPONSE_LEN] ;

   struct  {
      char   command_response[255]    ;
   }  rainbirdpy  ;
}  ;
union  RAINBIRDRESPONSE_  rainbirdResponse_  ;

char* rainbirdresponse_ClientData_var_name [] = { 
    "R001R:command_response[255]",
}  ;

/*
* Block ID: T001D
* Block Name: tankMon
* Description: Tank Monitor
* From: tank
* To: blynk
* Type: data
* MQTT Client ID: Tank Monitor Client
* MQTT Topic ID: /monitor/T001D
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
const char TANKMON_JSONID[] =  "mwp/json/data/monitor/T001D";
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
    "T001D:tank_gallons_per_minute",
    "T001D:water_height",
    "T001D:tank_gallons",
    "T001D:tank_per_full",
    "T001D:tank_total_gallons_24",
    "T001D:air_temp",
    "T001D:float1",
    "T001D:float2",
    "T001D:cycle_count",
    "T001D:fw_version",
}  ;

/*
* Block ID: H001D
* Block Name: houseMon
* Description: House Monitor
* From: house
* To: blynk
* Type: data
* MQTT Client ID: House Monitor Client
* MQTT Topic ID: /monitor/H001D
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
const char HOUSEMON_JSONID[] =  "mwp/json/data/monitor/H001D";
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
    "H001D:tank_gallons_per_minute",
    "H001D:water_height",
    "H001D:tank_gallons",
    "H001D:tank_per_full",
    "H001D:tank_total_gallons_24",
    "H001D:air_temp",
    "H001D:float1",
    "H001D:float2",
    "H001D:cycle_count",
    "H001D:fw_version",
}  ;

/*
* Block ID: W001D
* Block Name: wellMon
* Description: Well Monitor
* From: well
* To: blynk
* Type: data
* MQTT Client ID: Well Monitor Client
* MQTT Topic ID: /monitor/W001D
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
const char WELLMON_JSONID[] =  "mwp/json/data/monitor/W001D";
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
    "W001D:well_pump_1_on",
    "W001D:well_pump_2_on",
    "W001D:well_pump_3_on",
    "W001D:irrigation_pump_on",
    "W001D:house_water_pressure",
    "W001D:system_temp",
    "W001D:House_tank_pressure_switch_on",
    "W001D:septic_alert_on",
    "W001D:cycle_count",
    "W001D:fw_version",
}  ;

/*
* Block ID: M001D
* Block Name: monitor
* Description: Monitor Data for Blynk
* From: monitor
* To: Blynk
* Type: data
* MQTT Client ID: Monitor Client
* MQTT Topic ID: /monitor/M001D
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
const char MONITOR_JSONID[] =  "mwp/json/data/monitor/M001D";
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
    "M001D:pump_current_sense_1",
    "M001D:pump_current_sense_2",
    "M001D:pump_current_sense_3",
    "M001D:pump_current_sense_4",
    "M001D:pump_led_color_1",
    "M001D:pump_led_color_2",
    "M001D:pump_led_color_3",
    "M001D:pump_led_color_4",
    "M001D:pump_run_count",
    "M001D:pump_run_time_1",
    "M001D:pump_run_time_2",
    "M001D:pump_run_time_3",
    "M001D:pump_run_time_4",
    "M001D:float_state_43",
    "M001D:float_state_21",
    "M001D:all_float_led_colors",
    "M001D:septic_relay_alert",
    "M001D:septic_relay_alert_color",
    "M001D:press_relay_sense",
    "M001D:press_led_color",
}  ;

/*
* Block ID: A001D
* Block Name: alert
* Description: Alert Data for Blynk
* From: alert
* To: Blynk
* Type: data
* MQTT Client ID: Alert Client
* MQTT Topic ID: /monitor/A001D
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
const char ALERT_JSONID[] =  "mwp/json/data/monitor/A001D";
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
    "A001D:alert1",
    "A001D:alert2",
    "A001D:alert3",
    "A001D:alert4",
    "A001D:alert5",
    "A001D:alert6",
    "A001D:alert7",
    "A001D:alert8",
    "A001D:alert9",
    "A001D:alert10",
    "A001D:alert11",
    "A001D:alert12",
    "A001D:alert13",
    "A001D:alert14",
    "A001D:alert15",
    "A001D:alert16",
    "A001D:alert17",
    "A001D:alert18",
    "A001D:alert19",
    "A001D:alert20",
}  ;

//#define MilanoWaterProject\IrrigationHeader.h  (created and maintained in Excel)
/*********************************************************************************
 *********************************************************************************
 **** ^^^           ^^^           ^^^           ^^^          ^^^          ^^^ ****
 **** |||           |||           |||           |||          |||          ||| ****
 **** Data Above This Line is Auto Generated from the Excel Spreadsheet Above ****
 ****                                                                         ****
 *********************************************************************************
 ********************************************************************************/

/*********************************************************************************
 *********************************************************************************
 ******   Sensors 0-3 are standard flow sensors with 10 data words         *******
 ******   Sensors 4-7 are extended sensors with 20 data words              *******
**********************************************************************************/

struct flowSensorConfigTable flowSensorConfig[8] = {
    {0, "SPARE",         SPARESENS_CLIENTID, SPARESENS_TOPICID, SPARESENS_JSONID, SPARESENS_LEN},
    {1, "TANK",          TANKSENS_CLIENTID,     TANKSENS_TOPICID,TANKSENS_JSONID, TANKSENS_LEN},
    {2, "IRRIGATION",    IRRIGATIONSENS_CLIENTID, IRRIGATIONSENS_TOPICID, IRRIGATIONSENS_JSONID,IRRIGATIONSENS_LEN},
    {3, "HOUSE",         HOUSESENS_CLIENTID, HOUSESENS_TOPICID, HOUSESENS_JSONID, HOUSESENS_LEN},
    {4, "WELL",          WELLSENS_CLIENTID, WELLSENS_TOPICID, WELLSENS_JSONID, WELLSENS_LEN},
    {5, "SPARE",         SPARESENS_CLIENTID, SPARESENS_TOPICID, SPARESENS_JSONID, SPARESENS_LEN},
    {6, "SPARE",         SPARESENS_CLIENTID, SPARESENS_TOPICID, SPARESENS_JSONID, SPARESENS_LEN},
    {7, "SPARE",         SPARESENS_CLIENTID, SPARESENS_TOPICID, SPARESENS_JSONID, SPARESENS_LEN},
};
