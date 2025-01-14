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
#define trainingdata "/home/pi/MWPLogData/trainingdata/"

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
char rainbird_command3[] = "STOP";
char rainbird_command4[] = "ADVANCE";

/* Library function prototypes */

void log_message(const char *format, ...);
void log_test(int verbose, int log_level, int msg_level, const char *format, ...);
void parse_message(char* message, Controller* controller) ;
void flowmon(int new_data_flag, int milliseconds, int pulse_count, float *p_avgflowRateGPM, float *p_dailyGallons, float calibrationFactor) ;
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
/*************************************************
 *** Generic Sensor to Support 8 Flow Sensors  ***
 ***                                           ***    
 *** GPIO00 - stops operation of board             
 *** GPIO01 - stops operation of board
 *** GPIO02 - //Good also lights LED - Temp Sensor
 *** GPIO03 - //Good - Config 2 (Also TX Pin If grounded board won’t load)
 *** GPIO04 - //Good - Config 3 //also works as SDA
 *** GPIO05 - //Good - Disc Input 2 //also works as SCL
 *** GPIO06 - doesn’t exist
 *** GPIO12 - //Good - Config 1
 *** GPIO13 - //Good - Flow Sensor
 *** GPIO14 - //reserved SDA - Disc 1 Works as Input w/ pull-up
 *** GPIO15 - //reserved SCL - Works as Input w/o pull-up
 *** GPIO16 - //Good - Disc 3 (no pull-up)
******************************************************/
/*************************************************
 *** Generic Sensor to Support 8 Flow Sensors  ***
 ***            Rev. 2 12/20/24                ***    
 *** GPIO00 - stops operation of board             
 *** GPIO01 - stops operation of board
 *** GPIO02 - //Good - Temp Sensor also lights LED
 *** GPIO03 - //Good - Config 2 - Input w/ pull-up(Also TX Pin If grounded board won’t load)
 *** GPIO04 - //Good - SDA {Disc Input x non-extended boards}
 *** GPIO05 - //Good - SCL {Disc Input 2 non-extended boards}
 *** GPIO06 - doesn’t exist
 *** GPIO12 - //Good - Config 1 - Input w/ pull-up
 *** GPIO13 - //Good - Flow Sensor
 *** GPIO14 - //Good - Config 3 - Input w/ pull-up
 *** GPIO15 - //Good - Disc 1 (no pull-up)
 *** GPIO16 - //Good - Disc 2 (no pull-up)
******************************************************/
/******************************************************
 ******************************************************
 ******           Config Settings                 *****
 ****** ConFigPin   ConFigPin  ConFigPin SensorID *****
 ******     12          03         04             *****
 ******-------------------------------------------*****
 ******      G           G         G        0     *****
 ******      G           O         G        2     *****
 ******      O           G         G        1     *****
 ******      O           O         G        3     *****
 ******      G           G         O        4     *****
 ******      G           O         O        6     *****
 ******      O           G         O        5     *****
 ******      O           O         O        7     *****
 ******************************************************
 *****************************************************/

#define FLOWSENSOR  13
#define TEMPSENSOR  2
#define CONFIGPIN1  12  //GPIO 12   Input with Pullup
#define CONFIGPIN2  3   //GPIO 3    Input with Pullup
#define CONFIGPIN3  4   //GPIO 4    Input with Pullup
#define DISCINPUT2  14  //  Input with Pullup (only non-extended boards)
#define DISCINPUT1  5   //  Input with Pullup



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
* Category: sensor
* Type: data
* MQTT Client ID: Generic Flow Client
* MQTT Topic ID: 
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
const char GENERICSENS_TOPICID[] =  "mwp/data/sensor/generic/X001D";
const char GENERICSENS_JSONID[] =  "mwp/json/data/sensor/generic/X001D";
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
* Category: sensor
* Type: data
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: 
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
const char IRRIGATIONSENS_TOPICID[] =  "mwp/data/sensor/irrigation/S001D";
const char IRRIGATIONSENS_JSONID[] =  "mwp/json/data/sensor/irrigation/S001D";
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
* Category: sensor
* Type: data
* MQTT Client ID: Tank Flow Client
* MQTT Topic ID: 
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
const char TANKSENS_TOPICID[] =  "mwp/data/sensor/tank/S002D";
const char TANKSENS_JSONID[] =  "mwp/json/data/sensor/tank/S002D";
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
* Category: sensor
* Type: data
* MQTT Client ID: House Flow Client
* MQTT Topic ID: 
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
const char HOUSESENS_TOPICID[] =  "mwp/data/sensor/house/S003D";
const char HOUSESENS_JSONID[] =  "mwp/json/data/sensor/house/S003D";
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
* Block Name: well3Sens
* Description: well3 Flow Data
* From: well3
* Category: sensor
* Type: data
* MQTT Client ID: Well3 Flow Client
* MQTT Topic ID: 
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

