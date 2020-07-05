#include <avdweb_Switch.h>

#include <IRremote.h>

IRsend irsend;

unsigned long irCode = 0xC160E02E; // On/Off code van de airco
int irBits = 32;

int khz = 38; // 38kHz carrier frequency for the NEC protocol
unsigned int  rawFanData[] = {8950,4400, 650,1550, 650,1600, 650,450, 650,450, 650,500, 650,450, 650,500, 650,1600, 650,450, 650,1550, 650,1600, 650,450, 650,500, 600,550, 600,500, 650,450, 650,1600, 650,1600, 600,1600, 600,500, 650,500, 650,500, 600,500, 600,550, 550,500, 650,500, 650,1600, 600,500, 600,1600, 600,1650, 600,1650, 600,500, 600,550, 550,550, 600,500, 650,500, 600,550, 600,500, 600,550, 600,500, 650,500, 600,500, 600,500, 600,550, 600,500, 600,550, 600,550, 550,550, 550};  // NEC C160E02E
//unsigned int  rawAircoData[] = {8900,4400, 650,1600, 600,1600, 650,500, 600,500, 600,550, 600,500, 650,500, 600,1600, 650,500, 600,1600, 600,1650, 600,500, 600,500, 650,500, 600,550, 600,500, 600,1600, 650,1600, 600,1600, 650,500, 600,500, 650,500, 600,550, 600,500, 600,500, 650,1600, 600,500, 600,500, 650,500, 600,500, 650,500, 600,1650, 600,500, 600,500, 650,500, 600,500, 600,550, 600,500, 650,500, 600,500, 650,450, 650,500, 600,500, 600,550, 600,500, 600,550, 600,500, 650,450, 650};  // NEC C160E041
unsigned int  rawAircoData[] = {8900,4400, 650,1600, 600,1600, 600,500, 650,500, 600,500, 650,500, 650,500, 650,1550, 650,450, 650,1600, 600,1600, 650,500, 600,500, 600,550, 650,500, 650,450, 650,1550, 650,1600, 600,1600, 650,500, 600,500, 650,500, 600,500, 650,500, 600,500, 650,1600, 600,500, 650,450, 650,500, 600,500, 650,1600, 650,500, 650,450, 600,500, 650,450, 700,450, 650,450, 650,500, 600,550, 650,450, 700,400, 700,450, 650,450, 700,400, 650,500, 600,550, 650,450, 650,450, 650};  // NEC C160E042
//Data is airco 22 graden en fan low

const int irPin = 3; //HARD CODED IN LIBRARY

const int oneHrLed = 5;
const int twoHrLed = 4;
const int fourHrLed = 7;
const int modeLed = 6;

const byte setKeyPin = 10;
const byte startKeyPin=11;
const byte onKeyPin=12;
const byte modeKeyPin=9;

Switch setKey = Switch(setKeyPin); // button to GND, use internal 20K pullup resistor
Switch startKey = Switch(startKeyPin);
Switch onKey = Switch(onKeyPin);
Switch modeKey = Switch(modeKeyPin);

enum timerValues : unsigned int {eenUur=3600, tweeUur=7200, vierUur=14400};
timerValues timerState = eenUur;
unsigned long countDownValue=0;
int blinkCounter;

const unsigned int LUS=20;			   //wachttijd in ms voor de hele lus, let op key debounce miet minstens elke 20ms een update


enum MijnSTATES : byte {initialize, idle, countdown, fire, stopped }; //Welke states kent het programma
MijnSTATES oldstate,state = initialize;


enum Modes : byte {fan, airco};
Modes mode = fan;

void setup()
{
  state = initialize;
    
  pinMode(oneHrLed, OUTPUT);
  pinMode(twoHrLed, OUTPUT);
  pinMode(fourHrLed, OUTPUT);
  pinMode(modeLed, OUTPUT);

  Serial.begin(9600);
  Serial.println("Started Countdown-timer.ino");
  Serial.println("Versie 1, 5 juli 2020");

  animateLeds();
    
  timerState = eenUur;
  digitalWrite(oneHrLed,1);
  
  state = idle; 
}

