//nes-in.ino

#include <SPI.h>
#include "RF24.h"

#define NES 0x0
#define SNES 0x1

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
uno spi pins 
SPI: 10 (SS), 11 (MOSI), 12 (MISO), 13 (SCK). 
radio(ce_pin,cs_pin) */
RF24 radio(9,10);
byte addresses[][6] = {"1Node","SNES1"};
/**********************************************************/

const int radioNumber = 0;
const bool debugPrint = false;

const byte latch = 4; // Orange(HDR)
const byte clock = 7; // Yellow(HDR)
const byte data = 8; // Red(HDR)

unsigned long previousFrameMillis=0;
//int frameInterval = 16; // approx 1/60 (60Hz), slightly faster than NES
int frameInterval = 16; // approx 1/60 (60Hz), slightly faster than NES

unsigned long previousPrintMillis=0;
int printInterval = 1000;
/*

Every 60Hz,

12us Latch
wait 6us
8 pulses, 12us per cycle, 50% Duty Cycle
data clocked in on falling edge

*/


void setup() {
  // put your setup code here, to run once:
	Serial.begin(115200);
	Serial.println(F("S/NES RF24 Transmitter [From Controller]"));
	pinMode(latch, OUTPUT);
	pinMode(clock, OUTPUT);
	pinMode(data, INPUT_PULLUP); // is pull up needed? yup! :)
	
	// Radio setup
	radio.begin();
  	// Set the PA Level low to prevent power supply related issues since this is a
 	// getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  	radio.setPALevel(RF24_PA_LOW);

	  if(radioNumber){
	    radio.openWritingPipe(addresses[1]);
	    radio.openReadingPipe(1,addresses[0]);
	  }else{
	    radio.openWritingPipe(addresses[0]);
	    radio.openReadingPipe(1,addresses[1]);
	  }
//	radio.openWritingPipe(addresses[0]);
}

unsigned int previousController;

void loop() {
	// check the clock
	unsigned long currentMillis = millis();

	// time to get the next frame yet?
	if ((currentMillis - previousFrameMillis) > frameInterval) {
		unsigned int controllerButtons = 0xFFFF;

		// assert latch for 12 us
		digitalWrite(latch, HIGH);
		delayMicroseconds(8); // spec says 12us, adjusting for digitalWrite
		digitalWrite(latch, LOW);
		// wait for 6us (ugh)
		delayMicroseconds(4);

		if (debugPrint && (currentMillis - previousPrintMillis > printInterval))
			Serial.println(currentMillis);

		// clock out the data

		for (int pulseCounter=0; pulseCounter < 16; pulseCounter++) {
			// spec i read said 8 clocks for the data, but looks like latch
			// asserted low reads the first byte ('A')

			int dataValue = digitalRead(data);
			if (debugPrint && (currentMillis - previousPrintMillis > printInterval)) {
				Serial.print(dataValue);
			} 

			controllerButtons = controllerButtons << 1;

			if (dataValue) 
				controllerButtons = controllerButtons | 0x0001;
			else
				controllerButtons = controllerButtons & 0xFFFE;


			// clock pulse, approximately 24us long 9us HIGH, 15us LOW
			// much slower than NES
			// don't need the 8th clock because latch brought in the first bitu
			if (pulseCounter != 15) {
				digitalWrite(clock, HIGH);
				delayMicroseconds(4);
				digitalWrite(clock, LOW);
				delayMicroseconds(4);	
			}
			
		}
		if (debugPrint && (currentMillis - previousPrintMillis > printInterval) ) {
			Serial.println();
			Serial.print("bitRead: ");
			//for (int x=8; x>=0; x--)
			for (int x=0; x<sizeof(controllerButtons); x++)
				Serial.print(bitRead(controllerButtons,x));
			Serial.println();
			Serial.print("Bin: "); Serial.println(controllerButtons, BIN);	
			Serial.print("Dec: "); Serial.println(controllerButtons, DEC);
			Serial.print("Hex: "); Serial.println(controllerButtons, HEX);
			Serial.println(); 
			Serial.flush();
			previousPrintMillis = currentMillis;
		} 

		//printButtons(controllerButtons,NES);

		if (0 && (currentMillis - previousPrintMillis > printInterval)) {
			printButtons(controllerButtons,NES);
			Serial.flush();
			previousPrintMillis = currentMillis;
		}

		//Serial.print(F("Now sending: "));
    	//Serial.println(controllerButtons, BIN);

   		 /*if (!radio.write( &controllerButtons, sizeof(controllerButtons) )){
      		 Serial.println(F("failed"));
    	}*/

        if (controllerButtons != previousController) {
        	Serial.print(F("Last: "));
        	Serial.println(previousController, BIN);
        	Serial.print(F("Now : "));
        	Serial.println(controllerButtons, BIN);
        	radio.write( &controllerButtons, sizeof(controllerButtons));
        	previousController = controllerButtons;
        }
		previousFrameMillis = millis();
	}
}

String directions[] = {"right","left","down","up","start","select","b","a","err"};

void printButtons(unsigned int buttons, byte controllerType) {
	switch (controllerType) {
		case NES:
			for (int x=7; x>=0; x--)
				if (!bitRead(buttons,x))
					Serial.println(directions[x]);
		break;

		case SNES:
		break;

		default:
			return;
		break;
	}

}