const char WELL3SENS_CLIENTID[] =    "Well3 Flow Client" ;
const char WELL3SENS_TOPICID[] =  "mwp/data/sensor/well3/S004D";
const char WELL3SENS_JSONID[] =  "mwp/json/data/sensor/well3/S004D";
#define WELL3SENS_LEN 10

union   WELL3SENS_  {
   int     data_payload[WELL3SENS_LEN] ;

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
   }  well3  ;
}  ;
union  WELL3SENS_  well3Sens_  ;

char* well3sens_ClientData_var_name [] = { 
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
* Category: sensor
* Type: data
* MQTT Client ID: Well Flow Client
* MQTT Topic ID: 
* MSG Length: 20
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
*  10        int            adc_x1                Current Sense for Well #1                        
*  11        int            adc_x2                Current Sense for Well #2                        
*  12        int            adc_x3                Current Sense for Well #3                        
*  13        int            adc_x4                Current Sense for Irrigation                        
*  14        int            adc_x5                extended sensor adc - 5                        
*  15        int            adc_x6                extended sensor adc - 6                        
*  16        int            adc_x7                extended sensor adc - 7                        
*  17        int            adc_x8                extended sensor adc - 8                        
*  18        int            GPIO_x1                estended sensor GPIO - 1                        
*  19        int            GPIO_x2                estended sensor GPIO - 2                        
*/

const char WELLSENS_CLIENTID[] =    "Well Flow Client" ;
const char WELLSENS_TOPICID[] =  "mwp/data/sensor/well/S005D";
const char WELLSENS_JSONID[] =  "mwp/json/data/sensor/well/S005D";
#define WELLSENS_LEN 20

union   WELLSENS_  {
   int     data_payload[WELLSENS_LEN] ;

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
   }  well  ;
}  ;
union  WELLSENS_  wellSens_  ;

char* wellsens_ClientData_var_name [] = { 
    "S005D:pulse_count",
    "S005D:milliseconds",
    "S005D:new_data_flag",
    "S005D:adc_sensor",
    "S005D:gpio_sensor",
    "S005D:temp",
    "S005D:temp_w1",
    "S005D:temp_w2",
    "S005D:cycle_count",
    "S005D:fw_version",
    "S005D:adc_x1",
    "S005D:adc_x2",
    "S005D:adc_x3",
    "S005D:adc_x4",
    "S005D:adc_x5",
    "S005D:adc_x6",
    "S005D:adc_x7",
    "S005D:adc_x8",
    "S005D:GPIO_x1",
    "S005D:GPIO_x2",
}  ;

/*
* Block ID: S001C
* Block Name: irrigationCommand
* Description: irrigation Flow Command
* From: Irrigation
* Category: sensor
* Type: command
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: 
* MSG Length: 2
*  word #        data type            variable                description        min        max        nominal
*  0        int            command                Command Word        0        4        1
*  1        int            command_data_w1                Command Data Word 2        0        8        4
*/

const char IRRIGATIONCOMMAND_CLIENTID[] =    "Irrigation Flow Client" ;
const char IRRIGATIONCOMMAND_TOPICID[] =  "mwp/command/sensor/Irrigation/S001C";
const char IRRIGATIONCOMMAND_JSONID[] =  "mwp/json/command/sensor/Irrigation/S001C";
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
* Category: sensor
* Type: response
* MQTT Client ID: Irrigation Flow Client
* MQTT Topic ID: 
* MSG Length: 2
*  word #        data type            variable                description        min        max        nominal
*  0        int            command_response_w1                Command Response Word        0        4        1
*  1        int            command_response_w2                Command Response Word 1        0        8        4
*/

const char IRRIGATIONRESPONSE_CLIENTID[] =    "Irrigation Flow Client" ;
const char IRRIGATIONRESPONSE_TOPICID[] =  "mwp/response/sensor/Irrigation/S001CR";
const char IRRIGATIONRESPONSE_JSONID[] =  "mwp/json/response/sensor/Irrigation/S001CR";
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
* Category: monitor
* Type: data
* MQTT Client ID: Irrigation Monitor Client
* MQTT Topic ID: 
* MSG Length: 16
*  word #        data type            variable                description        min        max        nominal
*  0        float            pressurePSI                Pressure in PSI - Irrigation        0        50        10
*  1        float            temperatureF                Temperature - Irrigation Pump        0        5000        2000
*  2        float            intervalFlow                Gallons in interval        0        100        50
*  3        float            amperage                Amperage        -5        125        80
*  4        float            secondsOn                Second on for Pump        0        14400        60
*  5        float            gallonsMinute                Gallons per Minute        0        1        1
*  6        float            gallonsDay                Irrigation Total Gallons (24hrs)        1        22        1
*  7        float            controller                Controller        0        1        1
*  8        float            zone                Zone        1        22        1
*  9        float            spare1                Spare        0        28800        
*  10        float            spare2                Spare                        
*  11        float            spare3                Spare                        
*  12        float            spare4                Spare                        
*  13        float            spare5                Spare                        
*  14        float            cycleCount                Cycle Count                        
*  15        float            fwVersion                 FW Version 4 Hex                         
*/

