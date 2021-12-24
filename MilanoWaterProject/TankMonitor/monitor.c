/*******************************************************************************
** Interface to Blynk Server with Data
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
//#include "json.h"

//#define ADDRESS     "tcp://localhost:1883"
//#define CLIENTID    "Tank Subscriber"
//#define TOPIC       "Tank ESP"
//#define PAYLOAD     "Hello World!"

#define THINGSBOARD_HOST "tcp://demo.thingsboard.io:1883"
#define TB_ACCESS_TOKEN "DfOGk0Hhxl19rgmJvwf6"
#define TB_CLIENT_ID "54248e50-930a-11eb-93db-895db613ca88"

#define QOS         0
#define TIMEOUT     10000L
#define ADDRESS     "tcp://soldier.cloudmqtt.com:15599"
#define mqttPort 15599 
#define mqttUser "zerlcpdf"
#define mqttPassword  "OyHBShF_g9ya" 
#define CLIENTID    "Tank Subscriber"
#define TOPIC       "Tank ESP"
#define PAYLOAD     "Hello World!"

#define datafile "./datafile.txt"

    FILE *fptr;
    time_t t;
    struct tm * myTime; // Pointer to tm structure
    struct tm t_excel = { 0, 0, 0, 0, 0, 0, 0, 0, 0 } ; //excel date starts on 1900-Jan-0
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
unsigned short int data_payload[22] ;
unsigned short int ESP_payload[4] ;
float SystemData[70] ;
float blynk_payload [30];
int firmware = 0;
int SubFirmware = 0x80FF;

volatile MQTTClient_deliveryToken deliveredtoken;
MQTTClient client;
MQTTClient TB_client;
#define TRUE 1  //now defined in json.h
#define FALSE 0 //now defined in json.h
#define ON 1
#define OFF 0
#define AUTO 2
#define VoltoGal 7.048052
#define MaxTankGal  2290.  //based on current float height
#define PI 3.1415926535897932384626433832795
#define calibrationFactor 11.28 //Each flowsensor pulse is 11.28 ml
#define GREEN   0
#define BLUE    1
#define ORANGE  2
#define RED     3 

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

float TotalDailyGallons = 0;
float TotalGPM = 0;

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

struct pump Pump[NPumps+1];     //Add 1 so that I can start at 1,2,3 not 0
int PumpControlWord = 0 ;

void FloatManageCallback()
{
   static int DebounceCount = 0;
   static int previousHifloat  = 0;

/*
 * Debounce the float switch by allowing it time to stabilize
 */

   if ( DebounceFloatInProgress = TRUE ) {
      ++DebounceCount;
      if ( DebounceCount == 10 ) {
         DebounceCount = 0;
         DebounceFloatInProgress = FALSE;
      }
   }
   else {
      if ( readFloatHi != previousHifloat ) {
         DebounceFloatInProgress = TRUE;
      }
      else {
         HighFloat = readFloatHi;
      } 
     
   } 
   previousHifloat = readFloatHi;
   
}

