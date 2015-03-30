/*
  FutabaUsVfd Library
 * Display +5v (pin 1) & GND (pin 3) connected to regulated 5v, 500ma power supply.
 * Display GND   (pin 3) also connected to Arduino ground.
 * Display Clock (pin 2) connected to Arduino pin 2.
 * Display Data  (pin 4) connected to Arduino pin 3.
 * Display Reset (pin 5) connected to Arduino pin 4.
 http://arduino.cc/playground/Main/FutabaUsVfd
*/

// Include the library code.
#include <FutabaUsVfd.h>

FutabaUsVfd vfd(2, 3, 4);

#include <NewPing.h>

#define TRIGGER_PIN  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     7  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

int ledRedPin = 9;
int ledGreenPin = 10;

int potPin = 0;

int photoResistorPin = 1;

static unsigned long distanceMeasurementIntervalTimer;
static unsigned long ledRedBlinkIntervalTimer;
static unsigned long ledGreenBlinkIntervalTimer;

int ledRedBlinkRate = 0;
int ledGreenBlinkRate = 500;

boolean ledRedState = false;
boolean ledGreenState = false;

float potValue;
char potValueChar[5];

//cm min max for adjustment
int minDistanceCm = 10;
int maxDistanceCm = 100;

int parkDistanceOkThreshold = 10; // 5 cm
int parkDistanceBlinkThreshold = 40; // 50 cm

int minCriticalDistanceCm = 1;
int maxCriticalDistanceCm = 10;

int minBlinkRate = 1000;
int maxBlinkRate = 50;

int distanceMeasurementInterval = 250; // 250 ms = 4 readings per second

void setup() {
  
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  
  // Set up the VFD's number of columns and rows: 
  vfd.begin(16, 2);
  // Print a message to the VFD.
  
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  
  distanceMeasurementIntervalTimer = millis() + distanceMeasurementInterval;
  setRedLedBlinkRate(ledRedBlinkRate);
  setGreenLedBlinkRate(ledGreenBlinkRate);
}

void loop() {
  
  if( (long)( millis() - distanceMeasurementIntervalTimer ) >= 0){

    vfd.clear();
    
    int photoResistorReading = analogRead(photoResistorPin);
    
    vfd.setCursor(0, 0);
    vfd.print("LDR ");
    vfd.print(photoResistorReading);
    
    if(photoResistorReading > 200){
    
     Serial.println("---------------"); 
      // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
      unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
      
      int potReading = analogRead(potPin);
      potValue = (float)potReading / 1024;
      //---------------
      // potValue = 0
      // distance cm = 10
      // ------
      // potValue = 1024
      // distance cm = 100
      //---------------
      int comparisonPotValueCm = minDistanceCm + (potValue * (maxDistanceCm - minDistanceCm));
      
      int distanceMeasuredCm = uS / US_ROUNDTRIP_CM;
      
      //int parkDistanceOkThreshold = 5; // 5 cm
      //int parkDistanceBlinkThreshold = 50; // 5 cm
      
        Serial.print("distanceMeasuredCm: ");
        Serial.println(distanceMeasuredCm);
        
        Serial.print("comparisonPotValueCm: ");
        Serial.println(comparisonPotValueCm);
      
      if(distanceMeasuredCm >= comparisonPotValueCm){
        
        setRedLedBlinkRate(0);
        
        int distDelta = distanceMeasuredCm - comparisonPotValueCm;
        
        Serial.print("distDelta 1: ");
        Serial.println(distDelta);
        
        if ( distDelta <= parkDistanceBlinkThreshold){
          
          if ( distDelta <= (parkDistanceOkThreshold/2) ){
            
            setGreenLedBlinkRate(0);
            setRedLedBlinkRate(0);
            
            ledGreenState = true;
            ledRedState = true;
            analogWrite(ledGreenPin, 255);
            analogWrite(ledRedPin, 255);
            
          }else{
            distDelta = distDelta - (parkDistanceOkThreshold/2);
            
            Serial.print("distDelta 2: ");
            Serial.println(distDelta);
        
            float distProportion = distDelta / (float)parkDistanceBlinkThreshold;
            //distProportion -= 1;
            
            //distDelta
            //parkDistanceOkThreshold
            
            Serial.print("distProportion: ");
            Serial.println(distProportion);
          
            Serial.println("FINE TUNING");
            ledGreenBlinkRate = maxBlinkRate + (distProportion * (minBlinkRate - maxBlinkRate ));
            setGreenLedBlinkRate(ledGreenBlinkRate);
            
            Serial.print("ledGreenBlinkRate: ");
            Serial.println(ledGreenBlinkRate);
          }
        
        }else{
          setGreenLedBlinkRate(minBlinkRate);
          setRedLedBlinkRate(0);
        }
        
      }else{
        int overshootCm = comparisonPotValueCm - distanceMeasuredCm;
        
        Serial.println("overshoot detected! ");
        
        Serial.print("overshootCm: ");
        Serial.println(overshootCm);

        if( overshootCm > (parkDistanceOkThreshold/2) ){
          setGreenLedBlinkRate(0);
          setRedLedBlinkRate(maxBlinkRate);
          
          Serial.println("CRITICAL overshoot detected! ");
        
        }else{
            setGreenLedBlinkRate(0);
            setRedLedBlinkRate(0);
            
            ledGreenState = true;
            ledRedState = true;
            analogWrite(ledGreenPin, 255);
            analogWrite(ledRedPin, 255);
        }
        

        
        /*
        float overshootProportion = (float)overshoot / maxCriticalDistanceCm;
int minCriticalDistanceCm = 1;
int maxCriticalDistanceCm = 10;
        
        ledRedBlinkRate = minBlinkRate + (distProportion * (maxBlinkRate - minBlinkRate));
        */
      }
            
      
      vfd.setCursor(8, 0);
      vfd.print("POT ");
      vfd.print(potValue);
      
      vfd.setCursor(0, 1);
      vfd.print("Set ");
      vfd.print(comparisonPotValueCm);
      vfd.print("/");
      vfd.print(distanceMeasuredCm);
      vfd.print(" ");
      vfd.print(ledGreenBlinkRate);
      
      
    }else{ // end if(photoResistorReading > 200){
      ledGreenBlinkRate = 0;
      ledRedBlinkRate = 0;
      ledGreenState = false;
      ledRedState = false;
      analogWrite(ledGreenPin, 0);
      analogWrite(ledRedPin, 0);
    }
    
    //reset timer
    distanceMeasurementIntervalTimer = millis() + distanceMeasurementInterval;
  }
  
  if(ledRedBlinkRate > 0 && (long)( millis() - ledRedBlinkIntervalTimer ) >= 0 ){
    if(ledRedState){
      ledRedState = false;
      analogWrite(ledRedPin, 0);
    }else{
      ledRedState = true;
      analogWrite(ledRedPin, 255);
    }
    ledRedBlinkIntervalTimer = millis() + ledRedBlinkRate;
  }
  
  if(ledGreenBlinkRate > 0 && (long)( millis() - ledGreenBlinkIntervalTimer ) >= 0 ){
    if(ledGreenState){
      ledGreenState = false;
      analogWrite(ledGreenPin, 0);
    }else{
      ledGreenState = true;
      analogWrite(ledGreenPin, 255);
    }
    ledGreenBlinkIntervalTimer = millis() + ledGreenBlinkRate;
  }else{}
  
}

