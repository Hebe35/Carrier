// 23.07.2022
#include <avr/wdt.h>
#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Timer.h>
#include <DHT.h>                // From https://github.com/adafruit/DHT-sensor-library
#include <CarrierHeatpumpIR.h>  // From HeatpumpIR library, https://github.com/ToniA/arduino-heatpumpir/archive/master.zip
#include "emoncmsApikey.h"      // This only defines the API key. Excluded from Git, for obvious reasons

#define FIREPLACE_FAN_PIN                  49 // Pin for the fireplace relay
#define WATER_STOP_VALVE_PIN               29 // Pin for the water stop relay
#define WAREHOUSE_RELAY_PIN               A7 // Pin for the relay in the warehouse and electric car sharge 3*16a
#define ILP_HEAT_CABLE_RELAY_PIN          A8 // Pin for the ILP heat cable relay 
#define WATER_HEAT_RELAY_PIN              A10 // Pin for the water heat relay 
#define HOUSE_HEAT_RELAY_PIN              A11 // Pin for the  house heat drop relay
#define HOUSE_HEAT_RELAY2_PIN             A9 // Pin for the  house heat drop relay
#define HOUSE_HEAT_FISSIO_STATE_PIN       A12 // Pin for the  house heat drop from fissio
#define HOUSE_HEAT_FISSIO_STATE_2_PIN      47 // Pin for the  house heat drop from fissio
#define WATER_HEAT_FISSIO_STATE_PIN        16 // Pin for the water heat Fissio (pun/valk)
//#define WATER_HEAT_1000W_RELAY_PIN      A9 // Pin for the water heat 1000w relay 

#define DHT11_PIN            39 // Pin for the DHT11 temperature/humidity sensor, uses +5, GND and some digital pin
#define MQ7_PIN             A15 // Pin for the MQ-7 CO sensor pin, uses +5V, GND and some analog pin
#define MG811_PIN           A14 // Pin for the MG-811 Co2 sensor pin, uses +5V, GND and some analog pin
#define LIGHTNESS_PIN       A13 // Pin for the lightness sensor pin, uses +5V, GND and some analog pin_PIN            
#define IR_PIN                9 // Pin for the IR led, must be a PWM pin 9, 10, 11 ,12 ,46 not work enymore

#define ALARM_STATE_PIN      22 // Pin for the alarm system state
#define WATER_HEAT_STATE_PIN 17 // Pin for the water heat system state
#define SAUNA_HEAT_STATE_PIN 15 // Pin for the sauna heat system state

// Muuttujat

//House heat drop control with outside temp
const long HouseHeatOff          = 16;   // House heat drop ON temp  14 C°
const long HouseHeatOn           = 14;   // House heat drop OFF temp 12 C°
//Fireplace fan control
const long FireplaceFanOn           = 25;   // Fireplace fan ON temp  25 C°
const long FireplaceFanOff          = 24;   // Fireplace fan OFF temp 24 C°
//ILP heat cable control
const long ILPHeatCableControlOn    = -10;  // ILP heat cable control ON temp -10 C°
const long ILPHeatCableControlOff   = -9;   // ILP heat cable control OFF temp -9 C°
//water boiler Heat
const long WaterBoilerTopHeatOn     = 30;   // If water boiler top Temp drop under 30 C° heat ON 
//Shower water use
const long ShowerWaterUse           = 10;   // Shower water use l/min
const long ShowerWaterUseCutMin     = 6;    // Shower water use cut min 12min- ? min


// Use digital pins 4, 5, 6, 7, 8, 14, 10, and analog pin 0 to interface with the LCD
// Do not use Pin 10 while this shield is connected
// Use digital pins pins 10, 11, 12, and 13 to interface with the shield

LCDKeypad lcd;

// DS18B20 sensor wire colors:
// * Red: Vcc
// * Yellow: Ground
// * Green: Data

// 1-wire buses

OneWire ow0(30);
DallasTemperature owsensors0(&ow0);
OneWire ow1(38);
DallasTemperature owsensors1(&ow1);
OneWire ow2(34);
DallasTemperature owsensors2(&ow2);
OneWire ow3(31);
DallasTemperature owsensors3(&ow3);
OneWire ow4(33);
DallasTemperature owsensors4(&ow4);
OneWire ow5(27);
DallasTemperature owsensors5(&ow5);
OneWire ow6(24);
DallasTemperature owsensors6(&ow6);
OneWire ow7(42);
DallasTemperature owsensors7(&ow7);
OneWire ow8(44);
DallasTemperature owsensors8(&ow8);
OneWire ow9(40);
DallasTemperature owsensors9(&ow9);
OneWire ow10(43);
DallasTemperature owsensors10(&ow10);
OneWire ow11(41);
DallasTemperature owsensors11(&ow11);
OneWire ow12(37);
DallasTemperature owsensors12(&ow12);
OneWire ow13(23);
DallasTemperature owsensors13(&ow13);
OneWire ow14(36);
DallasTemperature owsensors14(&ow14);
OneWire ow15(32);
DallasTemperature owsensors15(&ow15);
OneWire ow16(26);
DallasTemperature owsensors16(&ow16);
OneWire ow17(28);
DallasTemperature owsensors17(&ow17);

// Structure to hold them
typedef struct owbus owbus;
struct owbus
{
    DallasTemperature owbus;
    float temperature;
    char* emon_name;
    char* name;
};

// and the array
owbus owbuses[] = {
  {owsensors0, DEVICE_DISCONNECTED, "fireplace", "Takkahuone"},             // Fireplace
  {owsensors1, DEVICE_DISCONNECTED, "kitchen", "Keitti\xEF"},                // Kitchen
  {owsensors2, DEVICE_DISCONNECTED, "utl_room", "Kodinhoitohuone"},          // Utility room
  {owsensors3, DEVICE_DISCONNECTED, "bedroom", "Julian huone"},              // Bedroom
  {owsensors4, DEVICE_DISCONNECTED, "master_bedroom", "Makuuhuone"},         // Master bedroom
  {owsensors5, DEVICE_DISCONNECTED, "warehouse", "Varasto"},                 // Warehouse
  {owsensors6, DEVICE_DISCONNECTED, "outdoor", "Ulkoilma"},                  // Outdoor air
  {owsensors7, DEVICE_DISCONNECTED, "aircond_intake", "Imuilma \x7E ILP "},  // Carrier intake air
  {owsensors8, DEVICE_DISCONNECTED, "aircond_out", "ILP \x7E puhallus"},     // Carrier outlet air
  {owsensors9, DEVICE_DISCONNECTED, "aircond_hotpipe", "ILP kuumakaasu"},    // Carrier hot gas pipe
  {owsensors10,DEVICE_DISCONNECTED, "boiler_mid", "Kuumavesivar.keski"},     // Hot water boiler middle
  {owsensors11,DEVICE_DISCONNECTED, "boiler_top", "Kuumavesivar.yl\xE1"},    // Hot water boiler up
  {owsensors12,DEVICE_DISCONNECTED, "hot_water", "Kuumavesi"},               // Hot water
  {owsensors13,DEVICE_DISCONNECTED, "water", "Tuleva vesi"},                 // Cold Water
  {owsensors14,DEVICE_DISCONNECTED, "vent_outdoor", "Ulkoilma \x7E LTO"},    // Ventilation machine fresh air in
  {owsensors15,DEVICE_DISCONNECTED, "vent_fresh", "LTO l\xE1mmitt\xE1\xE1"}, // Ventilation machine fresh air out
  {owsensors16,DEVICE_DISCONNECTED, "vent_dirty", "LTO \x7E sis\xE1ilma"},   // Ventilation machine waste air in
  {owsensors17,DEVICE_DISCONNECTED, "vent_waste", "LTO \x7E poistoilma"}     // Ventilation machine waste air out
};


// Structure for the Carrier mode
typedef struct CarrierHeatpump CarrierHeatpump;
struct CarrierHeatpump
{
  int operatingMode;
  int fanSpeed;
  int temperature;
  int humidityILPNotHeating;  // huminity when not heating > 50%;  
  bool fireplaceFan;
  bool heatCable;
};