const char IRRIGATIONMON_CLIENTID[] =    "Irrigation Monitor Client" ;
const char IRRIGATIONMON_TOPICID[] =  "mwp/data/monitor/Irrigation/I001D";
const char IRRIGATIONMON_JSONID[] =  "mwp/json/data/monitor/Irrigation/I001D";
#define IRRIGATIONMON_LEN 16

union   IRRIGATIONMON_  {
   float     data_payload[IRRIGATIONMON_LEN] ;

   struct  {
      float   pressurePSI    ;
      float   temperatureF    ;
      float   intervalFlow    ;
      float   amperage    ;
      float   secondsOn    ;
      float   gallonsMinute    ;
      float   gallonsDay    ;
      float   controller    ;
      float   zone    ;
      float   spare1    ;
      float   spare2    ;
      float   spare3    ;
      float   spare4    ;
      float   spare5    ;
      float   cycleCount    ;
      float   fwVersion    ;
   }  irrigation  ;
}  ;
union  IRRIGATIONMON_  irrigationMon_  ;

char* irrigationmon_ClientData_var_name [] = { 
    "I001D:pressurePSI",
    "I001D:temperatureF",
    "I001D:intervalFlow",
    "I001D:amperage",
    "I001D:secondsOn",
    "I001D:gallonsMinute",
    "I001D:gallonsDay",
    "I001D:controller",
    "I001D:zone",
    "I001D:spare1",
    "I001D:spare2",
    "I001D:spare3",
    "I001D:spare4",
    "I001D:spare5",
    "I001D:cycleCount",
    "I001D:fwVersion",
}  ;

/*
* Block ID: R001C
* Block Name: rainbirdCommand
* Description: Rainbird Command Block
* From: rainbirdPy
* To: xxx
* Category: sensor
* MQTT Client ID: RainbirdPy
* MQTT Topic ID: command/rainbird/command
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
* Category: sensor
* Type: response
* MQTT Client ID: RainbirdPy
* MQTT Topic ID: response/rainbird/+/active_zone
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
* Category: monitor
* Type: data
* MQTT Client ID: Tank Monitor Client
* MQTT Topic ID: 
* MSG Length: 16
*  word #        data type            variable                description        min        max        nominal
*  0        float            pressurePSI                Pressure in PSI - Atmospheric                        
*  1        float            temperatureF                Temperature - Tank Valve                        
*  2        float            intervalFlow                Gallons in interval -N/A                        
*  3        float            amperage                Amperage -N/A                        
*  4        float            secondsOn                Second on for Pump        0        14400        60
*  5        float            gallonsMinute                Gallons per Minute                        
*  6        float            gallonsDay                Tank Total Gallons (24hrs)                        
*  7        float            controller                Controller                        
*  8        float            zone                Zone                        
*  9        float            water_height                Water Height                        
*  10        float            tank_gallons                 FW Version 4 Hex                         
*  11        float            tank_per_full                Tank Percent Full                        
*  12        float            float1                Overfill Float                        
*  13        float            float2                Tank Low Float                        
*  14        float            cycleCount                Cycle Count                        
*  15        float            fwVersion                 FW Version 4 Hex                         
*/

const char TANKMON_CLIENTID[] =    "Tank Monitor Client" ;
const char TANKMON_TOPICID[] =  "mwp/data/monitor/tank/T001D";
const char TANKMON_JSONID[] =  "mwp/json/data/monitor/tank/T001D";
#define TANKMON_LEN 16

union   TANKMON_  {
   float     data_payload[TANKMON_LEN] ;

   struct  {
      float   pressurePSI    ;
      float   temperatureF    ;
      float   intervalFlow    ;
      float   amperage    ;
      float   secondsOn    ;
      float   gallonsMinute    ;
      float   gallonsDay    ;
      float   controller    ;
      float   zone    ;
      float   water_height    ;
      float   tank_gallons    ;
      float   tank_per_full    ;
      float   float1    ;
      float   float2    ;
      float   cycleCount    ;
      float   fwVersion    ;
   }  tank  ;
}  ;
union  TANKMON_  tankMon_  ;

char* tankmon_ClientData_var_name [] = { 
    "T001D:pressurePSI",
    "T001D:temperatureF",
    "T001D:intervalFlow",
    "T001D:amperage",
    "T001D:secondsOn",
    "T001D:gallonsMinute",
    "T001D:gallonsDay",
    "T001D:controller",
    "T001D:zone",
    "T001D:water_height",
    "T001D:tank_gallons",
    "T001D:tank_per_full",
    "T001D:float1",
    "T001D:float2",
    "T001D:cycleCount",
    "T001D:fwVersion",
}  ;