void PumpControl()
{

static int seconds = 0;
static int PNum = 1 ;
static int Interval;
static int CalcInterval;

   Pump[1].PumpScaleFactor = Pump1ScaleFactor ;
   Pump[2].PumpScaleFactor = Pump2ScaleFactor ;
   Pump[3].PumpScaleFactor = Pump3ScaleFactor ;
 
 /*
  * This module will run on a fixed interval to sense the state of the pumps and floats
  * and will control the pumps on/off state. The purpose is to ensure that the pumps
  * operate on a round-robin schedule, the duration determined by a constant specified
  * above.
  * This module will also compute stats for pump efficieny and run time efficiency
  * which can be used to spot issues and to fine tune the running of the system.
  */

  /*
   * First Get the APP Commanded Mode - Auto, On, Off
   */
   
   
   Pump[1].RunCommanded = data_payload[21] & 0b000011 ;
   Pump[2].RunCommanded = (data_payload[21] & 0b001100) >> 2;
   Pump[3].RunCommanded = (data_payload[21] & 0b110000) >> 4 ;
   //printf("Pump 1 Command: %d\n", Pump[1].RunCommanded);
   //printf("Pump 2 Command: %d\n", Pump[2].RunCommanded);
   //printf("Pump 3 Command: %d\n", Pump[3].RunCommanded);

/*
 * Build an On/Off Control Word to send to ESP
 */
 
  if ( Pump[1].RunCommanded == 1 ) {
      PumpControlWord = PumpControlWord | 0b00010000 ;
  }
  else {
      PumpControlWord = PumpControlWord & 0b11101111 ;
  }
  if ( Pump[2].RunCommanded == 1 ) {
      PumpControlWord = PumpControlWord | 0b00100000 ;
  }
  else {
      PumpControlWord = PumpControlWord & 0b11011111 ;
  }
  if ( Pump[3].RunCommanded == 1 ) {
      PumpControlWord = PumpControlWord | 0b01000000 ;
  }
  else {
      PumpControlWord = PumpControlWord & 0b10111111 ;
  } 
  //printf("Pump Control Word: %x\n", PumpControlWord);
 

 /*
  * Firtst Test for 'No Run' Conditions
  * 1. High Float -Tank Full
  * 2. Low Float - Tank is Dangerously Low
  * 3. Waiting for Float to Settle
  */
/*
   if ( HighFloat == TRUE || DebounceFloatInProgress == TRUE ) {  
      for (PNum = 1; PNum <= 3; ++PNum) {   
         Pump[PNum].RunCommanded = OFF; //Command the Pump to Off
         Interval = 0 ;                 //Reset the Interval Count
      }                                  //Recalculate the Pump Stats
   }
   else {
   ++Interval;
  
   if ( CalcInterval <= PumpCycleTime ) {
      if (Interval <= CalcInterval) {
         Pump[PNum].RunCommanded = ON;	                                   //Command on/off	
         Pump[PNum].RunTimeCommanded++;	                                   //Commanded Run Time seconds	
         if (Pump[PNum].RunStatus == ON) Pump[PNum].RunTimeActual++;       //Actual Run Time in seconds		
         //Pump[PNum].FlowRate = flowGallons;                                //Gallons in Interval	
         Pump[PNum].TotalFlow = Pump[PNum].TotalFlow + Pump[PNum].FlowRate; //GPM	
      }
   }
   else {
      //Pump[PNum].FlowEfficiency = Pump[PNum].TotalFlow / MaxFlowEfficiency;                // Pump Flow %	
      //Pump[PNum].PumpEfficiency = Pump[PNum].RunTimeActual / Pump[PNum].RunTimeCommanded ; // Run Time %
   }
      
    Pump[PNum].RunCommanded = OFF;                                     //Command the Pump Off

    Interval = 0;
    Pump[PNum].CycleTime++;	                                 //Number of Cycles in a 24 Hr Period
   // Pump[PNum].DailyFlowEfficiency = (Pump[PNum].DailyFlowEfficiency +  Pump[PNum].PumpEfficiency) / Pump[PNum].CycleTime;  // Pump Flow %	
    //Pump[PNum].DailyPumpEfficiency = (Pump[PNum].DailyPumpEfficiency +  Pump[PNum].PumpEfficiency) / Pump[PNum].CycleTime; // Run Time %	
   }
   */
}

void PumpStatus(){

#define POFF 0
#define PRUN 1
#define PPROT 2
#define PEMER 3

static int second_count = 0 ;   //Loop Counter: 0-59 then reset = 1 Minute
static int minute_count = 0 ;   //Loop Counter: 0-59 then reset = 1 Hour 
       int i;
   for (i=1; i<NPumps+1; i++) {      
       
      if ( Pump[i].RunCommanded == ON ) {
         if ( Pump[i].PumpPower == ON ) {
            Pump[i].RunStatus = PRUN;
            Pump[i].RunTimeActual++ ;
            if ( Pump[i].FlowRate <= .01 )
               Pump[i].RunStatus = PEMER;
         }
         else {
            Pump[i].RunStatus = PPROT ;
         }
      }
      else {
        Pump[i].RunStatus = POFF;
        if (Pump[i].PumpPower == ON) {
           Pump[i].RunStatus = PEMER;
        }
      }
  }

}  
    int Pump1State = 0;
    int Pump2State = 0;
    int Pump3State = 0;
    int Pump4State = 0;
    int Float100State = 0;
    int Float90State = 0;
    int Float50State = 0;
    int Float25State = 0;
    int PressSwitState = 0;
    int raw_voltage1_adc = 0;
    int raw_voltage2_adc = 0;
    int raw_voltage3_adc = 0;
    int raw_voltage4_adc = 0;