// Default mode for the heatpump is HEAT, AUTO FAN and 19 degrees.
// 19 degrees also means that 7.5 minutes after startup the command will be
// sent if huminity < 50%  
CarrierHeatpump carrierHeatpump = { 2, 0, 19, false };

// The Carrier heatpump instance, and the IRSender instance
HeatpumpIR *heatpumpIR = new CarrierHeatpumpIR();
IRSender irSender(IR_PIN); // IR led on Mega digital pin 9

// The number of the displayed sensor
int displayedSensor = 0;

// MAC & IP address for the Ethernet shield
byte macAddress[6] = { 0x02, 0x26, 0x89, 0x00, 0x00, 0xFE};
IPAddress ip(192, 168, 1, 109); // ip(192, 168, 1, 109);

// The timers
Timer timer;

// The amount of House power pulses since the last update to emoncms
volatile int housePowerPulses = 0;

// The amount of Heatpump power pulses since the last update to emoncms
volatile int heatpumpPowerPulses = 0;

// The amount of Heatpump in fan RPM pulses since the last update to emoncms
volatile int heatpumpRpmPulses = 0;

// The amount of water meter pulses since the last update to emoncms
volatile int waterPulses = 0;

int waterPulsesHistory[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 12 minute water use history

// DHT11, MQ-7 & MG811 sensor readings
DHT dht(DHT11_PIN, DHT11);

float DHT11Humidity = 0.0;
float DHT11Temperature = 0.0;
int MQ7COLevel = 0;
float MG811CO2Level = 400;
float MG811Voltage = 0.0;
float lightnessLevel = 0.00;

// Alarm state
int alarmState;
int alarmStateHistory;

// Water heat state
int waterHeatState;
int waterHeatReleyState;
int WaterHeatFissioState;
int waterHeat1000wReleyState;

// Sauna heat state
int saunaHeatState;

// House heat state
int houseHeatState;
int houseHeatState2;
int HouseHeatFissioState;
int HouseHeatFissioState2;

// ILP
float heatpumpCOP_EER = 0.00;
int heatpumpPower;
int heatpumpAirFlowRate;
int heatCOPEEROff = 0;

// LTO Ilto
int LTOefficient = 0;
int LTOefficientIn = 0;
int LTOefficientOut = 0;

// House
float housePower = 0.00;

// Warehouse and elecric car sharge
int WAREHOUSE_RELAY_STATE;

// test
int test;

// Water state
bool waterState;
bool waterLeakState;
bool showerWaterUse;
unsigned long lastWaterPulse = 0;

// HeatpumpPowerPulses
unsigned long lastHeatpumpPowerPulses = 0;

// HeatpumpRpmPulses
unsigned long lastHeatpumpRpmPulses = 0;

// HousePowerPulses
unsigned long lastHousePowerPulses = 0;


void setup()
{
  // Serial initialization
  Serial.begin(9600);
  delay(500);
  Serial.println(F("Starting..."));

  // LCD initialization
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("K\xE1ynnistyy..."); // 'Starting...'
  delay(1000);

  // Pull-up for interrupt pins
  digitalWrite(18, HIGH);
  digitalWrite(19, HIGH);
  digitalWrite(20, HIGH);
  digitalWrite(21, HIGH);

  // Fireplace fan relay
  pinMode(FIREPLACE_FAN_PIN, OUTPUT);

  // Default mode for the Fireplace is OFF
  digitalWrite(FIREPLACE_FAN_PIN, HIGH);  // Fireplace fan to OFF state

 // Water stop relay
  pinMode(WATER_STOP_VALVE_PIN, OUTPUT);

  // Default mode for the Water stop is OFF
  digitalWrite(WATER_STOP_VALVE_PIN, HIGH); // Water stop to OFF state, i.e. water flows
  
   // Relay in the house heat relay 4
  pinMode(HOUSE_HEAT_RELAY_PIN , OUTPUT);

 // Default mode for the house heat relay is OFF
  digitalWrite(HOUSE_HEAT_RELAY_PIN , LOW); // House heat relay to OFF state
  
  // Relay in the house heat drop relay 4
  pinMode(HOUSE_HEAT_RELAY2_PIN , OUTPUT);

 // Default mode for the house heat  relay is OFF
  digitalWrite(HOUSE_HEAT_RELAY2_PIN , LOW); // House heat relay to OFF state 
   
 // Relay in the water heat relay 2
  pinMode(WATER_HEAT_RELAY_PIN, OUTPUT);

 // Default mode for the water heat relay is OFF
  digitalWrite(WATER_HEAT_RELAY_PIN, HIGH); // Water heat relay to OFF state
  
  // Relay in the water heat  relay 1
 // pinMode(WATER_HEAT_1000W_RELAY_PIN, OUTPUT);

 // Default mode for the water heat 1000w relay is OFF
 // digitalWrite(WATER_HEAT_1000W_RELAY_PIN, HIGH); // Water heat 1000w relay to OFF state 

  // Relay in the warehouse
  pinMode(WAREHOUSE_RELAY_PIN, OUTPUT);

  // Default mode for the warehouse and electric car sharge 3*16a relay is ON
  digitalWrite(WAREHOUSE_RELAY_PIN, LOW); // Warehouse relay to ON state
  
  // Relay in the ILP heat cable relay 3
  pinMode(ILP_HEAT_CABLE_RELAY_PIN , OUTPUT);
  
  // Default mode for the ILP heat cable relay is OFF
  digitalWrite(ILP_HEAT_CABLE_RELAY_PIN, HIGH); //ILP relay to OFF state

  // Alarm state is an input pin
  pinMode(ALARM_STATE_PIN, INPUT_PULLUP);
  
  // Water heat state is an input pin
  pinMode(WATER_HEAT_STATE_PIN, INPUT_PULLUP);
  
  // Sauna heat state is an input pin
  pinMode(SAUNA_HEAT_STATE_PIN, INPUT_PULLUP);

  // House Fissio heat state is an input pin
  pinMode(HOUSE_HEAT_FISSIO_STATE_PIN, INPUT_PULLUP);

  // House Fissio heat state is an input pin
  pinMode(HOUSE_HEAT_FISSIO_STATE_2_PIN, INPUT_PULLUP);

  // Water Fissio heat state is an input pin
  pinMode(WATER_HEAT_FISSIO_STATE_PIN, INPUT_PULLUP);


  // waterLeak state is false
  waterLeakState == false;
  showerWaterUse == false;

  // List OneWire devices
  for (int i=0; i < sizeof(owbuses) / sizeof(struct owbus); i++)
  {
    lcd.clear();
    lcd.print(owbuses[i].name);
    lcd.setCursor(0, 1);
    lcd.print("laitteita: "); // 'devices'

    Serial.print(F("VÃ¤ylÃ¤ssÃ¤ ")); // 'In bus'
    Serial.print(owbuses[i].name);
    Serial.print(F(" on ")); // 'there is'
    owbuses[i].owbus.begin();
    owbuses[i].owbus.setWaitForConversion(false);
    int deviceCount = owbuses[i].owbus.getDeviceCount();
    Serial.print(deviceCount);
    Serial.println(" laitetta"); // 'devices'
    lcd.print(deviceCount);
    lcd.print(" kpl");

    delay(1000);
  }

  Serial.println("Initializing Ethernet...");

  // initialize the Ethernet adapter with static IP address
  Ethernet.begin(macAddress, ip);

   delay(1000); // give the Ethernet shield a second to initialize

  lcd.clear();
  lcd.print("IP-osoite");
  lcd.setCursor(0, 1);
  lcd.print(Ethernet.localIP());

  Serial.print("IP address (static): ");
  Serial.println(Ethernet.localIP());

  // Temperatures to be measured immediately
  requestTemperatures();

  // The timed calls
  timer.every(2100, feedWatchdog);                 // every ~ 2.1 seconds
  timer.every(1650, updateDisplay);                // every ~ 1.65 seconds
  timer.every(2000, alarmWaterShutoff);            // every 2 seconds
  timer.every(60000, readSensors);                 // every minute
  timer.every(60000, updateEmoncms);               // every minute
  timer.every(252000, controlHouse);               // every 4.2 minutes
  timer.every(60000, checkForWaterShutoff);        // every minute
  timer.every(15000, requestTemperatures);         // every 15 seconds
  timer.every(920000, controlCarrier);             // every ~ 15.333333. minutes  7.50028 minute = 450017ms =  oli 5.50028 minute = 330017ms

 // Heatpump rpm pulse counter interrupt
  // interrupt 2 uses pin 21
  attachInterrupt(2, incrementheatpumpRpmPulses, FALLING);
  
  // Water meter pulse counter interrupt
  // interrupt 3 uses pin 20
  attachInterrupt(3, incrementwaterPulses, FALLING);
  
  // Heatpump power pulse counter interrupt
  // interrupt 4 uses pin 19
  attachInterrupt(4, incrementheatpumpPowerPulses, FALLING);
   
  // House pulse counter interrupt
  // interrupt 5 uses pin 18
  attachInterrupt(5, incrementhousePowerPulses, FALLING);


  // Initialize the DHT library
  dht.begin();

  // Enable watchdog
   wdt_enable(WDTO_8S);
}

void loop()
{
  timer.update();

  // Alarm state is not an interrupt
  alarmState = digitalRead(ALARM_STATE_PIN);
  
  // Water heat state is not an interrupt
  waterHeatState = digitalRead(WATER_HEAT_STATE_PIN);
  
   // Sauna heat state is not an interrupt
  saunaHeatState = digitalRead(SAUNA_HEAT_STATE_PIN);

   // House Fissio heat state is not an interrupt
  HouseHeatFissioState = digitalRead(HOUSE_HEAT_FISSIO_STATE_PIN);

   // House Fissio heat state is not an interrupt
  HouseHeatFissioState2 = digitalRead(HOUSE_HEAT_FISSIO_STATE_2_PIN);

   // Water Fissio heat state is not an interrupt
  WaterHeatFissioState = digitalRead(WATER_HEAT_FISSIO_STATE_PIN);
  
  
}

// Request the temperature measurement on all 1-wire buses
// and schedule an event 750ms later to read the measurements
void requestTemperatures()
{
  for (int i=0; i < sizeof(owbuses) / sizeof(struct owbus); i++) {
    owbuses[i].owbus.requestTemperatures();
  }

  timer.after(750, readTemperatures);
}

// Read the measured temperatures
void readTemperatures()
{
  for (int i=0; i < sizeof(owbuses) / sizeof(struct owbus); i++) {
    float temperature = owbuses[i].owbus.getTempCByIndex(0);
    if (temperature != DEVICE_DISCONNECTED) {
      owbuses[i].temperature = owbuses[i].owbus.getTempCByIndex(0);
    }
  }
}

// Update the LCD display
void updateDisplay()
{
  // First show the temperature displays
  if ( displayedSensor < sizeof(owbuses) / sizeof(struct owbus) &&
       owbuses[displayedSensor].temperature != DEVICE_DISCONNECTED) {
    // Display the device name and temperature on the LCD
    lcd.clear();
    lcd.print(owbuses[displayedSensor].name);
    lcd.setCursor(0, 1);
    lcd.print(owbuses[displayedSensor].temperature);
    lcd.print(" \xDF""C");

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 1)) {
    // House Power level
    lcd.clear();
    lcd.print("Talon    s\xE1hk\xEFn-");
    lcd.setCursor(0, 1);
    lcd.print(housePower);
    lcd.print(" kW");
    lcd.setCursor(9, 1);
    lcd.print("k\xE1ytt\xEF");
   
    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 2)) {
    // Ventilation machine waste air in sensor level
    lcd.clear();
    lcd.print("Sis\xE1ilma \x7E LTO");

    lcd.setCursor(0, 1);
    lcd.print(DHT11Temperature);
    lcd.print(" \xDF""C");

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 3)) {
    // Humidity sensor level
    lcd.clear();
    lcd.print("Ilmankosteus");
    lcd.setCursor(0, 1);
    lcd.print(DHT11Humidity);
    lcd.print(" %  Sis\xE1ll\xE1");

     // And the same on debug display
    Serial.print(owbuses[displayedSensor].name);
    Serial.print(F(": "));
    Serial.println(owbuses[displayedSensor].temperature);
    displayedSensor++;

  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 4)) {
    // Heatpump temperature difference
    Serial.print("ILP lÃ¤mpenemÃ¤: ");
    Serial.println(owbuses[9].temperature - owbuses[8].temperature);

    lcd.clear();
    if (carrierHeatpump.operatingMode == MODE_HEAT)
    {
      lcd.print("ILP l\xE1mmitt\xE1\xE1:");
      lcd.setCursor(0, 1);
      lcd.print(owbuses[9].temperature - owbuses[8].temperature);
      lcd.print(" \xDF""C");
    } else if (carrierHeatpump.operatingMode == MODE_COOL) {
      lcd.print("ILP j\xE1\xE1hdytt\xE1\xE1:");
      lcd.setCursor(0, 1);
      lcd.print(owbuses[8].temperature - owbuses[9].temperature);
      lcd.print(" \xDF""C");
    } else {
      lcd.print("ILP puhaltaa:");
      lcd.setCursor(0, 1);
      lcd.print(owbuses[9].temperature);
      lcd.print(" \xDF""C");
    }
    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 5)) {
    // Mode display
    Serial.print("MODE: ");
    Serial.println(carrierHeatpump.operatingMode);
    Serial.print("FAN: ");
    Serial.println(carrierHeatpump.fanSpeed);
    Serial.print("TEMP: ");
    Serial.println(carrierHeatpump.temperature);

    lcd.clear();
    lcd.print("MODE: ");
    switch (carrierHeatpump.operatingMode) {
      case MODE_COOL:
        lcd.print("COOL");
        break;
      case MODE_HEAT:
        lcd.print("HEAT");
        break;
      case MODE_FAN:
        lcd.print("FAN");
        break;
    }

    lcd.setCursor(0, 1);
    lcd.print("TEMP: ");
    lcd.print(carrierHeatpump.temperature);

    lcd.setCursor(10, 1);
    lcd.print("FAN: ");
    if (carrierHeatpump.fanSpeed == FAN_AUTO) {
      lcd.print("A");
    } else {
      lcd.print(carrierHeatpump.fanSpeed);
      
       }
       
    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 6)) {

    // COP-EER level
    lcd.clear();
    lcd.print("ILP COP-EER ");
    lcd.setCursor(0, 1);
    lcd.print(heatpumpCOP_EER);
    //lcd.print("  ");
    
    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 7)) {
    // Heatpump Power level
    lcd.clear();
    lcd.print("ILP s\xE1hk\xEFnk\xE1ytt\xEF");
    lcd.setCursor(0, 1);
    lcd.print(heatpumpPower);
    lcd.print(" w");
    
    
       displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 8)) {
    // Heatpump Air Flow  level
    lcd.clear();
    lcd.print("ILP ilmanvirta ");
    lcd.setCursor(0, 1);
    lcd.print(heatpumpAirFlowRate);
    lcd.print(" m3/h");
    
      
    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 9)) {
    // Fireplace mode display
    lcd.clear();
    lcd.print("Takkapuhallin");

    if (carrierHeatpump.fireplaceFan) {
      lcd.setCursor(0, 1);
      lcd.print("ON");
      Serial.println("Fireplace fan: ON");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("OFF");
      Serial.println("Fireplace fan: OFF");
    }
    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 10)) {
    // CO2 level
    lcd.clear();
    lcd.print("CO-taso");
    lcd.setCursor(0, 1);
    lcd.print(MQ7COLevel);
    lcd.print(" ppm");
    
    // lcd.print(" ppm"); // This is certainly not ppm's. I don't know what unit this number stands for

   displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 11)) {
    // CO2 level
    lcd.clear();
    lcd.print("CO2-taso ");
    lcd.print(MG811Voltage);
    lcd.print(" V");
    lcd.setCursor(0, 1);
    lcd.print(MG811CO2Level);
    lcd.print(" ppm");

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 12)) {
    // water state mode display
    lcd.clear();
    lcd.print("Vedensulku");
    lcd.setCursor(0, 1);
    if (waterState == LOW) {
      lcd.print("ON");
    } else {
      lcd.print("OFF");
    }

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 13)) {
    // water Leak State mode display
    lcd.clear();
    lcd.print("Vuototesti");
    lcd.setCursor(0, 1);
    if (waterLeakState == true) {
      lcd.print("Vesivuoto");
    } else {
      lcd.print("OK");
    }

     displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 14)) {
    // water Leak State mode display
    lcd.clear();
    lcd.print("Suikunk\xE1ytt\xEF");
    lcd.setCursor(0, 1);
    if (showerWaterUse == true) {
      lcd.print("yli 12 min");
    } else {
      lcd.print("OK");
    }

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 15)) {
    // alarm State mode display
    lcd.clear();
    lcd.print("h\xE1lytin");
    lcd.setCursor(0, 1);
    if (alarmState == LOW) {
      lcd.print("ON");
    } else {
      lcd.print("OFF");
    }

  displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 16)) {
    // heat State mode display
    lcd.clear();
    lcd.print("l\xE1mmitys V");
    lcd.setCursor(13, 0);
    if (houseHeatState == LOW) {
      lcd.print("OFF");
    } else { 
      lcd.print("ON");
      }
      lcd.setCursor(0, 1);
      lcd.print("l\xE1mmitys L");
      lcd.setCursor(13, 1);
    if (houseHeatState2 == LOW) {
      lcd.print("OFF");
    } else {
      lcd.print("ON");
    }

displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 18)) {
    // sauna heat State mode display
    lcd.clear();
    lcd.print("saunan l\xE1mmitys");
    lcd.setCursor(0, 1);
    if (saunaHeatState == LOW) {
      lcd.print("ON");
    } else {
      lcd.print("OFF");
    }

 displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 19)) {
    // water heat State mode display
    lcd.clear();
    lcd.print("veden l\xE1mmitys");
    lcd.setCursor(0, 1);
    if (waterHeatState == LOW) {
      lcd.print("ON");
    } else {
      lcd.print("OFF");
    }

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 20)) {

    // waterPulsesHistory
    lcd.clear();
    lcd.print("Vedenkulutus");
    lcd.setCursor(0, 1);
    lcd.print(waterPulses);
    lcd.print(" Litraa ");

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 21)) {
    lcd.clear();
    lcd.print("Vedenk\xE1ytt\xEF");
    lcd.setCursor(0, 1);
    lcd.print("historia 12 min");

    displayedSensor++;
  } else if (displayedSensor < (sizeof(owbuses) / sizeof(struct owbus) + 22)) {

    lcd.clear();
    lcd.setCursor(0, 0);

    // Log the waterPulsesHistory[0-6] readings
    for (int i=0; i < sizeof(waterPulsesHistory) / sizeof(int)-6; i++) {
      lcd.print(waterPulsesHistory[i]);
      if (i < 5)
        lcd.print(",");
      else {
      lcd.print(" Lit");
      }
    }
    lcd.setCursor(0, 1);

    // Log the waterPulsesHistory[6-12] readings
    for (int i=6; i < sizeof(waterPulsesHistory) / sizeof(int); i++) {
      lcd.print(waterPulsesHistory[i]);
      if (i < 11)
        lcd.print(",");
      else {
      lcd.print(" raa");
      }
    }
    
    
    
    displayedSensor = 0;
  }
}
// Decide what to do with the Carrier heatpump

