#include <RF24.h>
#include <Button.h>

#define TX_RX_PIN 5
#define OUT_PIN 13
#define BUTTON_PIN 2
#define DEBOUNCE_MS 20

typedef enum ETX_STATE
{
	BTN_PRESS_ST,
	SEND_ST,
};

//typedef enum ERX_STATE
//{
//	RECEIVE_ST
//};

// SG: NRF24 Hardware initialization (Pin 9 -> , Pin 10 ->)
RF24 gNRF24(9,10);
Button gtBtn(BUTTON_PIN, true, true, DEBOUNCE_MS);

ETX_STATE gtTXState = BTN_PRESS_ST;
// ERX_STATE gRXState = RECEIVE_ST;

byte grgcAddress[][6] = {"1Node", "2Node"};
byte gbyCounter = 1;

const byte gbyTOGGLE = 0xAA;

// SG: false -> RX, true -> TX
bool gbRadioType;
bool gbOutState = false;

void TX_FSM()
{
	switch(gtTXState)
	{
		case BTN_PRESS_ST:
		{
			byte byBtn = gtBtn.wasPressed();

			if(byBtn)
			{
				Serial.println(F("Start send"));

				gNRF24.startWrite(&gbyTOGGLE, sizeof(gbyTOGGLE), 0);

				gtTXState = SEND_ST;
			}
		}
		break;
		case SEND_ST:
		{
			bool bTXOk;
			bool bFail;
			bool bVar;

			gNRF24.whatHappened(bTXOk, bFail, bVar);
			if(bTXOk || bFail)
			{
				Serial.println(F("Send finished"));

				gtTXState = BTN_PRESS_ST;
			}
		}
		break;
	}
}

void RX_FSM()
{
	byte byToggle;

	if(gNRF24.available())
	{
		gNRF24.read(&byToggle, sizeof(byToggle));

		if(gbyTOGGLE == byToggle)
		{
			gbOutState ^= 1;
			digitalWrite(OUT_PIN, gbOutState);
		}
	}
}


void setup()
{
	/* add setup code here */
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
	Serial.println(F("Setup finished"));
}

void loop()
{
	/* add main program code here */
	gtBtn.read();

	if(!gbRadioType)
	{
		TX_FSM();
	}
	else
	{
		RX_FSM();
	}
}
