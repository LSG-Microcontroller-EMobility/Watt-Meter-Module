#include <avr/io.h>

#include <Wire.h>  

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, if it's not working try 0x27..

double volatile  pulse = 0;

bool  check = true;

float volatile delta_volts_ampere = 0.060f;

float volatile ampere = 0.00f;

float volatile volts = 0.00f;

double volatile total_watts_H = 0.00f;

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

	batteryVoltageLcdReport1("testo di prova");

	lcdPrintMessage("W: ", 0, 0, false); lcdPrintMessage("|", 0, 7, false); lcdPrintMessage("V: ", 0, 8, false);

	lcdPrintMessage("A: ", 1, 0, false); lcdPrintMessage("|", 1, 7, false);
}

//int volatile counter_test = 0;

void loop() {

	//delay(900000); //15 minuti test

	char watts[5]{};

	char amps[8]{};

	char voltage[5]{};

	itoa(total_watts_H, watts, 10);

	dtostrf(ampere, 5, 2, amps);

	itoa(volts, voltage, 10);

	// prepared for test with higher precision
	/*pulse = 0;
	delay(1000);
	Serial.print(counter_test);
	while (1);*/
	Serial.print("watts/h: "); Serial.print(total_watts_H); Serial.print(" amps: "); Serial.print(ampere); Serial.print(" volts: "); Serial.println(volts);

	lcdPrintMessage(watts, 0, 2, (total_watts_H < 10) ? false : false);

	lcdPrintMessage(voltage, 0, 10, (total_watts_H < 10) ? false : false);

	lcdPrintMessage(amps, 1, 2, (ampere < 10) ? false : false);

	//while (1);

	//delay(54000);
	//if (ampere > 20) {
	//	digitalWrite(8, LOW);
	//}
	//else {
	//	digitalWrite(8, HIGH);
	//}
}

void lcdPrintMessage(char message[16], uint8_t row, uint8_t col, bool clear)
{
	char c = ' ';

	uint8_t index = 0;

	if (clear)
	{
		while (c != '\0')
		{
			c = message[index];

			lcd.setCursor(col + index, row); lcd.print(" ");

			index++;
		}
	}

	lcd.setCursor(col, row); //First line

	lcd.print(message);

	delay(100);

	//if(clear) delay(200);
}

void batteryVoltageLcdReport1(String message) {

	lcd.clear();

	//lcd.begin(16, 2);   // iInit the LCD for 16 chars 2 lines

	//lcd.backlight();   // Turn on the backligt (try lcd.noBaklight() to turn it off)

	lcd.setCursor(16, 0); //First line

	lcd.scrollDisplayLeft();

	lcd.autoscroll();

	for (int thisChar = 0; thisChar < message.length(); thisChar++) {

		lcd.print(message.charAt(thisChar));

		delay(350);
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