void controlCarrier()

{
  Serial.println("Controlling Carrier");


  int operatingMode = 2;
  int fanSpeed = 0;
  int temperature = 19;

  int aircond_intake = owbuses[7].temperature;
  int outdoor = owbuses[14].temperature;
  int fireplace = owbuses[0].temperature;
  int utility = owbuses[2].temperature;
  int kitchen = owbuses[1].temperature;
  int humidity = DHT11Humidity;
  int humidityILPNotHeat  = 50;  // huminity ilp not heat > 50%
  
  Serial.println("aircond_intake");
  Serial.println(aircond_intake);
  Serial.println("outdoor");
  Serial.println(outdoor);
  Serial.println("fireplace");
  Serial.println(fireplace);
  Serial.println("utility");
  Serial.println(utility);
  Serial.println("kitchen");
  Serial.println(kitchen);
  Serial.println("humidity");
  Serial.println(humidity);
  Serial.println("humidityILPNotHeat");
  Serial.println(humidityILPNotHeat);
  
  heatCOPEEROff = 0;
  
   
  // Fireplace fan control

  if (fireplace < FireplaceFanOff) {
    digitalWrite(FIREPLACE_FAN_PIN, HIGH);  // Fireplace fan to OFF state
    Serial.println("Takkapuhallin pois");
    carrierHeatpump.fireplaceFan = false;
  } else if (fireplace >=FireplaceFanOn) {
    digitalWrite(FIREPLACE_FAN_PIN, LOW);   // Fireplace fan to ON state
    Serial.println("Takkapuhallin pÃ¤Ã¤lle");
    carrierHeatpump.fireplaceFan = true;
  }
  

  // Heatpump control
  
  // Set the mode based on the outdoor temperature and COP-EER
  //operatingMode = MODE_HEAT;
  //fanSpeed = FAN_AUTO;
  
    if (heatpumpCOP_EER >= 5.0 && outdoor >= 15 && outdoor < 20) { 
    heatCOPEEROff = 1;  
  } else if (heatpumpCOP_EER >= 4.5 && outdoor >= 10   && outdoor < 15) { 
    heatCOPEEROff = 2;  
  } else if (heatpumpCOP_EER >= 4.0 && outdoor >= 5   && outdoor < 10)  {  
    heatCOPEEROff = 3;
  } else if (heatpumpCOP_EER >= 3.5 && outdoor >= 0   && outdoor < 5)   { 
    heatCOPEEROff = 4;  
  } else if (heatpumpCOP_EER >= 3.0 && outdoor >= -4  && outdoor < 0)   { 
    heatCOPEEROff = 5; 
  } else if (heatpumpCOP_EER >= 2.5 && outdoor >= -9  && outdoor < -4)  { 
    heatCOPEEROff = 6;
  } else if (heatpumpCOP_EER >= 2.3 && outdoor >= -14 && outdoor < -9)  { 
    heatCOPEEROff = 7;
  } else if (heatpumpCOP_EER >= 2.1 && outdoor >= -19 && outdoor < -14) { 
    heatCOPEEROff = 8;
  } else if (heatpumpCOP_EER >= 1.8 && outdoor >= -24 && outdoor < -19) {
    heatCOPEEROff = 9;
  } else if (heatpumpCOP_EER >= 1.6 && outdoor >= -29 && outdoor < -24) {
    heatCOPEEROff = 10;
  } else if (heatpumpCOP_EER >= 1.5 && outdoor >= -35 && outdoor < -29) { 
    heatCOPEEROff = 11;  
  } else if (heatpumpCOP_EER >= 1.2 && outdoor >= -45 && outdoor < -35) {
    heatCOPEEROff = 12;  
  }  
    
     // Heatpump control
  // Set the mode based on the outdoor temperature (summer cooling)
   if (outdoor >= 20 &&  aircond_intake >= 23) {
    operatingMode = MODE_COOL;
    temperature = 25;
    fanSpeed = FAN_AUTO;

    
    if (outdoor >= 22 && outdoor <= 23 && aircond_intake >= 23) {
      temperature = 24;
    } else if (outdoor >= 24 && outdoor <= 25 && aircond_intake >= 23) {
      temperature = 23;  
    } else if (outdoor >= 26 && outdoor <= 27 && aircond_intake >= 23) {
      temperature = 22;  
    } else if (outdoor >= 28 && outdoor <= 29 && aircond_intake >= 23) {
      temperature = 21;    
    } else if (outdoor >= 30 && aircond_intake >= 23) {
      temperature = 20;    
    }
  // Fireplace is hot, use the FAN mode
  } else if (fireplace >= 25) {
    // Default to MODE_FAN with FAN 1
    operatingMode = MODE_FAN;
    temperature = 22;
    fanSpeed = FAN_1;

    if (fireplace >= 26 && fireplace < 28) {
      fanSpeed = FAN_2;
    } else if (fireplace >= 28 && fireplace < 30) {
      fanSpeed = FAN_3;
    } else if (fireplace >= 30 && fireplace < 33) {
      fanSpeed = FAN_4;
    } else if ( fireplace >= 33) {
      fanSpeed = FAN_5;
    }
  // Utility room is hot, as the laundry drier has been running
  } else if ( utility >= 23) { 
    // Default to MODE_FAN with FAN 1
    operatingMode = MODE_FAN;
    temperature = 22;
    fanSpeed = FAN_1;

    if (utility >= 24 && utility < 25) { 
      fanSpeed = FAN_2;
    } else if ( utility >= 25 && utility < 26) {
      fanSpeed = FAN_3;
    } else if ( utility >= 26 && utility < 27) {
      fanSpeed = FAN_4;
    } else if ( utility >= 26 && utility < 27 && outdoor < 20) {
      fanSpeed = FAN_5;

    } else if (utility >= 27 && outdoor >= 20) {
      // COOL with AUTO FAN, +24
      operatingMode = MODE_COOL;
      temperature = 24;
      fanSpeed = FAN_AUTO;
    }

  // Kitchen is hot, as the oven has been running
  } else if ( kitchen >= 23) { 
    // Default to MODE_FAN with FAN 1
    operatingMode = MODE_FAN;
    temperature = 22;
    fanSpeed = FAN_1;

    if (kitchen >= 24 && kitchen < 25) { 
      fanSpeed = FAN_2;
    } else if ( kitchen >= 25 && kitchen < 26) {
      fanSpeed = FAN_3;
    } else if ( kitchen >= 26 && kitchen < 27) {
      fanSpeed = FAN_4;
    } else if ( kitchen >= 27 && kitchen < 28 && outdoor < 20) {
      fanSpeed = FAN_5;

    } else if (kitchen >= 27 && outdoor >= 20) {
      // COOL with AUTO FAN, +24
      operatingMode = MODE_COOL;
      temperature = 24;
      fanSpeed = FAN_AUTO;
    }

  // Fireplace or utility or kitchen room is not hot
  } else if (outdoor >= 21 && outdoor < 22) {
    // MODE_FAN with FAN_AUTO disable COOL and HEAT replacement all the time
    operatingMode = MODE_FAN;
    temperature = 22;
    fanSpeed = FAN_AUTO;

  // Set the mode based on the outdoor temperature (heating)
  } else if (humidity < 50 ) {
    // FAN with FAN 1 temp+22
    operatingMode = MODE_HEAT;
    temperature = 20;
    fanSpeed = FAN_AUTO;

    if ((outdoor >= 15 && outdoor < 20) && humidity < humidityILPNotHeat ) {
      temperature = 21;
    } else if ((outdoor >= 5 && outdoor < 15) && humidity < humidityILPNotHeat ) {
      temperature = 22;
    } else if ((outdoor >= 0 && outdoor < 5) && humidity < humidityILPNotHeat ){ // -1~500w 0~750w 1~1150w  2~1500w
      temperature = 23;
    } else if ((outdoor >= -4 && outdoor < 0) && humidity < humidityILPNotHeat ) {
      temperature = 24;
    } else if ((outdoor >= -9 && outdoor < -4) && humidity < humidityILPNotHeat ) {
      temperature = 25;
    } else if ((outdoor >= -14 && outdoor < -9) && humidity < humidityILPNotHeat ) {
      temperature = 26;
    } else if ((outdoor >= -19 && outdoor < -14) && humidity < humidityILPNotHeat ) {
      temperature = 27;
    } else if ((outdoor >= -24 && outdoor < -19) && humidity < humidityILPNotHeat ) {
      temperature = 28;
    } else if ((outdoor >= -29 && outdoor < -24) && humidity < humidityILPNotHeat ) {
      temperature = 29;
    } else if ((outdoor >= -35 && outdoor < -29) && humidity < humidityILPNotHeat ) {
      temperature = 30;
    } else if ((outdoor >= -45 && outdoor < -35) && humidity < humidityILPNotHeat ) {
      temperature = 30;
    }
    
    } else {
    // MODE_FAN with FAN_AUTO if nothing else match
    operatingMode = MODE_FAN;
    temperature = 22;
    fanSpeed = FAN_AUTO;
    
   } 
 if (heatpumpAirFlowRate  < 100 && humidity < humidityILPNotHeat ){
    operatingMode = carrierHeatpump.operatingMode;
    fanSpeed = carrierHeatpump.fanSpeed;
    temperature = carrierHeatpump.temperature;
     // ILP heat cable control
     
   }  
 if (heatCOPEEROff != 0 && humidity < humidityILPNotHeat ){
    operatingMode = carrierHeatpump.operatingMode;
    fanSpeed = carrierHeatpump.fanSpeed;
    temperature = carrierHeatpump.temperature;
     // ILP heat cable control
     
   } 

  
  // Did any of the values change? If so, send an IR command
  if (operatingMode != carrierHeatpump.operatingMode ||
      fanSpeed != carrierHeatpump.fanSpeed ||
      temperature != carrierHeatpump.temperature)
  {
    Serial.println("Sending Carrier command");
    Serial.print("MODE: ");
    Serial.println(operatingMode);
    Serial.print("FAN: ");
    Serial.println(fanSpeed);
    Serial.print("TEMP: ");
    Serial.println(temperature);


    // Save the state
    carrierHeatpump.operatingMode = operatingMode;
    carrierHeatpump.fanSpeed = fanSpeed;
    carrierHeatpump.temperature = temperature;

    // Send the IR command
    heatpumpIR->send(irSender, POWER_ON, carrierHeatpump.operatingMode, carrierHeatpump.fanSpeed, carrierHeatpump.temperature, VDIR_MANUAL, HDIR_MANUAL);
  
  }
  
  if (outdoor > ILPHeatCableControlOff || operatingMode != MODE_HEAT || heatpumpCOP_EER > 2){
    digitalWrite(ILP_HEAT_CABLE_RELAY_PIN, HIGH);  // ILP heat cable to OFF state
    Serial.println("ILP heat cable pois");
    carrierHeatpump.heatCable = false;
  } else if (outdoor <= ILPHeatCableControlOn && operatingMode == MODE_HEAT && heatpumpCOP_EER <= 2.5){
    digitalWrite(ILP_HEAT_CABLE_RELAY_PIN, LOW);   // ILP heat cable to ON state
    Serial.println("ILP heat cable pÃ¤Ã¤lle");
    carrierHeatpump.heatCable = true;
   
  }
}

