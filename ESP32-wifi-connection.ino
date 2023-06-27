//TUB CONNECTION
/*
  Complete details at https://RandomNerdTutorials.com/esp32-useful-wi-fi-functions-arduino/
*/

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>

// Replace with your network credentials (STATION)
const char *AP_SSID = "xxx";
const char *AP_PWD = "xxx";


WiFiMulti wifiMulti;

void initWiFi() {
  wifiMulti.addAP(AP_SSID, AP_PWD);
  Serial.print("Connecting to WiFi ..");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("connected");
  }
}


/*
  Liquid flow rate sensor -DIYhacking.com Arvind Sanjeev

  Measure the liquid/water flow rate using this code.
  Connect Vcc and Gnd of sensor to arduino, and the
  signal line to arduino digital pin 13.

*/
byte sensorInterrupt = 13;  
byte sensorPin       = 13;

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per litre/minute of flow.
// Depends on the flow meter sensor
float calibrationFactor = 5.5;
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
unsigned long secondOldTime;
unsigned long minute;
unsigned long thirdOldTime;
unsigned long fiveMinutes; //3minutes
bool sendData;

unsigned long complete_fill_up;
int refTUB;

String licensePlate;

void setup()
{
  Serial.begin(115200);
  initWiFi();

  pinMode(sensorPin, INPUT);
  

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  secondOldTime     = 0;
  minute            = 60000; //1 minute
  thirdOldTime      = 0;
  fiveMinutes       = 300000; //3 minutes
  sendData          = false;
  complete_fill_up  = 0;
  licensePlate      = "LT-432-VG";
  refTUB            = 1;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}



void loop() {

  if ((millis() - oldTime) > 1000)   // Only process counters once per second
  {
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;


    // Add the millilitres passed in this second to the cumulative total
    if (flowRate != 0) {

      totalMilliLitres += flowMilliLitres;


      // Print the flow rate for this second in litres / minute
      Serial.print("Flow rate: ");
      Serial.print(int(flowRate));  // Print the integer part of the variable
      Serial.print("L/min");
      Serial.print("\t");       // Print tab space

      // Print the cumulative total of litres flowed since starting
      Serial.print("Output Liquid Quantity: ");
      Serial.print(totalMilliLitres);
      Serial.println("mL");
      Serial.print("\t");       // Print tab space
      Serial.print(totalMilliLitres / 1000);
      Serial.print("L \t");

      // Boolean send data shift to true
      sendData = true;
    }

    //unsigned int frac;
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);


    // Each minute, update de complete fill up data
    if ((millis() - secondOldTime) > minute) {
      complete_fill_up = totalMilliLitres;

      // Print total in milliLitres
      Serial.print("Total = ");
      Serial.print(complete_fill_up);
      Serial.print("\n");

      secondOldTime = millis();
    }

    if((millis() - thirdOldTime) > fiveMinutes){
      if (sendData == true && complete_fill_up > 1000) {

        // Print total in milliLitres
        Serial.print("\n Warning data = ");
        Serial.print(complete_fill_up);
        Serial.print(" is sent. \n");

        thirdOldTime = millis();

        Serial.println("Posting JSON data to server...");
        // Block until we are able to connect to the WiFi access point
        if (wifiMulti.run() == WL_CONNECTED) {
          Serial.println("connected");
           
          HTTPClient http;   
            
          http.begin("https://hook.eu1.make.com/tkjmpvsb6kiwf9hoarneeo284y1g5g3k");
          http.addHeader("Content-Type", "application/json");         
           
          StaticJsonDocument<200> doc;
          // Add values in the document
          //
          doc["License_plate"] = licensePlate;
          doc["data"] = complete_fill_up;
          
          doc["ref"] = refTUB;
         
          String requestBody;
          serializeJson(doc, requestBody);
           
          int httpResponseCode = http.POST(requestBody);
       
          if(httpResponseCode>0){
             
            String response = http.getString();                       
             
            Serial.println(httpResponseCode);   
            Serial.println(response);

            complete_fill_up = 0;
            totalMilliLitres = 0;
            sendData = false;
          }
        }
        
      } 
    }
  }
}

/*
  Insterrupt Service Routine
*/
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
          
