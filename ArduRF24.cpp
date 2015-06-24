#include "ArduRF24.h"
#include <printf.h>
#include <RF24.h>
#include <Button.h>

#define TX_RX_PIN 5
#define OUT_PIN 7
#define BUTTON_PIN 4

#define DEBOUNCE_MS 20
#define OUT_BLINK_TIME_MS 250
#define DELAY_BLINK_MS 1500

typedef enum ETX_STATE
{
	BTN_PRESS_TX_ST,
	SEND_TX_ST,
};

typedef enum EBLINK_STATE
{
	INIT_BLINK_ST,
	DELAY_BLINK_ST,
};

//typedef enum ERX_STATE
//{
//	RECEIVE_ST
//};

// SG: NRF24 Hardware initialization (Pin 9 -> , Pin 10 ->)
RF24 gNRF24(9,10);
Button gtBtn(BUTTON_PIN, true, true, DEBOUNCE_MS);

ETX_STATE gtTXState = BTN_PRESS_TX_ST;
EBLINK_STATE gtBlinkState = INIT_BLINK_ST;
// ERX_STATE gRXState = RECEIVE_ST;

unsigned long ulBlinkOldTime;
unsigned long ulActiveBlinkOldTime;

byte grgcAddress[][6] = {"1Node", "2Node"};
byte gbyCounter = 1;

const byte gbyTOGGLE = 0xAA;

// SG: false -> RX, true -> TX
bool gbRadioType;
bool gbOutState = false;
bool gbFailBlink = false;
bool gbOkBlink = false;

void TX_FSM()
{
	byte byBtn = gtBtn.wasPressed();

	if(byBtn)
	{
		Serial.println(F("ArduRF24: Start sending"));

		bool bRet = gNRF24.write(&gbyTOGGLE, sizeof(gbyTOGGLE));

		if(bRet)
		{
			Serial.println(F("ArduRF24: Sending ok"));

			gbOkBlink = true;

			return;
		}

		Serial.println(F("ArduRF24: Sending failure"));

		gbFailBlink = true;
	}



	switch(gtTXState)
	{
		case BTN_PRESS_TX_ST:
		{

		}
		break;
		case SEND_TX_ST:
		{
			bool bTXOk;
			bool bFail;
			bool bVar;

			gNRF24.whatHappened(bTXOk, bFail, bVar);
			if(bTXOk || bFail)
			{
				Serial.println(F("ArduRF24: Send finished"));

				gtTXState = BTN_PRESS_TX_ST;
			}
		}
		break;
	}
}

void RX_FSM()
{
	byte byToggle;
	byte byPipe;

	if(gNRF24.available(&byPipe))
	{
		if(1 != byPipe)
		{
			return;
		}

		gNRF24.read(&byToggle, sizeof(byToggle));

		if(gbyTOGGLE == byToggle)
		{
			gbBlink = true;
		}
	}
}

void ActiveBlink()
{
	switch(gtBlinkState)
	{
		case INIT_BLINK_ST:
		{
			if(gbBlink)
			{
				ulActiveBlinkOldTime = millis();

				gtBlinkState = DELAY_BLINK_ST;
			}
		}
		break;
		case DELAY_BLINK_ST:
		{
			if((millis() - ulActiveBlinkOldTime) > DELAY_BLINK_MS)
			{
				gbBlink = false;

				gtBlinkState = DELAY_BLINK_ST;
			}
		}
		break;
	}
}

void BlinkLed()
{
	if((millis() - ulBlinkOldTime) > OUT_BLINK_TIME_MS)
	{
		ulBlinkOldTime = millis();
		if(gbBlink)
		{
			gbOutState = gbOutState ? false : true;

			Serial.print(F("ArduRF24: "));
			Serial.println(gbOutState);

			if(gbOutState)
			{
				digitalWrite((int)OUT_PIN, HIGH);

				return;
			}

			digitalWrite((int)OUT_PIN, LOW);

			return;
		}

		digitalWrite((int)OUT_PIN, LOW);
	}
}

void setup()
{
	/* add setup code here */
	printf_begin();
	Serial.begin(115200);

	pinMode(TX_RX_PIN, INPUT_PULLUP);
	// pinMode(BTN_PIN, INPUT);

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

		pinMode(OUT_PIN, OUTPUT);
		digitalWrite(OUT_PIN, false);
	}

	gNRF24.startListening();
	// gNRF24.writeAckPayload(1, &gbyCounter, 1);

	delay(100);

	gNRF24.printDetails();
	Serial.println(F("ArduRF24: Setup finished"));

	ulBlinkOldTime = millis();
}

void loop()
{
	/* add main program code here */
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
	BlinkLed();
}