//
// Report measurement data to emoncms.org
//
void updateEmoncms() {
  

  EthernetClient client;
  boolean notFirst = false;
  int emonWaterPulses = 0;
  int emonHeatpumpPowerPulses = 0;
  int emonHeatpumpRpmPulses = 0;
  int emonHousePowerPulses = 0;
  //int HeatCopEerOff;

  // Interrupts need to be disabled while the pulse counter is read or modified...
  noInterrupts();
  emonWaterPulses = waterPulses;
  waterPulses = 0;
  emonHeatpumpPowerPulses = heatpumpPowerPulses;
  heatpumpPowerPulses = 0;
  emonHeatpumpRpmPulses = heatpumpRpmPulses;
  heatpumpRpmPulses = 0;
  emonHousePowerPulses = housePowerPulses;
  housePowerPulses = 0;
  interrupts();
  
 
 //void incrementHeatpumpCOP_EER()
 {
   
  heatpumpAirFlowRate = (emonHeatpumpRpmPulses*0.668-199.5);
  heatpumpPower = (emonHeatpumpPowerPulses*60);
  
  housePower = (emonHousePowerPulses*60*0.001);
  
if  (heatpumpAirFlowRate  < 0 ){
  heatpumpAirFlowRate = 0;
  }
// RAFUn estimaattikaavalla ilmantiheydelle [kg/m3]
// 1,301-0,00525*T+0,000023*T*T, jossa T = puhallusilman lämpötila

//antotehon kaava [W]:
// ((Tpuh-Timu)*qv,puh*(1,301-0,00525*Tpuh+0,000023*Tpuh*Tpuh)*1,005/3,6)

  if (owbuses[8].temperature-owbuses[7].temperature >= 0 ) {
  // Lämmitys COP 
  heatpumpCOP_EER = ((owbuses[8].temperature-owbuses[7].temperature)*(emonHeatpumpRpmPulses*0.668-199.5)*(1.301-0.00525*owbuses[8].temperature+0.000023*owbuses[8].temperature*owbuses[8].temperature)*1.005/3.6) / (emonHeatpumpPowerPulses*60);
    
} else if (owbuses[8].temperature-owbuses[7].temperature < 0 ) {
  // Jäähdytys EER
  heatpumpCOP_EER = ((owbuses[7].temperature-owbuses[8].temperature)*(emonHeatpumpRpmPulses*0.668-199.5)*(1.301-0.00525*owbuses[8].temperature+0.000023*owbuses[8].temperature*owbuses[8].temperature)*1.005/3.6) / (emonHeatpumpPowerPulses*60);
}
  
  if (heatpumpPower <= 60) {
  heatpumpCOP_EER = 0;
} else if (carrierHeatpump.operatingMode == 4 || carrierHeatpump.operatingMode == 5 || carrierHeatpump.operatingMode == 6) {
  heatpumpCOP_EER = 0;
} else if (heatpumpCOP_EER < -1 ) {
  heatpumpCOP_EER = -1;
} else if (heatpumpCOP_EER > 10) {
  heatpumpCOP_EER = 10;
}
 
// Tpuh = puhallusilman lämpötila [C]
// Timu = imuilman lämpötila [C]
// qv,puh = puhallusilman määrä [m3/h]
}

  // Update the water use history
  for (byte i=((sizeof(waterPulsesHistory) / sizeof(int)) - 1); i > 0; i--) {
    waterPulsesHistory[i] = waterPulsesHistory[i-1];
  }
  waterPulsesHistory[0] = emonWaterPulses;

  // Report the data into emoncms.org
  Serial.println("Connecting to emoncms.org...");

  feedWatchdog();

  if (client.connect("192.168.1.144", 80)) {
    // send the HTTP GET request:
    client.print("GET http://192.168.1.144/emoncms/input/post.json?apikey=");
    client.print(EMONCMS_APIKEY);
    client.print("&json={");

    // Log sensor temperatures
    for (int i=0; i < sizeof(owbuses) / sizeof(struct owbus); i++) {
      if (owbuses[i].temperature != DEVICE_DISCONNECTED) {
        if (notFirst) {
          client.print(",");
        }
        notFirst = true;

        client.print(owbuses[i].emon_name);
        client.print(":");
        client.print(owbuses[i].temperature);
      }
    }

    // Log heatpump state
    client.print(",carrierHeatpump.temperature:");
    client.print(carrierHeatpump.temperature);
    client.print(",carrierHeatpump.operatingMode:"); // heatpump_mode
    client.print(carrierHeatpump.operatingMode);
    client.print(",carrierHeatpump.fanSpeed:");
    client.print(carrierHeatpump.fanSpeed);
    client.print(",fireplace_fan:");
    if (carrierHeatpump.fireplaceFan == true) {
      client.print("0");
    } else {
      client.print("1");
    }
    client.print(",carrier_heatpump_heat_cable:");
    if (carrierHeatpump.heatCable == true) {
      client.print("0");
    } else {
      client.print("1");
    }
    
// Log the water meter pulses
    client.print(",water_pulses:");
    client.print(emonWaterPulses);
    client.print(",water_pulses_2:");
    client.print(emonWaterPulses);
    client.print(",water_pulses_3:");
    client.print(emonWaterPulses);
    client.print(",water_pulses_4:");
    client.print(emonWaterPulses);
    client.print(",water_pulses_5:");
    client.print(emonWaterPulses);
    // Log the Heatpump COP / EER
    client.print(",Heatpump_COP_EER:");
    client.print(heatpumpCOP_EER);
    // Log the Heatpump Power W
    client.print(",Heatpump_Power:");
    client.print(heatpumpPower);
    // Log the Heatpump Air Flow Rate
    client.print(",Heatpump_Air_Flow_Rate:");
    client.print(heatpumpAirFlowRate);
    // Log the Heatpump power pulses
    client.print(",Heatpump_Power_pulses:");
    client.print(emonHeatpumpPowerPulses);
    // Log the Heatpump rpm pulses
    client.print(",Heatpump_Rpm_pulses:");
    client.print(emonHeatpumpRpmPulses);
    // Log the House power pulses
    client.print(",House_Power_pulses:");
    client.print(emonHousePowerPulses);
    // Log the House power pulses
    client.print(",House_Power_pulses2:");
    client.print(emonHousePowerPulses);
    // Log the House power kW
    client.print(",House_Power_kW:");
    client.print(housePower);
    // Log the DHT11 readings
    client.print(",dht11_humidity:");
    client.print(DHT11Humidity);
    client.print(",dht11_temperature:");
    client.print(DHT11Temperature);
    // Log the LTOefficient readings
    client.print(",LTOefficient:");
    client.print(LTOefficient);
    // Log the LTOefficientIn readings
    client.print(",LTOefficientIn:");
    client.print(LTOefficientIn);
    // Log the LTOefficientOut readings
    client.print(",LTOefficientOut:");
    client.print(LTOefficientOut);
    // Log the MQ7 readings
    client.print(",mq7_colevel:");
    client.print(MQ7COLevel);
    // Log the MG811 readings
    client.print(",mg811_co2level:");
    client.print(MG811CO2Level);
    // Log the MG811 readings
    client.print(",mg811_Voltage:");
    client.print(MG811Voltage);
    // Log the Heat Cop-Eer Off = 1;
    client.print(",Heat_Cop_Eer_Off:");
    client.print(heatCOPEEROff);
     // Log the LightnessLevel;
    client.print(",Lightness_Level:");
    client.print(lightnessLevel);
    

    // Log the waterPulsesHistory
    for (byte i=((sizeof(waterPulsesHistory) / sizeof(int)) - 1); i > 0; i--) {
      client.print(",waterPulsesHistory[");
      client.print(i);
      client.print("]:");
      client.print(waterPulsesHistory[i]);
    }

    // Log the alarm state
    client.print(",alarm_state:");
    if ( alarmState == LOW ) {
      client.print("0");
    } else {
      client.print("1");
    }
     // Log the water state
    client.print(",water_state:");
    if ( waterState == LOW ) {
      client.print("0");
    } else {
      client.print("1");
    }
    // Log the water heat state
    client.print(",water_heat_state:");
    if ( waterHeatState == HIGH ) {
      client.print("0");
    } else {
      client.print("1");
    }
    // Log the water heat state
    client.print(",water_heat_on:");
    if ( waterHeatState == HIGH ) {
      client.print("1");
    } else {
      client.print("0");
    }
     // Log the sauna heat state
    client.print(",sauna_heat_state:");
    if ( saunaHeatState == HIGH ) {
      client.print("0");
    } else {
      client.print("1");
    }
    // Log the House Fissio heat drop state
    client.print(",House_Heat_Fissio_State:");
    if ( HouseHeatFissioState == HIGH ) {
      client.print("1");
    } else {
      client.print("0");
    }
    // Log the House Fissio heat drop state2
    client.print(",House_Heat_Fissio_State2:");
    if ( HouseHeatFissioState2 == HIGH ) {
      client.print("1");
    } else {
      client.print("0");
    }
    // Log the Water Fissio heat state
    client.print(",Water_Heat_Fissio_State:");
    if ( WaterHeatFissioState == HIGH ) {
      client.print("1");
    } else {
      client.print("0");
    }
     // Log the sauna heat state
    client.print(",sauna_heat_on:");
    if ( saunaHeatState == HIGH ) {
      client.print("1");
    } else {
      client.print("0");
    }
     // Log the Warehouse Relay state
    client.print(",Warehouse_Relay_State:");
    if ( WAREHOUSE_RELAY_STATE == true ) {
      client.print("0");
    } else {
      client.print("1");
    }
    // Log the leak state
    client.print(",waterLeak_state:");
    if ( waterLeakState == true ) {
      client.print("0");
    } else {
      client.print("1");
    }
    // Log the leak state
    client.print(",showerWaterUse_state:");
    if ( showerWaterUse == true ) {
      client.print("0");
    } else {
      client.print("1");
    } 
    // houseHeatDropState
    client.print(",house_heat_state:");
    if ( houseHeatState  == true) {
      client.print("0");
    } else {
      client.print("1");
    }  
    // houseHeatDropState2
    client.print(",house_heat_state2:");
    if ( houseHeatState2  == true) {
      client.print("0");
    } else {
      client.print("1");
    } 
    // waterHeatReleyState
    client.print(",water_heat_reley_state:");
    if ( waterHeatReleyState  == true) {
      client.print("0");
    } else {
      client.print("1");
  //  } 
    // void waterHeat1000w
  //    client.print(", void waterHeat1000w :");
  //  if ( void waterHeat1000w  == true) {
  //    client.print("0");
  //  } else {
  //    client.print("1");
    }

   // feedWatchdog();
    
    client.println("} HTTP/1.1");
    client.println("Host: 192.168.0.15");
    client.println("User-Agent: Arduino-ethernet");
    client.println("Connection: close");
    client.println();

    Serial.println(F("\nemoncms.org response:\n---"));
    while (client.connected()) {
      while (client.available()) {
        char c = client.read();
        Serial.print(c);
      }
    }

    Serial.println();
    client.stop();
  }
}

