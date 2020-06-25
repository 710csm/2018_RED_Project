#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LOG_OUT 1 // use the log output function
#define FFT_N 128 // set to 256 point fft

#include <FFT.h> //include the library
#include <avr/io.h>
#include <avr/interrupt.h>

int color[4]={0};
int colorIndex = 0;

int r[6] = {0};
int g[6] = {0};
int b[6] = {0};
String readString = "";
char toRead;

#define N_PIXELS  15  // Number of pixels in strand
#define MIC_PIN   A0  // Microphone is attached to this analog pin
#define LED_PIN    4  // NeoPixel LED strand is connected to this pin
#define SAMPLE_WINDOW  30  // Sample window for average level
#define PEAK_HANG 7   //Time of pause before peak dot falls
#define PEAK_FALL 3   //Rate of falling peak dot
#define INPUT_FLOOR 40    //Lower range of analogRead input
#define INPUT_CEILING 250 //Max range of analogRead input, the lower the value the more sensitive (1023 = max)

byte peak[6] = {30};        // Peak level of column; used for falling dots
unsigned int sample;
 
byte dotCount[6] = {0};     //Frame counter for peak dot
byte dotHangCount[6] = {0}; //Frame counter for holding peak dot

Adafruit_NeoPixel strip[6] = { Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800), Adafruit_NeoPixel(N_PIXELS, LED_PIN+1, NEO_GRB + NEO_KHZ800)
                                ,Adafruit_NeoPixel(N_PIXELS, LED_PIN+2, NEO_GRB + NEO_KHZ800),Adafruit_NeoPixel(N_PIXELS, LED_PIN+3, NEO_GRB + NEO_KHZ800)
                                ,Adafruit_NeoPixel(N_PIXELS, LED_PIN+4, NEO_GRB + NEO_KHZ800),Adafruit_NeoPixel(N_PIXELS, LED_PIN+5, NEO_GRB + NEO_KHZ800)};

void setup() 
{
  Serial.begin(9600);
  TIMSK0 = 0;    // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40;  // use adc0
  DIDR0 = 0x01;  // turn off the digital input for adc0

  for(int i = 0 ; i < 6 ; i ++){
      strip[i].begin();
      strip[i].setBrightness(30);
      strip[i].show();// Initialize all pixels to 'off'
  }
    r[0]=210; g[0]=30; b[0]=30; 
    r[1]=210; g[1]=60; b[1]=66;
    r[2]=242; g[2]=109; b[2]=64;
    r[3]=200; g[3]=190; b[3]=77;
    r[4]=243; g[4]=210; b[4]=55;
    r[5]=243; g[5]=188; b[5]=1;
}
 
