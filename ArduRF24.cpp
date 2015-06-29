#include "ArduRF24.h"
#include <printf.h>
#include <RF24.h>
#include <Button.h>

#define TX_RX_PIN 5
#define OUT_PIN 7
#define BUTTON_PIN 4

#define DEBOUNCE_MS 20
#define BLINK_BASE_TIMER_MS 50
#define FAST_BLINK_CNT 2
#define SLOW_BLINK_CNT 5
#define BLINK_TOUT_MS 1500
#define DEBUG_STR F("ArduRF24: ")

typedef enum EBLINK_STATE
{
	INIT_BLINK_ST,
	BLINK_ST,
};

typedef enum EBLINK_TYPE
{
	NO_BLINK,
	SLOW_BLINK,
	FAST_BLINK
};

// SG: NRF24 Hardware initialization (Pin 9 -> , Pin 10 ->)
RF24 gNRF24(9,10);
Button gtBtn(BUTTON_PIN, true, true, DEBOUNCE_MS);

EBLINK_STATE gtBlinkState = INIT_BLINK_ST;
EBLINK_TYPE gtBlink = NO_BLINK;

unsigned long gulMillis;
unsigned long gulStopBlinkOldTime;
unsigned long gulBaseOldTime;
int giBlinkCnt;
int giMaxBlinkCnt;

byte grgcAddress[][6] = {"1Node", "2Node"};
byte gbyCounter = 1;

const byte gbyTOGGLE = 0xAA;

// SG: false -> RX, true -> TX
bool gbRadioType;
bool gbOutState = false;

void TX_FSM()
{
	byte byBtn = gtBtn.wasPressed();

	if(byBtn)
	{
		gNRF24.stopListening();

		Serial.print(DEBUG_STR);
		Serial.println(F("Start sending"));

		bool bRet = gNRF24.write(&gbyTOGGLE, sizeof(gbyTOGGLE));

		if(bRet)
		{
			Serial.print(DEBUG_STR);
			Serial.println(F("Sending ok"));

			gtBlink = FAST_BLINK;

			return;
		}

		Serial.print(DEBUG_STR);
		Serial.println(F("Sending failure"));

		gtBlink = SLOW_BLINK;

		return;
	}
}

void RX_FSM()
{
	byte byToggle;
	byte byPipe;

	if(gNRF24.available(&byPipe))
	{
		Serial.print(DEBUG_STR);
		Serial.print("Received data on ");
		Serial.println(byPipe);

		if(1 != byPipe)
		{
			return;
		}

		gNRF24.read(&byToggle, sizeof(byToggle));

		if(gbyTOGGLE == byToggle)
		{
			gtBlink = FAST_BLINK;

			return;
		}

		gtBlink = SLOW_BLINK;
	}
}

void ActiveBlink()
{
	switch(gtBlinkState)
	{
		case INIT_BLINK_ST:
		{
			if(NO_BLINK != gtBlink)
			{
				gulBaseOldTime = gulStopBlinkOldTime = gulMillis;
				gbOutState = true;

				switch(gtBlink)
				{
					case SLOW_BLINK:
					{
						giBlinkCnt = giMaxBlinkCnt = SLOW_BLINK_CNT;
					}
					break;
					case FAST_BLINK:
					{
						giBlinkCnt = giMaxBlinkCnt = FAST_BLINK_CNT;
					}
					break;
				}

				gtBlink = NO_BLINK;

				digitalWrite(OUT_PIN, HIGH);

				gtBlinkState = BLINK_ST;
			}
		}
		break;
		case BLINK_ST:
		{
			if((gulMillis - gulBaseOldTime) > BLINK_BASE_TIMER_MS)
			{
				gulBaseOldTime = gulMillis;

				// Serial.println(giBlinkCnt);

				if(giBlinkCnt > 0)
				{
					--giBlinkCnt;
				}
				else
				{
					giBlinkCnt = giMaxBlinkCnt;
					gbOutState = (gbOutState ? false : true);

					if(gbOutState)
					{
						digitalWrite(OUT_PIN, HIGH);
					}
					else
					{
						digitalWrite(OUT_PIN, LOW);
					}
				}
			}

			if((gulMillis - gulStopBlinkOldTime) > BLINK_TOUT_MS)
			{
				digitalWrite(OUT_PIN, LOW);

				gtBlinkState = INIT_BLINK_ST;
			}
		}
		break;
	}
}

void setup()
{
	/* add setup code here */
	printf_begin();
	Serial.begin(115200);
	pinMode(TX_RX_PIN, INPUT_PULLUP);

	delay(100);

	gNRF24.begin();
	gNRF24.enableAckPayload();
	gNRF24.enableDynamicPayloads();
	gbRadioType = digitalRead(TX_RX_PIN);

	if(gbRadioType)
	{
		gNRF24.openWritingPipe(grgcAddress[1]);
		gNRF24.openReadingPipe(1, grgcAddress[0]);
	}
	else
	{
		gNRF24.openWritingPipe(grgcAddress[0]);
		gNRF24.openReadingPipe(1, grgcAddress[1]);
		gNRF24.startListening();
		pinMode(OUT_PIN, OUTPUT);

		delay(100);

		digitalWrite(OUT_PIN, false);
	}

	gNRF24.startListening();
	gNRF24.printDetails();

	Serial.print(DEBUG_STR);
	Serial.print(F("Mode: "));
	if(gbRadioType)
	{
		Serial.println(F("TX"));
	}
	else
	{
		Serial.println(F("RX"));
	}

	Serial.print(DEBUG_STR);
	Serial.println(F("Setup finished"));

	gulBaseOldTime = millis();
}

void loop()
{
	/* add main program code here */
	gulMillis = millis();
	gtBtn.read();

	if(gbRadioType)
	{
		TX_FSM();
	}
	else
	{
		RX_FSM();
	}

	ActiveBlink();
}
