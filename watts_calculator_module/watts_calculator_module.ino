#include <avr/io.h>

#include <Wire.h>  

#include <LiquidCrystal_I2C.h>

#include <SPI.h>

#include <SD.h>

#include <SoftwareSerial.h>

SoftwareSerial sf(99,4);

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, if it's not working try 0x27..

double volatile  pulse = 0;

bool  check = true;

float volatile delta_volts_ampere = 0.060f;

float volatile ampere = 0.00f;

float volatile volts = 0.00f;

double volatile total_watts_H = 0.00f;

double volatile total_watts_to_transmit = 0.00f;

double volatile amps_to_transmit = 0.00f;

void lcdPrintMessage(char message[16], uint8_t row, uint8_t col, bool clear, uint8_t clear_lenght = 0);

File myFile;

String fileName = "test.txt";

void setup() {

	Serial.begin(9600);

	lcd.begin(16, 2);   // iInit the LCD for 16 chars 2 lines

	lcd.clear();

	TIMSK1 = _BV(OCIE1A); // Output Compare Match A Interrupt Enable

	//DDRB |= (1 << PB0);
	TCCR1A = 0b00000001; // fast pwm

	TCCR1B = 0b00001010; // x1

	//PORTB &= (1 << PB0);
	pinMode(8, OUTPUT);

	digitalWrite(8, HIGH);

	analogWrite(9, 127);

	analogReference(EXTERNAL);

	batteryVoltageLcdReport1("check system");

	lcdPrintMessage("w: ", 0, 0, false); lcdPrintMessage("|", 0, 7, false,0); lcdPrintMessage("v: ", 0, 8, false);

	lcdPrintMessage("a: ", 1, 0, false); lcdPrintMessage("|", 1, 7, false); lcdPrintMessage("w/h: ", 1, 8, false);

	sd_card_initialize();

	pinMode(3, INPUT_PULLUP);

	attachInterrupt(1, activateSystemInterrupt,FALLING);

}

//int volatile counter_test = 0;

void sd_card_initialize(){
	// Open serial communications and wait for port to open:
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
	}
	Serial.print("Initializing SD card...");

	if (!SD.begin(4)) {
		Serial.println("initialization failed!");
		while (1);
	}
	Serial.println("initialization done.");

	if (SD.exists(fileName))
	{
		SD.remove(fileName);
	}
}

void loop() {

	//delay(900000); //15 minuti test

	char watts[5]{};

	char amps[8]{};

	char voltage[5]{};

	char watts_h[5]{};

	itoa(total_watts_H, watts, 10);

	dtostrf(ampere, 5, 2, amps);

	itoa(volts, voltage, 10);

	itoa((ampere * volts), watts_h, 10);

	// prepared for test with higher precision
	/*pulse = 0;
	delay(1000);
	Serial.print(counter_test);
	while (1);*/
	Serial.print("watts: "); Serial.print(total_watts_H); Serial.print(" amps: "); Serial.print(ampere); Serial.print(" volts: "); Serial.println(volts);

	lcdPrintMessage(watts, 0, 2, true, 4);

	lcdPrintMessage(voltage, 0, 10, true, 4);

	lcdPrintMessage(amps, 1, 2, true, 5);

	lcdPrintMessage(watts_h, 1, 12, true, 4);

	// transmitt data to serial.

	//while (1);
}

void lcdPrintMessage(char message[16], uint8_t row, uint8_t col, bool clear,uint8_t clear_lenght = 0)
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

void batteryVoltageLcdReport1(String message) {

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
	pulse++;

	//frequenza impostata su timer1 a 7.8khz 
	if ((pulse >= (8000.00 / 1000.00))) {

		//counter_test++;
		ampere = ((3.30f / 1024.00f) * analogRead(A0)) / delta_volts_ampere;

		//calcolo teorico per impostare test su delata di 20Volts con zener da 60V su batteria da 72V
		volts = (ampere * (-0.4)) + 80;

		//volts = (((3.30f / 1024.00f) * analogRead(A1)) * 6.00f) + 60.00f;
		total_watts_H = total_watts_H + ((ampere / 3600000) * volts);

		pulse = 0;
	}
}

void activateSystemInterrupt()
{
	amps_to_transmit = ampere;
	total_watts_to_transmit = total_watts_H;
}