/*
* Block ID: H001D
* Block Name: houseMon
* Description: House Monitor
* From: house
* Category: monitor
* Type: data
* MQTT Client ID: House Monitor Client
* MQTT Topic ID: 
* MSG Length: 16
*  word #        data type            variable                description        min        max        nominal
*  0        float            pressurePSI                Pressure in PSI - House        0        25        15
*  1        float            temperatureF                Temperature - House Supply        0        5000        100
*  2        float            intervalFlow                Gallons in interval        0        100        65
*  3        float            amperage                Amperage        0        125        80
*  4        float            secondsOn                Second on for Pump        0        14400        60
*  5        float            gallonsMinute                Gallons per Minute                        
*  6        float            gallonsDay                House Total Gallons (24hrs)                        
*  7        float            controller                Controller                        
*  8        float            zone                Zone                        
*  9        float            spare1                Spare 1        0        28800        
*  10        float            spare2                Spare 2                        
*  11        float            spare3                Spare 3                        
*  12        float            spare4                Spare 4                        
*  13        float            spare5                Spare 5                        
*  14        float            cycleCount                Cycle Count                        
*  15        float            fwVersion                 FW Version 4 Hex                         
*/

const char HOUSEMON_CLIENTID[] =    "House Monitor Client" ;
const char HOUSEMON_TOPICID[] =  "mwp/data/monitor/house/H001D";
const char HOUSEMON_JSONID[] =  "mwp/json/data/monitor/house/H001D";
#define HOUSEMON_LEN 16

union   HOUSEMON_  {
   float     data_payload[HOUSEMON_LEN] ;

   struct  {
      float   pressurePSI    ;
      float   temperatureF    ;
      float   intervalFlow    ;
      float   amperage    ;
      float   secondsOn    ;
      float   gallonsMinute    ;
      float   gallonsDay    ;
      float   controller    ;
      float   zone    ;
      float   spare1    ;
      float   spare2    ;
      float   spare3    ;
      float   spare4    ;
      float   spare5    ;
      float   cycleCount    ;
      float   fwVersion    ;
   }  house  ;
}  ;
union  HOUSEMON_  houseMon_  ;

char* housemon_ClientData_var_name [] = { 
    "H001D:pressurePSI",
    "H001D:temperatureF",
    "H001D:intervalFlow",
    "H001D:amperage",
    "H001D:secondsOn",
    "H001D:gallonsMinute",
    "H001D:gallonsDay",
    "H001D:controller",
    "H001D:zone",
    "H001D:spare1",
    "H001D:spare2",
    "H001D:spare3",
    "H001D:spare4",
    "H001D:spare5",
    "H001D:cycleCount",
    "H001D:fwVersion",
}  ;

/*
* Block ID: W001D
* Block Name: wellMon
* Description: Well Monitor
* From: well
* Category: monitor
* Type: data
* MQTT Client ID: Well Monitor Client
* MQTT Topic ID: 
* MSG Length: 18
*  word #        data type            variable                description        min        max        nominal
*  0        float            well_pump_1_on                Well Pump 1 On        0        1        
*  1        float            well_pump_2_on                Well Pump 2 On        0        1        
*  2        float            well_pump_3_on                Well Pump 3 On        0        1        
*  3        float            irrigation_pump_on                Irrigation Pump On        0        1        
*  4        float            house_water_pressure                House Water Pressure        0        100        
*  5        float            system_temp                System Temp        0        150        
*  6        float            House_tank_pressure_switch_on                House Tank Pressure Switch On        0        1        
*  7        float            septic_alert_on                Septic Alert On        0        1        
*  8        float            cycle_count                Cycle Count        0        28800        
*  9        float            fw_version                 FW Version 4 Hex                         
*  10        float            amp_pump_1                amp count pump 1        0        1024        
*  11        float            amp_pump_2                amp count pump 2        0        1024        
*  12        float            amp_pump_3                amp count pump 3        0        1024        
*  13        float            amp_pump_4                amp count pump 4        0        1024        
*  14        float            amp_5                amp count 5        0        1024        
*  15        float            amp_6                amp count 6        0        1024        
*  16        float            amp_7                amp count 7        0        1024        
*  17        float            amp_8                amp count 8        0        1024        
*/

const char WELLMON_CLIENTID[] =    "Well Monitor Client" ;
const char WELLMON_TOPICID[] =  "mwp/data/monitor/well/W001D";
const char WELLMON_JSONID[] =  "mwp/json/data/monitor/well/W001D";
#define WELLMON_LEN 18

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
      float   amp_pump_1    ;
      float   amp_pump_2    ;
      float   amp_pump_3    ;
      float   amp_pump_4    ;
      float   amp_5    ;
      float   amp_6    ;
      float   amp_7    ;
      float   amp_8    ;
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
    "W001D:amp_pump_1",
    "W001D:amp_pump_2",
    "W001D:amp_pump_3",
    "W001D:amp_pump_4",
    "W001D:amp_5",
    "W001D:amp_6",
    "W001D:amp_7",
    "W001D:amp_8",
}  ;