void readSensors() {

  
  readDHT11();
  readMQ7();
  readMG811();
  readLIGHTNESS();
}

//
// Read the DHT11 humidity & temperature sensor
//
void readDHT11() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (!isnan(DHT11Humidity)) {
    DHT11Humidity = humidity;
  }
  if (!isnan(DHT11Temperature)) {    
    DHT11Temperature = temperature;
  }

  // Calculate the LTOefficient 

  //int vent_dirty = owbuses[16].temperature; // sisäilma
  //int vent_waste_house = DHT11Temperature;
  //int vent_outdoor = owbuses[14].temperature;
  //int vent_waste_out = owbuses[17].temperature;

  LTOefficient= ((DHT11Temperature-owbuses[17].temperature)/(DHT11Temperature-owbuses[14].temperature))*100; //
  
  // Calculate the LTOefficientIn 

  LTOefficientIn= ((owbuses[16].temperature-owbuses[14].temperature)/(DHT11Temperature-owbuses[14].temperature))*100; //

  // Calculate the LTOefficientOut 

  LTOefficientOut= ((DHT11Temperature-owbuses[17].temperature)/(DHT11Temperature-owbuses[14].temperature))*100; //

}

//
// Read the MQ-7 CO sensor, see http://www.dfrobot.com/wiki/index.php?title=Carbon_Monoxide_Sensor(MQ7)_(SKU:SEN0132)
//
void readMQ7() {
  MQ7COLevel = analogRead(MQ7_PIN); //FREE AIR 51 18.05.2014
}


