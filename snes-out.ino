// nes-out.ino
// https://hackaday.io/project/12940-wireless-snes-with-nrf24
// @baldengineer

#include <SPI.h>
#include "RF24.h"

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
uno spi pins 
SPI: 10 (SS), 11 (MOSI), 12 (MISO), 13 (SCK). 
radio(ce_pin,cs_pin) */
const int radioNumber = 1;
RF24 radio(9,10);
byte addresses[][6] = {"1Node","SNES1"};
/**********************************************************/

/*
const byte latch = 3y; // Orange(HDR)
const byte clock = 7; // Yellow(HDR)
const byte data = 8; // Red(HDR)
*/

const byte latch = 3; // PD3
const byte clock = 7; // PD7
const byte data = 8; //  PB0



void setup() {
 // Serial.begin(115200);
//  Serial.println(F("S/NES RF24 Receiver [To Console]"));
  
  pinMode(latch, INPUT); // black PD3, pin 3
  pinMode(clock, INPUT); // orange PD7, pin 7
  pinMode(data, OUTPUT); // red PB0, pin 8
  digitalWrite(data, LOW);

	attachInterrupt(digitalPinToInterrupt(latch), handleController, RISING);


  radio.begin();

	// Set the PA Level low to prevent power supply related issues since this is a
	// getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  
  // Open a reading pipe on this radio
//  radio.openReadingPipe(1,addresses[0]);

  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }

    // Start the radio listening for data
  radio.startListening();
}

bool skip = false;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
int interval = 250;

volatile int controller = 0xFFFF;

int lastLatchState = LOW;
int lastClockState = HIGH;
int latchState = 0;
int clockState = 0;


void handleSerial() {
	return;

	char incomingByte = Serial.read();

	switch (incomingByte) {
		case '!':
			skip = true;
		break;

		case '@':
			skip = false;
		break;
	}
}

int previousController=0;

int handleRadio() {
  int incomingController = 0xFFFF;
  int newController = 0xFFFF;

  
	//while (radio.available()) {                                   
	  radio.read( &incomingController, sizeof(incomingController) );             // Get the payload
//	}
//   Serial.print(F("Rec: "));      
//   Serial.println(incomingController, BIN);  

  for(int x=0; x < 16; x++) {
  	// 0000 0000 0000 0000y
  	if (bitRead(incomingController, x)) 
  		newController = newController | 0x01;
  	else
  		newController = newController & 0xFFFE;
  	if (x != 15)
  		newController = newController << 1;
  }
  	  if (previousController != incomingController) {
  	  	/*
  	     	Serial.println(F("Chg"));
	  	    Serial.print(F("Last: "));
        	Serial.println(previousController, BIN);
        	Serial.print(F("Now : "));
        	Serial.println(incomingController, BIN);
        	Serial.print(F("Swiz: "));
        	Serial.println(newController, BIN); */
        	//radio.write( &incomingController, sizeof(incomingController));
        	previousController = incomingController;
	  }
 // Serial.print(F("Now: "));      
//  Serial.println(newController, BIN);  

 // delay (500);
  return newController;
}

void handleController() {
	int thisControllerValue = controller;
	for (int x = 0; x<16; x++) {			
			if (thisControllerValue & 0x01)
				PORTB = PORTB | 0x01;  // 0000 0001
			else
				PORTB = PORTB & 0xFE;  // 1111 1110

			while (PIND & 0x80);
			while (!(PIND & 0x80));

			thisControllerValue = thisControllerValue >> 1;
	}
	//PORTB = PORTB | 0x01; // PB0, HIGH: 0000 0001
	PORTB = PORTB & 0xFE; // PB0, LOW.
	return;
}

void loop() {
	/*if (Serial.available())
		handleSerial(); */

	while (skip)
		handleSerial();

	if( radio.available())
		controller = handleRadio();
	
	currentMillis = millis();
}
