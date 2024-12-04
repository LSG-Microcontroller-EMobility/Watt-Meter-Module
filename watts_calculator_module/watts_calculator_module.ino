
//20V-- - R1(8.2k?) -- - +-- - R2(1.7k?) -- - GND
//						|
//				Vout(circa 3.4V)

//#define SERIAL_DEBUG

#include <Wire.h>  

#include <LiquidCrystal_I2C.h>

#include <SoftwareSerial.h>

SoftwareSerial sf(99,8);

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, if it's not working try 0x27..

bool  check = true;

float volatile ampere = 0.00f;

float volatile volts = 0.00f;

float volatile total_watts_H = 0.00f;

float volatile total_watts_to_transmit = 0.00f;

float volatile amps_to_transmit = 0.00f;

void lcd_print_message(char message[8], uint8_t row, uint8_t col, bool clear, uint8_t clear_lenght = 0);

volatile float delta = 0.00f;

char watts_to_display[7]{ '\0' };

char voltage_to_display[6]{ '\0' };

char amps_to_display[7]{ '\0' };

char watts_h_to_display[5]{ '\0' };

float volatile value;

const float volatile aref_voltage = 4.55f;

void setup() {

	Serial.begin(9600);

	sf.begin(600);

#ifdef SERIAL_DEBUG
	Serial.println(F("start"));
#endif // DEBUG

	lcd.begin(16, 2);   // iInit the LCD for 16 chars 2 lines

	lcd.clear();

	analogReference(EXTERNAL);

	//Scroll_Function_Lcd_Message("check system");

	//remove this if use scroll, used to delay read from current sensor.
	delay(2000);

	lcd_print_message("w: ", 0, 0, false); lcd_print_message("|", 0, 8, false,0); lcd_print_message("v: ", 0, 9, false);

	lcd_print_message("a: ", 1, 0, false); lcd_print_message("|", 1, 8, false); lcd_print_message("wh: ", 1, 9, false);

	for (int i = 0; i < 100; i++)
	{
		delta = delta + ((aref_voltage / 1024.00f) * analogRead(A0));
	}

	delta = delta / 100.00f;

	

	//// Imposta Timer1 in modalità CTC
	TIMSK1 = (1 << OCIE1A);          // Abilita l'interrupt di confronto A

	TCCR1A = 0b00000001; // fast pwm                     // Modalità CTC su Timer1 (WGM12 = 1 in TCCR1B)

	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // WGM12 = 1, prescaler 64

	OCR1A = 250;                     // Imposta il valore di confronto per 1 ms

	attachInterrupt(0, activate_system_interrupt, FALLING);

}

void loop() {
#ifdef SERIAL_DEBUG
	print_to_console();
#endif // DEBUG

	lcd_print_message();
	
	send_data();
}

void print_to_console()
{
	Serial.println(F("---"));
	Serial.print(F("volts : ")); Serial.println(volts);
	Serial.print(F("ampere : ")); Serial.println((ampere));
	Serial.print(F("total_watts_H : ")); Serial.println(total_watts_H);
	Serial.print(F("watts : ")); Serial.println(((ampere) * volts));
	Serial.println(F("---"));
}

void lcd_print_message()
{
	dtostrf((ampere * volts), 6, 1, watts_to_display);

	//itoa((ampere * volts), watts, 10);
	//itoa(volts, voltage, 10);

	dtostrf(volts, 5, 2, voltage_to_display);

	dtostrf(ampere, 6, 2, amps_to_display);

	//dtostrf(total_watts_H, 4, 0, watts_h);\

	itoa(total_watts_H, watts_h_to_display, 10);

	lcd_print_message(watts_to_display, 0, 2, true, 4);

	lcd_print_message(voltage_to_display, 0, 11, true, 4);

	lcd_print_message(amps_to_display, 1, 2, true, 5);

	lcd_print_message(watts_h_to_display, 1, 12, true, 4);

}

void send_data()
{
	sf.print('*');
	sf.print(amps_to_transmit);
	//sf.print(10.12f);
	sf.print('\0');
	sf.print(total_watts_to_transmit);
	//sf.print(1500.13f);
	delay(200);
}

void lcd_print_message(char message[8], uint8_t row, uint8_t col, bool clear,uint8_t clear_lenght = 0)
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

	//delay(100);
}

void scroll_function_lcd_message(String message) {

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

unsigned long volatile count = 0;
	
ISR(TIMER1_COMPA_vect)
{
	
		value = ((aref_voltage / 1024.00f) * analogRead(A0));

		//value += ((aref_voltage / 1024.00f) * analogRead(A0));

		//value += ((aref_voltage / 1024.00f) * analogRead(A0));

		ampere = ((value /*/ 2*/) - delta) < 0.055f ? 0.00f : ((value /*/ 2*/) - delta);

		ampere = (ampere / 0.080f);

		volts = (((aref_voltage / 1024.00f) * analogRead(A1)));

		//volts += (((aref_voltage / 1024.00f) * analogRead(A1)));

		//volts += (((aref_voltage / 1024.00f) * analogRead(A1)));

		volts = ((volts /*/ 2*/) * (20.00f / aref_voltage)) + 60.00f;
		
		total_watts_H += ((ampere) / 3600000.00f) * volts;

		/*if (count == 999)
		{
			count = 0;
			Serial.println(millis());
		}
		else
		{
			count++;
		}*/

}

void activate_system_interrupt()
{

	TIMSK1 &= ~(1 << OCIE1A);  // Clear del bit OCIE1A per disabilitare l'interrupt del Timer1

	//Serial.println("inter...");

	amps_to_transmit = ampere;
	total_watts_to_transmit = total_watts_H;

	/*amps_to_transmit = 50.00f;
	total_watts_to_transmit = 1500;*/

	
#ifdef SERIAL_DEBUG
	Serial.println(F("----------    interrupt --------------"));
	Serial.print(F("send watts: ")); Serial.println(total_watts_to_transmit);
	Serial.print(F("send amps: ")); Serial.println(amps_to_transmit);
#endif // _DEBUG

	TIMSK1 |= (1 << OCIE1A);  // Set del bit OCIE1A per riabilitare l'interrupt del Timer1
}