//
// Read the MG811 CO2 sensor, see http://www.veetech.org.uk/Prototype_CO2_Monitor.htm
//
void readMG811() {
  // Sensor Calibration Constants
  const float v400ppm = 4.70;   //MUST BE SET ACCORDING TO CALIBRATION -> FREE AIR 2.85  USE 1.11 16.05.2015
  const float v40000ppm = 1.85; //MUST BE SET ACCORDING TO CALIBRATION -> FREE AIR 1.87  USE 0.40 16.05.2015
  const float deltavs = v400ppm - v40000ppm;
  const float A = deltavs/(log10(400) - log10(40000));
  const float B = log10(400);

  // Read co2 data from sensor
  int data = analogRead(MG811_PIN); //digitise output from c02 sensor
  MG811Voltage = data/204.6;        //convert output to voltage

  // Calculate co2 from log10 formula (see sensor datasheet)
  float power = ((MG811Voltage - v400ppm)/A) + B;
  MG811CO2Level = pow(10,power);
}

// Read the LIGHTNESS sensor

void readLIGHTNESS() {
  float lightness = analogRead(LIGHTNESS_PIN); //
  lightnessLevel = 1023-lightness;
}

//
// Water use checks
//
void checkForWaterShutoff() {
  checkForWaterUse();
  checkForShowerWaterUse();
  checkForwaterLeak();


}