void setGreenLedBlinkRate(int blinkRate){
    if(blinkRate == 0){
      analogWrite(ledGreenPin, 0);
      ledGreenState = false;
    }
    if(blinkRate != ledGreenBlinkRate){
      if(ledGreenBlinkRate == 0){
        ledGreenBlinkIntervalTimer = millis() + ledGreenBlinkRate;
      }
      ledGreenBlinkRate = blinkRate;
    }
    
    /*
    ledGreenBlinkRate = blinkRate;
    
    int currentMillis = millis();
    if( (long)( currentMillis - ledGreenBlinkIntervalTimer ) < 0 ){
      ledGreenBlinkIntervalTimer = currentMillis + ledGreenBlinkRate;
    }
    */
}
void setRedLedBlinkRate(int blinkRate){
    if(blinkRate == 0){
      analogWrite(ledRedPin, 0);
      ledRedState = false;
    }
    if(blinkRate != ledRedBlinkRate){
      if(ledRedBlinkRate == 0){
        ledGreenBlinkIntervalTimer = millis() + ledGreenBlinkRate;
      }
      ledRedBlinkRate = blinkRate;
    }
    
    /*
    ledGreenBlinkRate = blinkRate;
    
    int currentMillis = millis();
    if( (long)( currentMillis - ledGreenBlinkIntervalTimer ) < 0 ){
      ledGreenBlinkIntervalTimer = currentMillis + ledGreenBlinkRate;
    }
    */
}

//------------PrintFloat---------------
void floatToCharArr(char outputStr[10], float value, int places) {
  // this is used to cast digits
  int digit;
  float tens = 0.1;
  int tenscount = 0;
  int i;
  float tempfloat = value;
  char charDigit;
  
  //char outputStr[places + 2];
  int outputStrPos = 0;
  
  // if value is negative, set tempfloat to the abs value
  // make sure we round properly. this could use pow from  
  //<math.h>, but doesn't seem worth the import
  // if this rounding step isn't here, the value  54.321 prints as  
  //54.3209
  
  // calculate rounding term d:   0.5/pow(10,places)
  float d = 0.5;
  if (value < 0)
        d *= -1.0;
  // divide by ten for each decimal place
  for (i = 0; i < places; i++)
        d/= 10.0;
  // this small addition, combined with truncation will round our  
  // values properly
  tempfloat +=  d;
  
  // first get value tens to be the large power of ten less than value
  // tenscount isn't necessary but it would be useful if you wanted  
  // to know after this how many chars the number will take
  
  if (value < 0)
        tempfloat *= -1.0;
  while ((tens * 10.0) <= tempfloat) {
        tens *= 10.0;
        tenscount += 1;
  }
  
  // write out the negative if needed
  if (value < 0){
    outputStr[outputStrPos] = '-';
    outputStrPos++;
  }
  
  if (tenscount == 0){
    outputStr[outputStrPos] = '0';
    outputStrPos++;
  }
  
  for (i=0; i< tenscount; i++) {
        digit = (int) (tempfloat/tens);
        itoa(digit, &charDigit, 10);
        outputStr[outputStrPos] = charDigit;
        tempfloat = tempfloat - ((float)digit * tens);
        tens /= 10.0;
        outputStrPos++;
  }
  
  // if no places after decimal, stop now and return
  if (places <= 0){
    outputStr[outputStrPos] = '\0';
    return;
  }
  
  // otherwise, write the point and continue on
  
  outputStr[outputStrPos] = '.';
  outputStrPos++;
  //Serial.print('.');
  
  // now write out each decimal place by shifting digits one by one  
  // into the ones place and writing the truncated value
  for (i = 0; i < places; i++) {
        tempfloat *= 10.0;
        digit = (int) tempfloat;
        //Serial.print(digit,DEC);
        itoa(digit, &charDigit, 10);
        outputStr[outputStrPos] = charDigit;
        outputStrPos++;
        // once written, subtract off that digit
        tempfloat = tempfloat - (float) digit;
  }
  
  outputStr[outputStrPos] = '\0';
  return;
}

