
#include <Wire.h>  

#include <LiquidCrystal_I2C.h>

#include <SoftwareSerial.h>

SoftwareSerial sf(99,4);

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, if it's not working try 0x27..

bool  check = true;

float volatile ampere = 0.00f;

float volatile volts = 0.00f;

double volatile total_watts_H = 0.00f;

float volatile total_watts_to_transmit = 0.00f;

float volatile amps_to_transmit = 0.00f;

void Lcd_Print_Message(char message[16], uint8_t row, uint8_t col, bool clear, uint8_t clear_lenght = 0);

volatile float delta = 0.00f;


void setup() {

	Serial.begin(9600);

	Serial.println(F("start"));

	sf.begin(600);

	lcd.begin(16, 2);   // iInit the LCD for 16 chars 2 lines

	lcd.clear();

	for (int i = 0; i < 100; i++)
	{
		delta = delta + ((3.25f / 1024.00f) * analogRead(A0));
	}

	delta = delta / 100.00f;

	

	//TIMSK1 = _BV(OCIE1A); // Output Compare Match A Interrupt Enable

	////DDRB |= (1 << PB0);
	//TCCR1A = 0b00000001; // fast pwm

	//TCCR1B = 0b00001010; // x1

	//PORTB &= (1 << PB0);
	pinMode(8, OUTPUT);

	digitalWrite(8, HIGH);

	analogWrite(9, 127);

	analogReference(EXTERNAL);

	//Scroll_Function_Lcd_Message("check system");

	Lcd_Print_Message("w: ", 0, 0, false); Lcd_Print_Message("|", 0, 7, false,0); Lcd_Print_Message("v: ", 0, 8, false);

	Lcd_Print_Message("a: ", 1, 0, false); Lcd_Print_Message("|", 1, 7, false); Lcd_Print_Message("w/h: ", 1, 8, false);

	pinMode(3, INPUT_PULLUP);

	attachInterrupt(1, Activate_System_Interrupt,FALLING);

	// Imposta Timer1 in modalità CTC
	TIMSK1 = (1 << OCIE1A);          // Abilita l'interrupt di confronto A

	TCCR1A = 0b00000001; // fast pwm                     // Modalità CTC su Timer1 (WGM12 = 1 in TCCR1B)

	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // WGM12 = 1, prescaler 64

	OCR1A = 250;                     // Imposta il valore di confronto per 1 ms

	
}

//int volatile counter_test = 0;

//void sd_card_initialize(){
//	// Open serial communications and wait for port to open:
//	while (!Serial) {
//		; // wait for serial port to connect. Needed for native USB port only
//	}
//	Serial.print("Initializing SD card...");
//
//	if (!SD.begin(4)) {
//		Serial.println("initialization failed!");
//		while (1);
//	}
//	Serial.println("initialization done.");
//
//	if (SD.exists(fileName))
//	{
//		SD.remove(fileName);
//	}
//}

void loop() {

	Serial.println(F("-------------------------------------")); 
	Serial.print(F("volts : ")); Serial.println(volts);
	Serial.print(F("ampere : ")); Serial.println((ampere));
	Serial.print(F("total_watts_H : ")); Serial.println(total_watts_H);
	Serial.print(F("watts : ")); Serial.println(((ampere) * volts));
	Serial.println(F("-------------------------------------"));

	char watts[5]{'\0'};

	char amps[8]{};

	char voltage[5]{};

	char watts_h[5]{};

	itoa(total_watts_H, watts_h, 10);

	dtostrf(ampere, 5, 2, amps);

	itoa(volts, voltage, 10);

	//dtostrf((ampere * volts), 5, 1, watts);

	itoa((ampere * volts), watts, 10);

	// prepared for test with higher precision
	/*pulse = 0;
	delay(1000);
	Serial.print(counter_test);
	while (1);*/
	//Serial.print("watts: "); Serial.print(total_watts_H); Serial.print(" amps: "); Serial.print(ampere); Serial.print(" volts: "); Serial.println(volts);
	
	Lcd_Print_Message(watts, 0, 2, true, 4);

	Lcd_Print_Message(voltage, 0, 10, true, 4);

	Lcd_Print_Message(amps, 1, 2, true, 5);

	Lcd_Print_Message(watts_h, 1, 12, true, 4);



	// transmitt data to serial.

	Send_Data();

	//while (1);
}

void Send_Data()
{
	//Serial.println(F("send data"));
	sf.print('*');
	sf.print(amps_to_transmit);
	sf.print('\0');
	sf.print(total_watts_to_transmit);
	delay(200);
}

void Lcd_Print_Message(char message[5], uint8_t row, uint8_t col, bool clear,uint8_t clear_lenght = 0)
{
	if (clear)
	{
		for (int i = 0; i < clear_lenght; i++)
		{
			lcd.setCursor(col + i, row); lcd.print(" ");
		}
	}

	lcd.setCursor(col, row); //First line

	lcd.print(message);

	delay(100);
}

void Scroll_Function_Lcd_Message(String message) {

	lcd.clear();

	lcd.backlight();   // Turn on the backligt (try lcd.noBaklight() to turn it off)

	lcd.setCursor(16, 0); //First line

	lcd.scrollDisplayLeft();

	lcd.autoscroll();

	for (int thisChar = 0; thisChar < message.length(); thisChar++) {

		lcd.print(message.charAt(thisChar));

		delay(250);
	}
	delay(2000);

	lcd.noAutoscroll();

	lcd.clear();
}


ISR(TIMER1_COMPA_vect)
{
		float value = ((3.25f / 1024.00f) * analogRead(A0)) - delta;

		ampere = value < 0.055f ? 0.00f : value;

		ampere = (ampere / 0.060f);

		//todo: IMPORTANTE MODIFICARE ZENER CON UN 3.9v
		volts = (((3.25f / 1024.00f) * analogRead(A1)));
		
		total_watts_H += ((ampere) / 3600000) * volts;
}

void Activate_System_Interrupt()
{
	amps_to_transmit = ampere;
	total_watts_to_transmit = total_watts_H;

#ifdef _DEBUG
	Serial.println(F("interrupt"));
	Serial.print(F("send watts: ")); Serial.println(total_watts_to_transmit);
	Serial.print(F("send amps: ")); Serial.println(amps_to_transmit);
#endif // _DEBUG

}