//
// Check for excessive water use
//
void checkForWaterUse() {

  for (byte i=0; i < (sizeof(waterPulsesHistory) / sizeof(int))-6; i++) { // 12-6=6 MITTAUSTA
    // If all samples are 1, shut off water
    if ( waterPulsesHistory[i] == 1 ) { // 1
      continue;
    }
    // If recent samples are 13-14 or over, shut off water
    else if (waterPulsesHistory[0] >= 13 && // 13
             waterPulsesHistory[1] >= 14 && // 14
             waterPulsesHistory[2] >= 13) { // 13
      continue;
        
    } else {
      return;
    }
  }
 // Water leak - shut off water
  digitalWrite(WATER_STOP_VALVE_PIN, LOW);
  waterState = LOW;
  waterLeakState = true;
  Serial.println("Water leak True");
}

//
// Check for excessive shower water use
//
void checkForShowerWaterUse() {
}

//
// Check if the water valve needs to be shut due to a leak - same amount of water use for 12 minutes
//
void checkForwaterLeak() {

  int firstWaterPulse = waterPulsesHistory[0];
  
   for (byte i=0; i < (sizeof(waterPulsesHistory) / sizeof(int)); i++) {
    // It's not a leak if water isn't flowing
    if ( waterPulsesHistory[i] == 0 ) {
      return;
     }  
     for (byte i=0; i < (sizeof(waterPulsesHistory) / sizeof(int))-ShowerWaterUseCutMin; i++) { // 12-6=6 MITTAUSTA
    // If all samples are 10, shut off water
     if ( waterPulsesHistory[i] == ShowerWaterUse ) {
      return;
      }    
    // If all samples are within +-3 of the first sample, shut off water
    else if ( waterPulsesHistory[i] >= firstWaterPulse-3 && waterPulsesHistory[i] <= firstWaterPulse+3 ) {
      continue;
    
    } else {
      return;
    }
  }
  }
  // Water leak - shut off water
  digitalWrite(WATER_STOP_VALVE_PIN, LOW);
  waterState = LOW;
  showerWaterUse = true;
  Serial.println("Water leak True");
}

//
// Check if the water valve needs to be open or shut, based on the alarm state
// When the alarm turns off, water will always turn on, and all leak history info
// is cleared
//
void alarmWaterShutoff() {

  if ( alarmState != alarmStateHistory ) {
    // Alarm goes to a different state - clear all leak information so that alarm OFF will
    // always turn water on
    digitalWrite(WATER_STOP_VALVE_PIN, alarmState);

    waterState = alarmState;
    waterLeakState = false;
    showerWaterUse = false;

    for (byte i=0; i < (sizeof(waterPulsesHistory) / sizeof(int)); i++) {
      waterPulsesHistory[i] = 0;
    }
  }

  alarmStateHistory = alarmState;
}