void loop()
{
  long beginTime = millis();  //meet de tijd om de loop te timen
  
   switch (state) 
   {
     
     case idle:
     {
       readKeys();
       
       if (setKey.pushed())
       {
         switch (timerState) 
         {
           case eenUur:
           {
             timerState=tweeUur;
             digitalWrite(oneHrLed,0);
             digitalWrite(twoHrLed,1);
             digitalWrite(fourHrLed,0);
             break;
             
           }
           case tweeUur:
           {
             timerState=vierUur;
             digitalWrite(oneHrLed,0);
             digitalWrite(twoHrLed,0);
             digitalWrite(fourHrLed,1);
             break;
           
           }
           case vierUur:
           {
             timerState=eenUur;
             digitalWrite(oneHrLed,1);
             digitalWrite(twoHrLed,0);
             digitalWrite(fourHrLed,0);
             break;
           }
         }  
       }
       
       if (startKey.pushed())
       {
  
         countDownValue = ((long)timerState*1000)/LUS;
         //Serial.println(timerState);
         //Serial.println(countDownValue);
         state=countdown;
         
       }
       
       if (onKey.pushed())
       {
         sendIrCommand();
       }

       if (modeKey.pushed())
       {

         if (mode==fan)
         {
          mode = airco;
         }
         else
         {
          mode = fan;
         }
           
       }

       break; // idle
     }
     case countdown:
     {
       blinkLeds();
       countDownValue--;
       
       if (countDownValue==0)
       {
         state=fire;
       }
       readKeys();
       
       if (onKey.pushed())
       {
         sendIrCommand();
       }
       if (startKey.pushed())
       {
         state=idle;
         restoreLedState();
       }
        break;
     }
     case fire:
     {
       sendIrCommand();
       state=stopped;
       break;
     }
     case stopped:
     {
       // lees toetsen
      
       digitalWrite(oneHrLed,0);
       digitalWrite(twoHrLed,0);
       digitalWrite(fourHrLed,0);
       
       readKeys();
       if (setKey.pushed()) 
       { 
         state=idle;
         restoreLedState();
       }
                      
       break;
     }
   
  }
  
  // set mode led
  if (mode==airco)
  {     
   digitalWrite(modeLed,1);
  }
  else
  {
    digitalWrite(modeLed,0);
  } 
 
  while ((millis() - beginTime) < LUS); //wacht to gedefinieerde lus tijd verstreken is
}

void readKeys()
{
  setKey.poll();
  startKey.poll(); 
  onKey.poll();
  modeKey.poll();
}


void sendIrCommand()
{
   
   //irsend.sendNEC(irCode, irBits);
   if (mode==fan)
   {
    irsend.sendRaw(rawFanData, sizeof(rawFanData) / sizeof(rawFanData[0]), khz);
   }
   else
   {
    irsend.sendRaw(rawAircoData, sizeof(rawAircoData) / sizeof(rawAircoData[0]), khz);
   }
         
}

void blinkLeds()
{
  blinkCounter = (blinkCounter+1) % (1000/LUS);
  
  if (!blinkCounter)
  {
    switch (timerState) 
         {
           case eenUur:
           {
             digitalWrite(oneHrLed,!digitalRead(oneHrLed));
             break;
             
           }
           case tweeUur:
           {
             digitalWrite(twoHrLed,!digitalRead(twoHrLed));
             break;
           
           }
           case vierUur:
           {
             digitalWrite(fourHrLed,!digitalRead(fourHrLed));
             break;
           }
        }
  }
  
}

void restoreLedState()
{
  switch (timerState) 
         {
           case eenUur:
           {
             digitalWrite(oneHrLed,1);
             break;
             
           }
           case tweeUur:
           {
             digitalWrite(twoHrLed,1);
             break;
           
           }
           case vierUur:
           {
             digitalWrite(fourHrLed,1);
             break;
           }
        }
}

void animateLeds()
{
       digitalWrite(oneHrLed,1);
       digitalWrite(twoHrLed,1);
       digitalWrite(fourHrLed,1);
       digitalWrite(modeLed,1);
       delay(500);
       digitalWrite(modeLed,0);
       delay(200);
       digitalWrite(fourHrLed,0);
       delay(200);
       digitalWrite(twoHrLed,0);
       delay(200);

}