void loop() 
{
    unsigned int c[6], y[6];
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 256 ; i += 2) { // save 256 samples
       while(!(ADCSRA & 0x10)); // wait for adc to be ready
         ADCSRA = 0xf5; // restart adc
         byte m = ADCL; // fetch adc data
         byte j = ADCH;
         int k = (j << 8) | m; // form into an int
         k -= 0x0200; // form into a signed int
         k <<= 6; // form into a 16b signed int

         fft_input[i] = k; // put real data into even bins
         fft_input[i+1] = 0; // set odd bins to 0
     }
     
     fft_window();  // window the data for better frequency response
     fft_reorder(); // reorder the data before doing the fft
     fft_run();     // process the data in the fft
     fft_mag_log(); // take the output of the fft
     sei();

    if(fft_log_out[2] - 100 < 0) fft_log_out[2] = 0;
    else if(fft_log_out[2] - 100 > 20) fft_log_out[2]+=10;
    else fft_log_out[2] -= 100;

    if(fft_log_out[11] - 100 < 0) fft_log_out[11] = 0;
    else if(fft_log_out[11] - 100 > 20) fft_log_out[11]+=20;
    else fft_log_out[11] -= 100;

    if(fft_log_out[14] - 100 < 0) fft_log_out[14] = 0;
    else if(fft_log_out[14] - 100 > 20) fft_log_out[14]+=20;
    else fft_log_out[14] -= 100;

    if(fft_log_out[19] - 100 < 0) fft_log_out[19] = 0;
    else if(fft_log_out[19] - 100 > 20) fft_log_out[19]+=30;
    else fft_log_out[19] -= 100;

    if(fft_log_out[20] - 100 < 0) fft_log_out[20] = 0;
    else if(fft_log_out[20] - 100 > 20) fft_log_out[20]+=20;
    else fft_log_out[20] -= 100;

    if(fft_log_out[25] - 100 < 0) fft_log_out[25] = 0;
    else if(fft_log_out[25] - 100 > 20) fft_log_out[25]+=20;
    else fft_log_out[25] -= 100;

    while(Serial.available()>0)
    {
      for(int i=0; i<4; i++){
          delay(100);
          toRead = Serial.read();
          
          if(toRead == 'm'){
            color[colorIndex++] = atoi(readString.c_str());
            readString = "";
            continue;
          }
          readString += toRead;
      }
    }
    while(readString != ""){
        colorIndex = 0;
        Serial.print(color[0]);
        Serial.print(color[1]);
        Serial.print(color[2]);
        Serial.println(color[3]);
        readString = "";
        Serial.flush();
    }
     
      if(color[0] == 1){
        r[0]=color[1]; g[0]=color[2]; b[0]=color[3]; 
      }
      if(color[0] == 2){
        r[1]=color[1]; g[1]=color[2]; b[1]=color[3]; 
      }
      if(color[0] == 3){
        r[2]=color[1]; g[2]=color[2]; b[2]=color[3]; 
      }
      if(color[0] == 4){
        r[3]=color[1]; g[3]=color[2]; b[3]=color[3]; 
      }
      if(color[0] == 5){
        r[4]=color[1]; g[4]=color[2]; b[4]=color[3]; 
      }
      if(color[0] == 6){
        r[5]=color[1]; g[5]=color[2]; b[5]=color[3]; 
      }
    
      for (int j=0;j<=strip[0].numPixels()-1;j++){
           strip[0].setPixelColor(j,Wheel(map(j,0,strip[0].numPixels()-1,30,150), strip[0],1));
           strip[1].setPixelColor(j,Wheel(map(j,0,strip[1].numPixels()-1,30,150), strip[1],2));
           strip[2].setPixelColor(j,Wheel(map(j,0,strip[2].numPixels()-1,30,150), strip[2],3));
           strip[3].setPixelColor(j,Wheel(map(j,0,strip[3].numPixels()-1,30,150), strip[3],4));
           strip[4].setPixelColor(j,Wheel(map(j,0,strip[4].numPixels()-1,30,150), strip[4],5));
           strip[5].setPixelColor(j,Wheel(map(j,0,strip[5].numPixels()-1,30,150), strip[5],6));
      }
    
      c[0] = fscale(INPUT_FLOOR, INPUT_CEILING, strip[0].numPixels(), 0, fft_log_out[2], 2);
      c[1] = fscale(INPUT_FLOOR, INPUT_CEILING, strip[1].numPixels(), 0, fft_log_out[11], 2);
      c[2] = fscale(INPUT_FLOOR, INPUT_CEILING, strip[2].numPixels(), 0, fft_log_out[14], 2);
      c[3] = fscale(INPUT_FLOOR, INPUT_CEILING, strip[3].numPixels(), 0, fft_log_out[19], 2);
      c[4] = fscale(INPUT_FLOOR, INPUT_CEILING, strip[4].numPixels(), 0, fft_log_out[20], 2);
      c[5] = fscale(INPUT_FLOOR, INPUT_CEILING, strip[5].numPixels(), 0, fft_log_out[25], 2);
  
   //====================strip set====================
   for(int i = 0 ; i < 6 ; i ++){
      if(c[i] < peak[i]) {
        peak[i] = c[i];        // Keep dot on top
        dotHangCount[i] = 0;    // make the dot hang before falling
      }

      // Set the peak dot to match the rainbow gradient
      y[i] = strip[i].numPixels() - peak[i];

      if (c[i] <= strip[i].numPixels()) { // Fill partial column with off pixels
        drawLine(strip[i].numPixels(), y[i], strip[i].Color(0, 0, 0), strip[i]);
      }
   }
  
    for(int i = 0; i<6; i++){
      strip[i].setPixelColor(y[i]-1,Wheel(map(y[i],0,strip[i].numPixels()-1,30,150),strip[i],7)); //down dot  
    }
   
    // Frame based peak dot animation
    for(int i = 0 ; i < 6 ; i++){
      if(dotHangCount[i] > PEAK_HANG) { //Peak pause length
        if(++dotCount[i] >= PEAK_FALL) { //Fall rate 
          peak[i]++;
          dotCount[i] = 0;
        }
      }
      else {
        dotHangCount[i]++; 
      }
    }
   
     for(int i = 0 ; i < 6 ; i ++){
        strip[i].show();
     }
  }

//Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c, Adafruit_NeoPixel& stripCurrent) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i=from; i<=to; i++){
     stripCurrent.setPixelColor(i, c);
  }
}
 
float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve){
 
  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;
 
  // condition curve parameter
  // limit range
 
  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;
 
  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function
 
  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }
 
  // Zero Refference the values
  OriginalRange = originalMax - originalMin;
 
  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }
 
  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float
 
  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }
 
  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }
  return rangedValue;
} 
 
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos, Adafruit_NeoPixel& stripCurrent, int index) {
 switch(index){
   case 1:
      return stripCurrent.Color(r[0],g[0],b[0]);
      break;
    
  case 2:
       return stripCurrent.Color(r[1],g[1],b[1]);
       break;
    
  case 3:  
        return stripCurrent.Color(r[2],g[2],b[2]);
        break;

  case 4:
        return stripCurrent.Color(r[3],g[3],b[3]);
        break;
    
  case 5:
       return stripCurrent.Color(r[4],g[4],b[4]);
       break;
    
  case 6:
        return stripCurrent.Color(r[5],g[5],b[5]);
        break;

  case 7:
      return stripCurrent.Color(255,255,255);  //노란색: return stripCurrent.Color(240,180,20);
      break;
    }
}
