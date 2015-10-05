/**************************************************************
 * Blynk-enabled door sentinel.
 *
 * This code provides custom-variables that are connected to
 * sensors, lights and lock. 
 *
 * Intended to be installed on an Arduino connected to
 * a Raspberry Pi home automation hub.
 *
 * Based on Blynk usb-serial example.
 *
 * The MsTimer2 Library is needed to schedule a countdown event at regular intervals
 *
 * Pi needs to run the cloud-to-serial connector:
 *
 *      ./blynk-ser.sh (may need to run with sudo)
 *
 *      ./blynk-ser.sh -c <serial port> -b <baud rate> -s <server address> -p <server port>
 *
 **************************************************************/

#include <MsTimer2.h>

// #include <SoftwareSerial.h>
// SoftwareSerial SwSerial(11, 12); // RX, TX
// #define BLYNK_PRINT SwSerial
#include <BlynkSimpleSerial.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "putyourkeyhere";

// 
// IO PIN ASSIGNMENTS
// 
#define PIN_LOCKRELAY   2
#define PIN_LIGHTRELAY  3
#define PIN_INSIDEPIR   4
#define PIN_OUTSIDEPIR  5
#define PIN_REEDSWITCH  6
#define PIN_HELLO       LED_BUILTIN

// 
// Configuration constants
// 
#define HZ              4
#define LIGHT_INTERVAL  30
#define LOCK_INTERVAL   5

// 
// Blynk "Virtual Pins"
//
// Blynk uses Virtual pins for pushing events to the cloud and also for
// sending actions to the Thing.
// 
// In this case the sensors, light and lock status are pushed on pins V0,V1
// and V2, while the lights and lock can be actuated by writing to pins V3
// and V4.
//
// Virtual pin v5 is the "whoops, did I leave the front door open" sensor
//
#define VPIN_RD_PIR     V0
#define VPIN_RD_LIGHT   V1
#define VPIN_RD_LOCK    V2

#define VPIN_WR_LIGHT   V3
#define VPIN_WR_LOCK    V4

#define VPIN_RD_REED    V5

boolean movement;   // this is set when either PIR sensor detects movement
boolean opened;     // this is set when the Reed switch detects open-door

// 
// Counters that energize the relays
//
// When an actuation event happens the counter is set to a value, and it is
// decremented at each poll.   When it hits zero, the corresponding relay is
// switched off.
// 

unsigned long light_countdown;
unsigned long lock_countdown;


// 
// Turn on the light relay, and set a countdown
// 
void lighton() {
	digitalWrite(PIN_LIGHTRELAY, HIGH);
	light_countdown = LIGHT_INTERVAL * HZ;
}

// 
// Turn off the light relay and cancel the coutdown
// 
void lightoff() {
	digitalWrite(PIN_LIGHTRELAY, LOW);
	light_countdown = 0;
}

// 
// Turn on the lock relay and set a countdown
// 
void lockopened() {
	digitalWrite(PIN_LOCKRELAY, HIGH);
	lock_countdown = LOCK_INTERVAL * HZ;
}

// 
// Turn off the lock relay and cancel the countdown
// 
void lockclosed() {
	digitalWrite(PIN_LOCKRELAY, LOW);
	lock_countdown = 0;
}
	
// 
// Poll the Input pins 4 times a second, when an action condition is 
// 
void iopoll() 
{
	// 
	// Do we have movement, if so, turn on the lights, and push state to Blynk
	// 
	movement = digitalRead(PIN_INSIDEPIR)||digitalRead(PIN_OUTSIDEPIR);
	digitalWrite(PIN_HELLO, movement);
	Blynk.virtualWrite(VPIN_RD_PIR, movement);

	// 
	// Is the door open?  If so, push state to Blynk
	// 
	opened = digitalRead(PIN_REEDSWITCH);
	Blynk.virtualWrite(VPIN_RD_REED, opened);
	
	// 
	// If the lights are on, decrement the counter.
	// When the counter hits zero, turn off the lights.
	// In either case, push state to Blynk
	// 
	if (light_countdown) {
		--light_countdown;
		if (light_countdown==0) lightoff();
	}
	else if (movement) 
	{
		lighton();
	}
	Blynk.virtualWrite(VPIN_RD_LIGHT, light_countdown);
	
	// 
	// If the lock relay is on, decrement the counter.
	// When the counter hits zero, turn off the relay.
	// In either case, push state to Blynk
	// 
	if (lock_countdown) {
		--lock_countdown;
		if (lock_countdown==0) {
			lockclosed();
		}
	}
	Blynk.virtualWrite(VPIN_RD_LOCK, lock_countdown);
}

// 
// Read/Write hooks for Blynk virtual pins
// 
BLYNK_READ(VPIN_RD_PIR) 
{
	BLYNK_LOG("BR PIR");
	Blynk.virtualWrite(VPIN_RD_PIR, movement);
}

BLYNK_READ(VPIN_RD_LIGHT) 
{
	BLYNK_LOG("BR Light");
	Blynk.virtualWrite(VPIN_RD_LIGHT, light_countdown);
}

BLYNK_READ(VPIN_RD_REED) 
{
	BLYNK_LOG("BR Reed");
	Blynk.virtualWrite(VPIN_RD_LIGHT, opened);
}

BLYNK_READ(VPIN_RD_LOCK) 
{
	BLYNK_LOG("BR LOCK");
	Blynk.virtualWrite(VPIN_RD_LOCK, lock_countdown);
}

BLYNK_WRITE(VPIN_WR_LIGHT) 
{
	BLYNK_LOG("BW Light");
	if (param.asInt()) lighton(); //else lightoff();
}

BLYNK_WRITE(VPIN_WR_LOCK)
{
	BLYNK_LOG("BW Lock");
	if (param.asInt()) lockopened(); //else lockclosed();
}

void setup()
{
	// 
	// Set up lock and light relay pins as outputs, and the sensors as inputs
	// 
	pinMode(PIN_LOCKRELAY, OUTPUT);
	digitalWrite(PIN_LOCKRELAY, LOW);

	pinMode(PIN_LIGHTRELAY, OUTPUT);
	digitalWrite(PIN_LIGHTRELAY, LOW);

	pinMode(PIN_INSIDEPIR, INPUT_PULLUP);
	pinMode(PIN_OUTSIDEPIR, INPUT_PULLUP);
	pinMode(PIN_REEDSWITCH, INPUT_PULLUP);
	pinMode(PIN_HELLO, OUTPUT);

	// 
	// Enable an IO poll operation HZ times per second
	// 
	MsTimer2::set(1000/HZ, iopoll); 
	MsTimer2::start();	

	// 
	// Activate Blynk
	// 
	Blynk.begin(auth);
	BLYNK_LOG("Blynk Here");
}

void loop()
{
	Blynk.run();
}