/*
* Block ID: W003D
* Block Name: well3Mon
* Description: Well3 Monitor
* From: well3
* Category: monitor
* Type: data
* MQTT Client ID: Well3 Monitor Client
* MQTT Topic ID: 
* MSG Length: 16
*  word #        data type            variable                description        min        max        nominal
*  0        float            pressurePSI                Pressure in PSI - Well3        0        25        15
*  1        float            temperatureF                Temperature - Well3 Supply Line        0        5000        100
*  2        float            intervalFlow                Gallons in interval        0        100        65
*  3        float            amperage                Amperage        0        125        80
*  4        float            secondsOn                Second on for Pump        0        14400        60
*  5        float            gallonsMinute                Gallons per Minute                        
*  6        float            gallonsDay                Well3 Total Gallons (24hrs)                        
*  7        float            controller                Controller                        
*  8        float            zone                Zone                        
*  9        float            spare1                Spare 1        0        28800        
*  10        float            spare2                Spare 2                        
*  11        float            spare3                Spare 3                        
*  12        float            spare4                Spare 4                        
*  13        float            spare5                Spare 5                        
*  14        float            cycleCount                Cycle Count                        
*  15        float            fwVersion                 FW Version 4 Hex                         
*/

const char WELL3MON_CLIENTID[] =    "Well3 Monitor Client" ;
const char WELL3MON_TOPICID[] =  "mwp/data/monitor/well3/W003D";
const char WELL3MON_JSONID[] =  "mwp/json/data/monitor/well3/W003D";
#define WELL3MON_LEN 16

union   WELL3MON_  {
   float     data_payload[WELL3MON_LEN] ;

   struct  {
      float   pressurePSI    ;
      float   temperatureF    ;
      float   intervalFlow    ;
      float   amperage    ;
      float   secondsOn    ;
      float   gallonsMinute    ;
      float   gallonsDay    ;
      float   controller    ;
      float   zone    ;
      float   spare1    ;
      float   spare2    ;
      float   spare3    ;
      float   spare4    ;
      float   spare5    ;
      float   cycleCount    ;
      float   fwVersion    ;
   }  well3  ;
}  ;
union  WELL3MON_  well3Mon_  ;

char* well3mon_ClientData_var_name [] = { 
    "W003D:pressurePSI",
    "W003D:temperatureF",
    "W003D:intervalFlow",
    "W003D:amperage",
    "W003D:secondsOn",
    "W003D:gallonsMinute",
    "W003D:gallonsDay",
    "W003D:controller",
    "W003D:zone",
    "W003D:spare1",
    "W003D:spare2",
    "W003D:spare3",
    "W003D:spare4",
    "W003D:spare5",
    "W003D:cycleCount",
    "W003D:fwVersion",
}  ;

/*
* Block ID: 
* Block Name: monitor
* Description: Monitor Data for Blynk
* From: monitor
* Category: monitor
* Type: data
* MQTT Client ID: Monitor Client
* MQTT Topic ID: 
* MSG Length: 30
*  word #        data type            variable                description        min        max        nominal
*  0        float            Tank_Water_Height                        0        8        
*  1        float            Tank_Gallons                        0        2500        
*  2        float            Tank_Percent_Full                        0        100        
*  3        float            House_Pressure                        0        100        
*  4        float            Well3_Pressure                        0        100        
*  5        float            Irrigation_Pressure                        0        100        
*  6        float            House_Gallons_Minute                        0        80        
*  7        float            Well3_Gallons_Minute                        0        80        
*  8        float            Irrigation_Gallons_Minute                        0        80        
*  9        float            House_Gallons_Day                        0        4000        
*  10        float            Well3_Gallons_Day                        0        4000        
*  11        float            Irrigation_Gallons_Day                        0        4000        
*  12        float            System_Temp                        -32        120        
*  13        float            House_Water_Temp                        -32        120        
*  14        float            Irrigation_Pump_Temp                        -32        120        
*  15        float            Air_Temp                        -32        120        
*  16        float            Spare1                        0        0        
*  17        float            Spare2                        0        0        
*  18        float            Well_1_LED_Bright                        0        255        
*  19        float            Well_2_LED_Bright                        0        255        
*  20        float            Well_3_LED_Bright                        0        255        
*  21        float            Irrig_4_LED_Bright                        0        255        
*  22        float            Spare1_LED_Bright                        0        255        
*  23        float            Spare2_LED_Bright                        0        255        
*  24        float            Well_1_LED_Color                        0        6        
*  25        float            Well_2_LED_Color                        0        6        
*  26        float            Well_3_LED_Color                        0        6        
*  27        float            Irrig_4_LED_Color                        0        6        
*  28        float            Controller                        0        6        
*  29        float            Zone                        0        32        
*/