//
// Increment the number of water meter pulses
// * Do not count more than one pulse per second, for example thunder will cause false pulses
//
void incrementwaterPulses()
{
  if ( (millis() - lastWaterPulse) > 1000 ) {
    waterPulses++;
  }

  lastWaterPulse = millis();
}

//
// Increment the number of increment Heatpump Power Pulses
// * Do not count more than one pulse per second, for example thunder will cause false pulses
//
void incrementheatpumpPowerPulses()
{
  if ( (millis() - lastHeatpumpPowerPulses) > 1000 ) {
    heatpumpPowerPulses++;
  }

  lastHeatpumpPowerPulses = millis();
}

//
// Increment the number of increment Heatpump Rpm Pulses
// * Do not count more than 33 pulse per second, for example thunder will cause false pulses
//
void incrementheatpumpRpmPulses()
{
  if ( (millis() - lastHeatpumpRpmPulses) > 30 ) {
    heatpumpRpmPulses++;
  }

  lastHeatpumpRpmPulses = millis();
}

//
// Increment the number of increment House Power Pulses
// * Do not count more than 5 pulse per second, for example thunder will cause false pulses
//
void incrementhousePowerPulses()
{
  if ( (millis() - lastHousePowerPulses) > 200 ) {
    housePowerPulses++;
  }

  lastHousePowerPulses = millis();
}
 
void controlHouse() {
     alarmHouseHeatDrop();
     waterHeat();
     waterHeat1000w();
     }
     
// alarm house heat drop 

void alarmHouseHeatDrop()   
{
  // Keittiö, olohuone, takkahuone
  int outdoorTemp = owbuses[14].temperature;
  int JuliaBedroomTemp = owbuses[3].temperature;
  int masterBedroomTemp = owbuses[4].temperature;
  int utility = owbuses[2].temperature;

 // house heat varaavat lattiat

   // if outside temp goes to a on > 16 - house heat off
 if (outdoorTemp  >= HouseHeatOff ) {// outdoor temp
   digitalWrite(HOUSE_HEAT_RELAY_PIN, LOW);
   houseHeatState = false;
   Serial.println("house heat state OFF");

    // housePower >= 10 kw a on state - house heat on
   } else if (housePower >= 10) {// housePower >= 10 kw
    digitalWrite(HOUSE_HEAT_RELAY_PIN, LOW);
     houseHeatState = false;
     Serial.println("house heat state OFF");

   // water heat to a on state - house heat on
   //} else if (outdoorTemp  >= 5 && waterHeatState == LOW) {// waterHeatState == LOW
   // digitalWrite(HOUSE_HEAT_RELAY_PIN, LOW);
   //  houseHeatState = false;
   //  Serial.println("house heat state OFF");

     // sauna heat to a on state - house heat OFF
   } else if (saunaHeatState == LOW) {// saunaHeatState == LOW
    digitalWrite(HOUSE_HEAT_RELAY_PIN, LOW);
     houseHeatState = false;
     Serial.println("house heat state OFF");
   
   // Alarm goes to a on state - house heat OFF
   } else if (outdoorTemp  >= -10 && alarmState == LOW) {// outdoor temp
    digitalWrite(HOUSE_HEAT_RELAY_PIN, alarmState);
     houseHeatState = false;
     Serial.println("house heat state OFF");

   // if house heat Fission state goes to a off  - house heat OFF   
   } else if(HouseHeatFissioState  == HIGH && JuliaBedroomTemp  >= 18 && masterBedroomTemp  >= 18 ){
    houseHeatState = false;
    digitalWrite(HOUSE_HEAT_RELAY_PIN, LOW);
    Serial.println("house heat state OFF");

  // if Fissio heat goes to a on  - house heat on
   } else if (HouseHeatFissioState  == LOW && outdoorTemp  <= HouseHeatOn || JuliaBedroomTemp  <= 19 ||  masterBedroomTemp  <= 19 ) {
   digitalWrite(HOUSE_HEAT_RELAY_PIN, HIGH);
   houseHeatState = true;
   Serial.println("house heat state ON");
    
  }

  // house heat lattiat
  
 // if outside temp goes to a on > 16 - house heat off
// } else  if (outdoorTemp  >= HouseHeatOff ) {// outdoor temp
//  digitalWrite(HOUSE_HEAT_RELAY2_PIN, HIGH);
//  houseHeatState2 = true;
//  Serial.println("house heat state2 ON");


    // housePower >= 20 kw a on state - house heat OFF and warehouse 230v 16a off
 if (housePower >= 20) {// housePower >= 20 kw
    digitalWrite(HOUSE_HEAT_RELAY2_PIN, LOW);
    digitalWrite(WAREHOUSE_RELAY_PIN, HIGH);
     houseHeatState2 = false;
     Serial.println("house heat state2 OFF");
     WAREHOUSE_RELAY_STATE = false;
     Serial.println("WAREHOUSE_RELAY_STATE OFF");
     
   // sauna heat to a on state - house heat OFF
   } else if (saunaHeatState == LOW) {// saunaHeatState == LOW
    digitalWrite(HOUSE_HEAT_RELAY2_PIN, LOW);
    digitalWrite(WAREHOUSE_RELAY_PIN, HIGH);
     houseHeatState2 = false;
     Serial.println("house heat state2 OFF");
     WAREHOUSE_RELAY_STATE = false;
     Serial.println("WAREHOUSE_RELAY_STATE OFF");
   
   // Alarm goes to a on state - house heat OFF
   } else if (outdoorTemp  >= 15 && alarmState == LOW) {// outdoor temp
    digitalWrite(HOUSE_HEAT_RELAY2_PIN, alarmState);
     houseHeatState2  = false;
     Serial.println("house heat state2 OFF");
     
  // if Fissio heat2 goes to a OFF  - house heat OFF  
    } else if(HouseHeatFissioState2  == HIGH && utility >= 20){// { && outdoorTemp  <= HouseHeatOn
    houseHeatState2 = false;
    digitalWrite(HOUSE_HEAT_RELAY2_PIN, LOW);
    Serial.println("house heat state2 OFF");

  // if Fissio heat2 goes to a on  - house heat on
   } else if (HouseHeatFissioState2  == LOW ||  utility <= 19) {
   digitalWrite(HOUSE_HEAT_RELAY2_PIN, HIGH);
   houseHeatState2 = true;
   Serial.println("house heat state2 ON");
    
  }
    // housePower <= 15 kw a on state - warehouse 230v 16a ON
 if (housePower <= 15 && saunaHeatState == HIGH ) {// housePower <= 15 kw
    digitalWrite(WAREHOUSE_RELAY_PIN, LOW);
     WAREHOUSE_RELAY_STATE = true;
     Serial.println("WAREHOUSE_RELAY_STATE ON");
  
  }

}

     
// water Heat if top temperature is lower that 30   

void waterHeat() 
{
  
  int boilerTopTemp = owbuses[11].temperature;
  
 // If water goes lever to a on state - water heat on
  if (WaterHeatFissioState == LOW || boilerTopTemp <= WaterBoilerTopHeatOn ) {
    digitalWrite(WATER_HEAT_RELAY_PIN, LOW);
    waterHeatReleyState = true;
    Serial.println("water heat reley state ON");
    
    } else {
    waterHeatReleyState = false;
    digitalWrite(WATER_HEAT_RELAY_PIN, HIGH);
    Serial.println("water heat reley state OFF");
  }  
   
}
void waterHeat1000w() 
{
  

//  if (House_Power_kW/1000)  < SolarPower )) {// boiler_top temp
    
    // If solar power goes over house use power - water heat on
//    digitalWrite(WATER_HEAT_1000W_RELAY_PIN, LOW);
//    waterHeat1000wReleyState = true;
//    Serial.println("waterHeat1000wReleyStateON");
//    } else {
//    waterHeat1000wReleyState = false;
//    digitalWrite(WATER_HEAT_1000W_RELAY_PIN, HIGH);
//    Serial.println("waterHeat1000wReleyStateOFF");
//  }  
    
}

//
// The most important thing of all, feed the watchdog
//
void feedWatchdog()
{
  wdt_reset();
}
