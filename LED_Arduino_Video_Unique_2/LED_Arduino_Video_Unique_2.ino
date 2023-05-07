#include <FastLED.h>

#define NB_LED 40 // attention, le buffer de l'arduino n'est que de 64bits, donc il faut surveiller le "Serial.available() >= (NB_LED)*3". (NB_LED)*3 Doit être inférieur à 64. 
#define LED_PIN 4

CRGB led[NB_LED];

int start;
bool error;
int red;
int green;
int blue;

void setup() {
  Serial.begin(115200, SERIAL_8N1); //57600 on ne met que 7 bits (car valeurs divisées par deux pour accelerer la transmission)
  FastLED.addLeds<NEOPIXEL, LED_PIN>(led, NB_LED);
  clearSerialData();
}

void loop() { 
  error = false;
  start = 0;
  int i = 0;

  while(start != 200){
    if(Serial.available() > 0){
      start = Serial.read();
    }  
  }

  while(i < NB_LED){
    if(Serial.available() > 2) {
      red = Serial.read();
      if(red == 201) { // end msg has been received before all data has been received (data loss)
        error = true;
        break;        
      }

      green = Serial.read();
      if(green == 201) { // end msg has been received before all data has been received (data loss)
        error = true;
        break;        
      }

      blue = Serial.read();
      if(blue == 201) { // end msg has been received before all data has been received (data loss)
        error = true;
        break;        
      }

      led[i] = CRGB(red << 1, blue << 1, green << 1);
      i++;
    }
  }

  if(!error) {
    while(Serial.available() <= 0);

    if(Serial.read() == 201)  
      FastLED.show();
  }
}

void clearSerialData(){
  int clearSerialData;
  while(Serial.available() > 0){
    clearSerialData = Serial.read();
  }
}