const char MONITOR_CLIENTID[] =    "Monitor Client" ;
const char MONITOR_TOPICID[] =  "mwp/data/monitor/monitor/";
const char MONITOR_JSONID[] =  "mwp/json/data/monitor/monitor/";
#define MONITOR_LEN 30

union   MONITOR_  {
   float     data_payload[MONITOR_LEN] ;

   struct  {
      float   Tank_Water_Height    ;
      float   Tank_Gallons    ;
      float   Tank_Percent_Full    ;
      float   House_Pressure    ;
      float   Well3_Pressure    ;
      float   Irrigation_Pressure    ;
      float   House_Gallons_Minute    ;
      float   Well3_Gallons_Minute    ;
      float   Irrigation_Gallons_Minute    ;
      float   House_Gallons_Day    ;
      float   Well3_Gallons_Day    ;
      float   Irrigation_Gallons_Day    ;
      float   System_Temp    ;
      float   House_Water_Temp    ;
      float   Irrigation_Pump_Temp    ;
      float   Air_Temp    ;
      float   Spare1    ;
      float   Spare2    ;
      float   Well_1_LED_Bright    ;
      float   Well_2_LED_Bright    ;
      float   Well_3_LED_Bright    ;
      float   Irrig_4_LED_Bright    ;
      float   Spare1_LED_Bright    ;
      float   Spare2_LED_Bright    ;
      float   Well_1_LED_Color    ;
      float   Well_2_LED_Color    ;
      float   Well_3_LED_Color    ;
      float   Irrig_4_LED_Color    ;
      float   Controller    ;
      float   Zone    ;
   }  monitor  ;
}  ;
union  MONITOR_  monitor_  ;

char* monitor_ClientData_var_name [] = { 
    "Tank_Water_Height",
    "Tank_Gallons",
    "Tank_Percent_Full",
    "House_Pressure",
    "Well3_Pressure",
    "Irrigation_Pressure",
    "House_Gallons_Minute",
    "Well3_Gallons_Minute",
    "Irrigation_Gallons_Minute",
    "House_Gallons_Day",
    "Well3_Gallons_Day",
    "Irrigation_Gallons_Day",
    "System_Temp",
    "House_Water_Temp",
    "Irrigation_Pump_Temp",
    "Air_Temp",
    "Spare1",
    "Spare2",
    "Well_1_LED_Bright",
    "Well_2_LED_Bright",
    "Well_3_LED_Bright",
    "Irrig_4_LED_Bright",
    "Spare1_LED_Bright",
    "Spare2_LED_Bright",
    "Well_1_LED_Color",
    "Well_2_LED_Color",
    "Well_3_LED_Color",
    "Irrig_4_LED_Color",
    "Controller",
    "Zone",
}  ;

/*
* Block ID: 
* Block Name: alert
* Description: Alert Data for Blynk
* From: alert
* Category: alert
* Type: data
* MQTT Client ID: Alert Client
* MQTT Topic ID: 
* MSG Length: 40
*  word #        data type            variable                description        min        max        nominal
*  0        int            Alert1                Alert 1        0        100        
*  1        int            Alert2                 Alert 2        0        100        
*  2        int            Alert3                Alert 3        0        100        
*  3        int            Alert4                Alert 4        0        100        
*  4        int            Alert5                Alert 5        0        100        
*  5        int            Alert6                Alert 6        0        100        
*  6        int            Alert7                Alert 7        0        100        
*  7        int            Alert8                Alert 8        0        100        
*  8        int            Alert9                Alert 9        0        100        
*  9        int            Alert10                Alert 10        0        100        
*  10        int            Alert11                Alert 11        0        100        
*  11        int            Alert12                Alert 12        0        100        
*  12        int            Alert13                Alert 13        0        100        
*  13        int            Alert14                Alert 14        0        100        
*  14        int            Alert15                Alert 15        0        100        
*  15        int            Alert16                Alert 16        0        100        
*  16        int            Alert17                Alert 17        0        100        
*  17        int            Alert18                Alert 18        0        100        
*  18        int            Alert19                Alert 19        0        100        
*  19        int            Alert20                Alert 20        0        100        
*  20        int            Alert21                Alert 21        0        100        
*  21        int            Alert22                Alert 22        0        100        
*  22        int            Alert23                Alert 23        0        100        
*  23        int            Alert24                Alert 24        0        100        
*  24        int            Alert25                Alert 25        0        100        
*  25        int            Alert26                Alert 26        0        100        
*  26        int            Alert27                Alert 27        0        100        
*  27        int            Alert28                Alert 28        0        100        
*  28        int            Alert29                Alert 29        0        100        
*  29        int            Alert30                Alert 30        0        100        
*  30        int            Alert31                Alert 31        0        100        
*  31        int            Alert32                Alert 32        0        100        
*  32        int            Alert33                Alert 33        0        100        
*  33        int            Alert34                Alert 34        0        100        
*  34        int            Alert35                Alert 35        0        100        
*  35        int            Alert36                Alert 36        0        100        
*  36        int            Alert37                Alert 37        0        100        
*  37        int            Alert38                Alert 38        0        100        
*  38        int            Alert39                Alert 39        0        100        
*  39        int            Alert40                Alert 40        0        100        
*/