void ConvertSensorData()
{
    int i;
 static int j;
 static int k;
    int PNum;
    const int Diameter1 = 8;                                     //internal Diameter of tank 1 in Ft           
    const int Depth1 = 8;                                        //total depth of tank in Ft  
    const int Area1 = PI * ((Diameter1 / 2) * (Diameter1 / 2));  //area of base of tank 1
    
    float PresSensorLSB = .0000625;   //lsb voltage value from datasheet
    static float PresSensorValueArray[10] = {0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
    float PresSensorValue;
    float WaterPresSensorValue;    
    static int   PresIndex = 0;
    float ConstantX = .34;      //Used Excel Polynomial Fitting to come up with equation
    float Constant  = .0962;
    float PresSensorAverage;
    float WaterHeight = 0;
    float TankGallons = 0;
    float TankPerFull = 0;

    unsigned short int PresSensorRawValue;
    
    int pressState = 0;
    int pressLedColor ;
    int floatstate[5];
    int floatLedcolor[5];
    int PumpCurrentSense[4];
    int PumpLedColor[4]; 

    int flowCount = 0;
    int flowGap = 0;
static int P1TotalflowCount = 0;
static int P2TotalflowCount = 0;
static int P1TotalflowGap = 0;
static int P2TotalflowGap = 0;
    float flowRate;
    float flowMilliLitres ;
    float flowGallons = 0;    
static float P1accumulatedGPM = 0;
static float P2accumulatedGPM = 0;
    
    int raw_temp = 0;
    float AmbientTempF = 0;
    float AmbientTempC = 0;
   
   /*
    * Convert Raw hydrostatic Pressure Sensor
    * A/D to Water Height, Gallons & Percent Full
    */

   PresSensorRawValue = data_payload[8]; 
   
   /*
    * Rolling Average to Smooth data
    */
    
   PresSensorValueArray[PresIndex] = PresSensorRawValue * PresSensorLSB;        //Convert sensor value to voltage
   ++PresIndex;
   if (PresIndex == 10) PresIndex = 0;

   for( i=0; i<=9; ++i){
       PresSensorAverage = PresSensorAverage + PresSensorValueArray[i];
   }
   PresSensorValue = PresSensorAverage/10; 
   //printf("%d  Sensor Value: %f\n",  data_payload[9], PresSensorValue);
   
   /*
    *** Use the Equation y=Constandx(x) + Constant solve for x to compute Water Height in tank 
    */

   if (PresSensorValue <= Constant)
   { 
    PresSensorValue = Constant;
   }
    
   WaterHeight = ((PresSensorValue-ConstantX)/Constant) +.1;  //The .1 accounts for the sensor not sitting on the bottom
   //printf("Water Height = %f\n", WaterHeight);

   /*
    * Use the Equation (PI*R^2*WaterHeight)*VoltoGal to compute Water Gallons in tank 
    */
   TankGallons = ((Area1)*WaterHeight)*VoltoGal;
   //printf("Gallons in Tank = %f\n", TankGallons);

   /*
    *  Use the Equation Calculated Gallons/Max Gallons to compute Percent Gallons in tank 
    */

   TankPerFull = TankGallons/MaxTankGal * 100;
   //printf("Percent Gallons in Tank = %f\n", TankPerFull);
   
   // Channel 2 Voltage Sensor 16 bit data
     raw_voltage1_adc = (int16_t) data_payload[1]; 
     if (raw_voltage1_adc > 2500) {
        Pump1State = 1;
        PumpCurrentSense[1] = 255;
        Pump[1].PumpPower = ON;
        PumpLedColor[1] = BLUE; }
     else {
        Pump1State =0;
        PumpCurrentSense[1] = 255 ;
        Pump[1].PumpPower = OFF;
        PumpLedColor[1] = GREEN ; }
       
 // Channel 3 Voltage Sensor 16 bit data
     raw_voltage2_adc = (int16_t) data_payload[2]; 
     //printf("voltage ch 2: %d\n", raw_voltage2_adc);
     if (raw_voltage2_adc > 500){
        Pump2State = 1;
        PumpCurrentSense[2] = 255;
        Pump[2].PumpPower = ON;
        PumpLedColor[2] = BLUE;}
     else {
        Pump2State = 0;
        PumpCurrentSense[2] = 255 ;
        Pump[2].PumpPower = OFF;
        PumpLedColor[2] = GREEN ; }
 
// Channel 4 Voltage Sensor 16 bit data
     raw_voltage3_adc = (int16_t) data_payload[3];
     if (raw_voltage3_adc > 500){
        Pump3State = 1;
        PumpCurrentSense[3] = 255;
        Pump[3].PumpPower = ON;
        PumpLedColor[3] = BLUE;}
     else {
        Pump3State = 0;
        PumpCurrentSense[3] = 255;
        Pump[3].PumpPower = OFF;
        PumpLedColor[3] = GREEN ;}

// MCP3428 #2 Channel 4 Voltage Sensor 16 bit data
     WaterPresSensorValue = (int16_t)data_payload[9] * .00275496;
     //printf("Raw Water Pressure: 0%x  %d  %f\n", data_payload[9],data_payload[9], WaterPresSensorValue);
     raw_voltage4_adc = (int16_t) data_payload[11];
     if (raw_voltage4_adc > 500){
        Pump4State = 1;
        PumpCurrentSense[4] = 255;
        Pump[4].PumpPower = ON;
        PumpLedColor[4] = BLUE;}
     else {
        Pump4State = 0;
        PumpCurrentSense[4] = 255;
        Pump[4].PumpPower = OFF;
        PumpLedColor[4] = GREEN ;}

   /*
    * Convert the Discrete data
    */
    
     Float100State  = (data_payload[4] & 0x0001) ;
     Float90State   = (data_payload[4] & 0x0002) >> 1;
     Float50State   = (data_payload[4] & 0x0004) >> 2;
     Float25State   = (data_payload[4] & 0x0008) >> 3;
     PressSwitState = (data_payload[4] & 0x0010) >> 4;
     
     if (Float100State == 1){
        floatstate[1] = 255;
        floatLedcolor[1] = GREEN;}
     else {
        floatstate[1] = 255;
        floatLedcolor[1] = RED;}
     
     if (Float90State == 1){
        floatstate[2] = 255;
        floatLedcolor[2] = GREEN;}
     else {
        floatstate[2] = 255;
        floatLedcolor[2] = RED; }
        
     if (Float50State == 1){
        floatstate[3] = 255;
        floatLedcolor[3] = GREEN;}
     else {
        floatstate[3] = 255;
        floatLedcolor[3] = RED; }
        
     if (Float25State == 1){
        floatstate[4] = 255;
        floatLedcolor[4] = GREEN;}
     else {
        floatstate[4] = 255;
        floatLedcolor[4] = RED; }
     
     if (PressSwitState == 1){
        pressState = 255;
        pressLedColor = BLUE;}
     else {
        pressState = 255;
        pressLedColor = GREEN; }

     // printf("Hi FLoat: %x  Low Float: %x\n", HighFloatState, LowFloatState) ;
     
     /*
      * Calculate flow in Gallons
      */

     flowCount = (int) data_payload[6];
     flowGap = (int) data_payload[7] ;

     if ( Pump[1].PumpPower == ON ) {
        PNum = 1;
        if (flowCount != 0 ) {
           P1TotalflowCount += flowCount;
           P1TotalflowGap += flowGap;
           flowRate = flowCount * Pump[PNum].PumpScaleFactor;

           /*    
            * Determine how many litres have
            * passed through the sensor in this 1 second interval
            */ 
            flowMilliLitres = flowRate;
            flowGallons = (flowMilliLitres/1000)/3.79;
            //printf("Pump 1 flow count %d -- Instant Flow Rate %f -- Gallons: %f\n", flowCount, flowRate,flowGallons);
            Pump[PNum].TotalFlow += flowGallons ;    
       
            P1accumulatedGPM += flowGallons;
            ++j;
            if ( j == 4 ) {
              TotalGPM = ((5000/P1TotalflowGap) * P1accumulatedGPM) * 12;
              Pump[PNum].FlowRate = TotalGPM ;
              //printf("Pump 1 Total Flow Count %d -- TotalaccumulateGPM %f -- TotalGPM: %f\n", P1TotalflowCount, P1accumulatedGPM, TotalGPM);
              j= 0;
              P1accumulatedGPM = 0;
              P1TotalflowCount = 0;
              P1TotalflowGap = 0;
              }
           }
       }
       else {
          //Pump[1].TotalFlow = 0;
       }
       if ( Pump[2].PumpPower == ON ) {
          PNum = 2;
          if (flowCount != 0 ) {
             P2TotalflowCount += flowCount;
             P2TotalflowGap += flowGap;
             flowRate = flowCount * Pump[PNum].PumpScaleFactor;

           /*    
            * Determine how many litres have
            * passed through the sensor in this 1 second interval
            */ 
            flowMilliLitres = flowRate;
            flowGallons = (flowMilliLitres/1000)/3.79;
            //printf("Pump 2 flow count %d -- Instant Flow Rate %f -- Gallons: %f\n", flowCount, flowRate,flowGallons);
            Pump[PNum].TotalFlow += flowGallons ;    
       
            P2accumulatedGPM += flowGallons;
            ++k;
            if ( k == 4 ) {
              TotalGPM = ((5000/P2TotalflowGap) * P2accumulatedGPM) * 12;
              //printf("Pump 2 Total Flow Gap %d Flow Count %d -- TotalAccumulateGPM %f -- TotalGPM: %f\n", P2TotalflowGap, P2TotalflowCount, P2accumulatedGPM, TotalGPM);
              Pump[PNum].FlowRate = TotalGPM ;
              k = 0;
              P2accumulatedGPM = 0;
              P2TotalflowCount = 0;
              P2TotalflowGap = 0;
              }
           }
         }
         else {
            //Pump[2].TotalFlow = 0;
         }
         TotalDailyGallons = (Pump[1].TotalFlow + Pump[2].TotalFlow);
         //printf("--- Total Daily Gallons Pump1: %f \n", Pump[1].TotalFlow );
         //printf("+++ Total Daily Gallons Pump2: %f \n", Pump[2].TotalFlow );
         //printf("*** Total Daily Gallons: %f ***\n", TotalDailyGallons);
   /*
    * Convert Raw Temp Sensor
    * to degrees farenhiet
    */
       
    raw_temp = data_payload[5]; 
    AmbientTempC = raw_temp * .0625; //LSB weight for 12 bit conversion is .0625
    AmbientTempF = (AmbientTempC * 1.8) + 32.0;
    //printf("Ambient Temp:%f  \n", AmbientTempF); 
 
    /*
     * Set Firmware Version
     */
     
     firmware = data_payload[20] & SubFirmware;
 
   /*
    * Load Up the System Data
    */

   SystemData[0] = PresSensorValue;
   SystemData[1] = WaterHeight;
   SystemData[2] = TankGallons;
   SystemData[3] = TankPerFull;
   SystemData[4] = TotalGPM;
   SystemData[5] = PumpCurrentSense[1];
   SystemData[6] = PumpCurrentSense[2];
   SystemData[7] = PumpCurrentSense[3];
   SystemData[8] = firmware; 
   SystemData[9] = data_payload[16];  //I2C Faults
   SystemData[10] = data_payload[12]; //Cycle Count
   SystemData[11] = AmbientTempF;
   SystemData[12] = floatstate[1];
   SystemData[13] = floatstate[2];
   SystemData[14] = floatstate[3];
   SystemData[15] = floatstate[4];
   SystemData[16] = PumpLedColor[1];
   SystemData[17] = PumpLedColor[2];
   SystemData[18] = PumpLedColor[3];
   SystemData[19] = floatLedcolor[1];
   SystemData[20] = floatLedcolor[2];
   SystemData[21] = floatLedcolor[3];
   SystemData[22] = floatLedcolor[4];
   SystemData[23] = Pump[1].RunCommanded;
   SystemData[24] = Pump[2].RunCommanded;
   SystemData[25] = Pump[3].RunCommanded;
   SystemData[26] = WaterPresSensorValue;
   SystemData[27] = PumpCurrentSense[4];
   SystemData[28] = PumpLedColor[4];
   SystemData[29] = pressState;
   SystemData[30] = pressLedColor;
}  

void LoadData()
{
static int interval = 0;
 
   int i;
   int j;
   int Offset = 31;
   
   j = Offset; 
   for (i=1; i<=NPumps; i++) {      
      SystemData[j]   = Pump[i].RunCommanded ;
      SystemData[j+1] = Pump[i].RunStatus ;
      SystemData[j+2] = Pump[i].PumpPower ;
      SystemData[j+3] = Pump[i].RunTimeCommanded ;
      SystemData[j+4] = Pump[i].RunTimeActual ;
      SystemData[j+5] = Pump[i].CycleTime ;
      SystemData[j+6] = Pump[i].FlowRate ;
      SystemData[j+7] = Pump[i].TotalFlow ;
      SystemData[j+8] = Pump[i].TimestampOn ;    
      
      j += 10;
   }
   ++interval;
   
   if (( Pump[1].PumpPower == ON ) || 
       ( Pump[2].PumpPower == ON ) ||
       ( Pump[3].PumpPower == ON ) ||
       ( interval == 120 )) {      
      fptr = fopen(datafile, "a");
      for (i=0; i<30; i++) {
         fprintf(fptr, "%f ", SystemData[i] );
         //printf("%f\n", SystemData[i]);
      }
      fprintf(fptr, "%s", ctime(&t));
      fclose(fptr);

  }
  if ( interval == 120 ) interval = 0;
}    
/*
void SendDataToThingsBoard()
{
   int buff_length;
   char payload[800] ;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   
   json_object *my_object, *my_attribute;

   my_object = json_object_new_object();

	json_object_object_add(my_object, "Tank Pressure"    , json_object_new_double(SystemData[0]));
  json_object_object_add(my_object, "Water Height"     , json_object_new_double(SystemData[1]));
  json_object_object_add(my_object, "Tank Gallons"     , json_object_new_double(SystemData[2]));
  json_object_object_add(my_object, "Tank Percent Full", json_object_new_double(SystemData[3]));
  json_object_object_add(my_object, "GPM"              , json_object_new_double(SystemData[4]));
  json_object_object_add(my_object, "Pump 1 Amps"      , json_object_new_double(raw_voltage1_adc));
  json_object_object_add(my_object, "Pump 2 Amps"      , json_object_new_double(raw_voltage2_adc));
  json_object_object_add(my_object, "Pump 3 Amps"      , json_object_new_double(raw_voltage3_adc));
  json_object_object_add(my_object, "Daily Gallons"    , json_object_new_double(SystemData[8]));
  json_object_object_add(my_object, "I2C Faults"       , json_object_new_double(SystemData[9]));
  json_object_object_add(my_object, "Cycle Count"      , json_object_new_double(SystemData[10]));
  json_object_object_add(my_object, "Temperature"      , json_object_new_double(SystemData[11]));
  json_object_object_add(my_object, "Float 1 State"    , json_object_new_double(Float100State));
  json_object_object_add(my_object, "Float 2 State"    , json_object_new_double(Float90State));
  json_object_object_add(my_object, "Float 3 State"    , json_object_new_double(Float50State));
  json_object_object_add(my_object, "Float 4 State"    , json_object_new_double(Float25State));
  json_object_object_add(my_object, "Home Pressure"    , json_object_new_double(SystemData[26]));
  json_object_object_add(my_object, "Pump 4 Amps"      , json_object_new_double(raw_voltage4_adc));
  json_object_object_add(my_object, "Home Demand"      , json_object_new_double(PressSwitState));


  printf("my_object=\n");
	json_object_object_foreach(my_object, key, val)
	{
		printf("\t%s: %s\n", key, json_object_to_json_string(val));
	}
	printf("my_object.to_string()=%s\n", json_object_to_json_string(my_object));

  strcpy( payload, json_object_to_json_string(my_object) );
  buff_length = strlen(payload);
  //printf("the payload is %d bytes long\n", buff_length);

  
    pubmsg.payload = payload;
    pubmsg.payloadlen = buff_length;
    pubmsg.qos = QOS;
    pubmsg.retained = 1;
  // MQTTClient_publishMessage(TB_client, "v1/devices/me/telemetry", &pubmsg, &token);
  
   my_attribute = json_object_new_object();
  
  json_object_object_add(my_attribute, "attribute1"    , json_object_new_boolean(Float100State));
  json_object_object_add(my_attribute, "attribute2"    , json_object_new_boolean(Float90State));
  json_object_object_add(my_attribute, "attribute3"    , json_object_new_boolean(Float50State));
  json_object_object_add(my_attribute, "attribute4"    , json_object_new_boolean(Float25State));
  json_object_object_add(my_attribute, "attribute5"    , json_object_new_boolean(Pump1State));
  json_object_object_add(my_attribute, "attribute6"    , json_object_new_boolean(Pump2State));
  json_object_object_add(my_attribute, "attribute7"    , json_object_new_boolean(Pump3State));
  json_object_object_add(my_attribute, "attribute8"    , json_object_new_boolean(Pump4State));
  json_object_object_add(my_attribute, "attribute9"    , json_object_new_boolean(PressSwitState));
  json_object_object_add(my_attribute, "attribute10"    , json_object_new_int(firmware));
  
  strcpy( payload, json_object_to_json_string(my_attribute) );
  buff_length = strlen(payload);
    
  pubmsg.payload = payload;
  pubmsg.payloadlen = buff_length;
  pubmsg.qos = QOS;
  pubmsg.retained = 1;
 // MQTTClient_publishMessage(TB_client, "v1/devices/me/attributes", &pubmsg, &token);

}
*/
void SendDataToBlynk()
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    int i;
   
   /*
    * Load Up the Payload
    */
   for (i=0; i<=30; i++) {
      blynk_payload[i] = SystemData[i] ;
      //printf("%f\n", blynk_payload[i]);
   } 
   
   pubmsg.payload = blynk_payload;
   pubmsg.payloadlen = 124;
   pubmsg.qos = QOS;
   pubmsg.retained = 1;
   MQTTClient_publishMessage(client, "Tank Blynk", &pubmsg,&token);
     
}
void SendDataToESP()
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int i;
    static int LastPumpControlWord =  0;
   /*
    * Load Up the Payload
    */
   if (PumpControlWord != LastPumpControlWord ) {
      ESP_payload[0] = PumpControlWord ;
      LastPumpControlWord = PumpControlWord ;

      pubmsg.payload = ESP_payload;
      pubmsg.payloadlen = 2;
      pubmsg.qos = QOS;
      pubmsg.retained = 1;
      MQTTClient_publishMessage(client, "ESP Control", &pubmsg, &token);
  }
}

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    unsigned short int raw_data_payload[21] ;
    unsigned short int* payloadptr;
    time(&t);
    static int faultCount = 0;
  
    //printf("Recv Data Arrived - Topic Name: %s Payload length: %d\n", topicName, message->payloadlen);
    
    if (message->payloadlen != 0) {
       if (message->payloadlen == 2 ) {
          printf("%s  App Data Payload: ", ctime(&t));
          payloadptr = message->payload;
          data_payload[21] = *payloadptr++ ;
          printf("%0X \n", data_payload[21]);
          MQTTClient_freeMessage(&message);
          MQTTClient_free(topicName); 
       }
       else {
          printf("ESP8266: ");
          payloadptr = message->payload;
          for(i=0; i < (message->payloadlen/2); i++)
          {
             raw_data_payload[i] = *payloadptr++ ;
             printf("%0X ", raw_data_payload[i]);
          }
          printf("%0X ", raw_data_payload[21]);
          printf("%s", ctime(&t));
          MQTTClient_freeMessage(&message);
          MQTTClient_free(topicName);
              
          if ( raw_data_payload[16] != faultCount ) {
             faultCount = raw_data_payload[16];
             data_payload[16] = raw_data_payload[16];
             printf("Error - Buffer Flushed\n");
           }
           else {
              for ( i=0; i<=20; i++) {
                 data_payload[i] = raw_data_payload[i];
              }
           }
      }
   }
   
   return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
   
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connectOptions TB_conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;


    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = mqttUser;
    conn_opts.password = mqttPassword;
    MQTTClient_setCallbacks(client, NULL,connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);
/*
   MQTTClient_create(&TB_client, THINGSBOARD_HOST, TB_CLIENT_ID,      
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    TB_conn_opts.keepAliveInterval = 60;
    TB_conn_opts.cleansession = 1;
    TB_conn_opts.username = TB_ACCESS_TOKEN;
    TB_conn_opts.password = TB_ACCESS_TOKEN;

    if ((rc = MQTTClient_connect(TB_client, &TB_conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect to Thingsboard, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
 */
    
    /*
     * Start Execution Loop
     */
         
    while (1) 
    { 
       ConvertSensorData();
       PumpControl();
       PumpStatus();
       LoadData();
       SendDataToBlynk();
       //SendDataToThingsBoard();
       SendDataToESP();
       
       sleep(2);
    }

    MQTTClient_unsubscribe(client, TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