const char ALERT_CLIENTID[] =    "Alert Client" ;
const char ALERT_TOPICID[] =  "mwp/data/alert/alert/";
const char ALERT_JSONID[] =  "mwp/json/data/alert/alert/";
#define ALERT_LEN 40

union   ALERT_  {
   int     data_payload[ALERT_LEN] ;

   struct  {
      int   Alert1    ;
      int   Alert2     ;
      int   Alert3    ;
      int   Alert4    ;
      int   Alert5    ;
      int   Alert6    ;
      int   Alert7    ;
      int   Alert8    ;
      int   Alert9    ;
      int   Alert10    ;
      int   Alert11    ;
      int   Alert12    ;
      int   Alert13    ;
      int   Alert14    ;
      int   Alert15    ;
      int   Alert16    ;
      int   Alert17    ;
      int   Alert18    ;
      int   Alert19    ;
      int   Alert20    ;
      int   Alert21    ;
      int   Alert22    ;
      int   Alert23    ;
      int   Alert24    ;
      int   Alert25    ;
      int   Alert26    ;
      int   Alert27    ;
      int   Alert28    ;
      int   Alert29    ;
      int   Alert30    ;
      int   Alert31    ;
      int   Alert32    ;
      int   Alert33    ;
      int   Alert34    ;
      int   Alert35    ;
      int   Alert36    ;
      int   Alert37    ;
      int   Alert38    ;
      int   Alert39    ;
      int   Alert40    ;
   }  alert  ;
}  ;
union  ALERT_  alert_  ;

char* alert_ClientData_var_name [] = { 
    "Alert1",
    "Alert2 ",
    "Alert3",
    "Alert4",
    "Alert5",
    "Alert6",
    "Alert7",
    "Alert8",
    "Alert9",
    "Alert10",
    "Alert11",
    "Alert12",
    "Alert13",
    "Alert14",
    "Alert15",
    "Alert16",
    "Alert17",
    "Alert18",
    "Alert19",
    "Alert20",
    "Alert21",
    "Alert22",
    "Alert23",
    "Alert24",
    "Alert25",
    "Alert26",
    "Alert27",
    "Alert28",
    "Alert29",
    "Alert30",
    "Alert31",
    "Alert32",
    "Alert33",
    "Alert34",
    "Alert35",
    "Alert36",
    "Alert37",
    "Alert38",
    "Alert39",
    "Alert40",
}  ;

/*
* Block ID: A001C
* Block Name: alertCommand
* Description: Alert Command Block
* From: blynkAlert
* To: xxx
* Category: alert
* MQTT Client ID: 
* MQTT Topic ID: command/rainbird/command
* MSG Length: 7
*  word #        data type            variable                description        min        max        nominal
*  0        int            clearCommand                Command Word - Clear        0        1        0
*/

const char ALERTCOMMAND_CLIENTID[] =    "" ;
const char ALERTCOMMAND_TOPICID[] =  "mwp/command/rainbird/command";
const char ALERTCOMMAND_JSONID[] =  "mwp/json/command/rainbird/command";
#define ALERTCOMMAND_LEN 7

union   ALERTCOMMAND_  {
   int     data_payload[ALERTCOMMAND_LEN] ;

   struct  {
      int   clearCommand    ;
   }  blynkalert  ;
}  ;
union  ALERTCOMMAND_  alertCommand_  ;

char* alertcommand_ClientData_var_name [] = { 
    "A001C:clearCommand",
}  ;

/*
* Block ID: L001D
* Block Name: logger
* Description: logger Data
* From: logger
* Category: logger
* Type: data
* MQTT Client ID: Logger Client
* MQTT Topic ID: 
* MSG Length: 10
*  word #        data type            variable                description        min        max        nominal
*  0        int            pump                Pump Number 1,2,3 Wells & 4 Irrigation                        
*  1        int            param1                Additional source desc - 1 front 2 back 3 outback                        
*  2        int            param2                Additional source desc - 1-22 zones                        
*  3        int            intervalFlow                Gallons Flowed in Interval                        
*  4        int            pressure                Static Pressure                        
*  5        int            amperage                Integer Represents Amps                        
*  6        int            temperature                Temperature                        
*  7        int            timestamp                unix timestamp                        
*  8        int            unused1                                        
*  9        int            unused2                                        
*  10        int            unused3                                        
*  11        int            unused4                                        
*  12        int            unused5                                        
*  13        int            unused6                                        
*  14        int            unused7                                        
*  15        int            unused8                                        
*  16        int            unused9                                        
*  17        int            unused10                                        
*  18        int            unused11                                        
*  19        int            unused12                                        
*/

const char LOGGER_CLIENTID[] =    "Logger Client" ;
const char LOGGER_TOPICID[] =  "mwp/data/logger/logger/L001D";
const char LOGGER_JSONID[] =  "mwp/json/data/logger/logger/L001D";
#define LOGGER_LEN 10

union   LOGGER_  {
   int     data_payload[LOGGER_LEN] ;

   struct  {
      int   pump    ;
      int   param1    ;
      int   param2    ;
      int   intervalFlow    ;
      int   pressure    ;
      int   amperage    ;
      int   temperature    ;
      int   timestamp    ;
      int   unused1    ;
      int   unused2    ;
      int   unused3    ;
      int   unused4    ;
      int   unused5    ;
      int   unused6    ;
      int   unused7    ;
      int   unused8    ;
      int   unused9    ;
      int   unused10    ;
      int   unused11    ;
      int   unused12    ;
   }  logger  ;
}  ;
union  LOGGER_  logger_  ;

char* logger_ClientData_var_name [] = { 
    "L001D:pump",
    "L001D:param1",
    "L001D:param2",
    "L001D:intervalFlow",
    "L001D:pressure",
    "L001D:amperage",
    "L001D:temperature",
    "L001D:timestamp",
    "L001D:unused1",
    "L001D:unused2",
    "L001D:unused3",
    "L001D:unused4",
    "L001D:unused5",
    "L001D:unused6",
    "L001D:unused7",
    "L001D:unused8",
    "L001D:unused9",
    "L001D:unused10",
    "L001D:unused11",
    "L001D:unused12",
}  ;

/*
* Block ID: 
* Block Name: log
* Description: Log Data
* From: log
* Category: log
* Type: data
* MQTT Client ID: Log Client
* MQTT Topic ID: 
* MSG Length: 8
*  word #        data type            variable                description        min        max        nominal
*  0        int            Controller                Controller        1        5        1
*  1        int            Zone                Zone        1        32        1
*  2        float            pressurePSI                Pressure in PSI - Well3        0        100        65
*  3        float            temperatureF                Temperature - Well3 Supply Line        0        125        80
*  4        float            intervalFlow                Gallons in interval        0        5        0.5
*  5        float            amperage                Amperage        0        2024        1000
*  6        float            secondsOn                Seconds On Time for Pump        0        2024        14400
*  7        float            gallonsTank                Gallons in Tank (pressure sensor)        0        2024        2000
*/

const char LOG_CLIENTID[] =    "Log Client" ;
const char LOG_TOPICID[] =  "mwp/data/log/log/";
const char LOG_JSONID[] =  "mwp/json/data/log/log/";
#define LOG_LEN 8

union   LOG_  {
   int     data_payload[LOG_LEN] ;

   struct  {
      int   Controller    ;
      int   Zone    ;
      float   pressurePSI    ;
      float   temperatureF    ;
      float   intervalFlow    ;
      float   amperage    ;
      float   secondsOn    ;
      float   gallonsTank    ;
   }  log  ;
}  ;
union  LOG_  log_  ;

char* log_ClientData_var_name [] = { 
    "Controller",
    "Zone",
    "pressurePSI",
    "temperatureF",
    "intervalFlow",
    "amperage",
    "secondsOn",
    "gallonsTank",
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
    {0, "Well3",         WELL3SENS_CLIENTID, WELL3SENS_TOPICID, WELL3SENS_JSONID, WELL3SENS_LEN},
    {1, "TANK",          TANKSENS_CLIENTID,     TANKSENS_TOPICID,TANKSENS_JSONID, TANKSENS_LEN},
    {2, "IRRIGATION",    IRRIGATIONSENS_CLIENTID, IRRIGATIONSENS_TOPICID, IRRIGATIONSENS_JSONID,IRRIGATIONSENS_LEN},
    {3, "HOUSE",         HOUSESENS_CLIENTID, HOUSESENS_TOPICID, HOUSESENS_JSONID, HOUSESENS_LEN},
    {4, "WELL",          WELLSENS_CLIENTID, WELLSENS_TOPICID, WELLSENS_JSONID, WELLSENS_LEN},
    {5, "SPARE",         GENERICSENS_CLIENTID, GENERICSENS_TOPICID , GENERICSENS_JSONID, GENERICSENS_LEN},
    {6, "SPARE",         GENERICSENS_CLIENTID, GENERICSENS_TOPICID, GENERICSENS_JSONID, GENERICSENS_LEN},
    {7, "SPARE",         GENERICSENS_CLIENTID, GENERICSENS_TOPICID, GENERICSENS_JSONID, GENERICSENS_LEN},
};
