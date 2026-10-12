//20V-- - R1(8.2k?) -- - +-- - R2(1.7k?) -- - GND					|
//				Vout(circa 3.4V)
// Debug del monitor seriale: 0 = produzione, 1 = debug a 9600 baud.
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL 0
#endif

#if DEBUG_SERIAL
#define DEBUG_SERIAL_BEGIN(...) do { Serial.begin(__VA_ARGS__); } while (0)
#define DEBUG_SERIAL_PRINT(...) do { Serial.print(__VA_ARGS__); } while (0)
#define DEBUG_SERIAL_PRINTLN(...) do { Serial.println(__VA_ARGS__); } while (0)
#else
#define DEBUG_SERIAL_BEGIN(...) do { } while (0)
#define DEBUG_SERIAL_PRINT(...) do { } while (0)
#define DEBUG_SERIAL_PRINTLN(...) do { } while (0)
#endif

#include <Wire.h>  
#include <LiquidCrystal_I2C.h>
#include "src/external_library/SoftwareSerialTxOnly.h"
SoftwareSerialTxOnly sf(0xFF, 8); // RX disabled, TX on pin 8
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, if it's not working try 0x27..
bool  check = true;
float volatile ampere = 0.00f;
float volatile volts = 0.00f;
float volatile total_watts_H = 0.00f;
float volatile total_watts_to_transmit = 0.00f;
float volatile amps_to_transmit = 0.00f;
#if DEBUG_SERIAL
volatile bool transmission_debug_pending = false;
#endif
void lcd_print_message(char message[8], uint8_t row, uint8_t col, bool clear, uint8_t clear_lenght = 0);
#if DEBUG_SERIAL
void print_to_console();
void print_transmission_debug_if_pending();
#endif
volatile float delta = 0.00f;
char watts_to_display[7]{ '\0' };
char voltage_to_display[6]{ '\0' };
char amps_to_display[7]{ '\0' };
char watts_h_to_display[5]{ '\0' };
const float volatile aref_voltage = 4.55f;
const float aref_step = aref_voltage / 1024.00f;
const float ext_voltage_step = 20.00f / aref_voltage;
void setup() {
	DEBUG_SERIAL_BEGIN(9600);
	sf.begin(600);
	DEBUG_SERIAL_PRINTLN(F("start"));
	lcd.begin(16, 2);   // iInit the LCD for 16 chars 2 lines
	lcd.clear();
	analogReference(EXTERNAL);
	//Scroll_Function_Lcd_Message("check system");
	//remove this if use scroll, used to delay read from current sensor.
	delay(2000);
	lcd_print_message("w: ", 0, 0, false); lcd_print_message("|", 0, 8, false, 0); lcd_print_message("v: ", 0, 9, false);
	lcd_print_message("a: ", 1, 0, false); lcd_print_message("|", 1, 8, false); lcd_print_message("wh: ", 1, 9, false);
	for (int i = 0; i < 100; i++) {
		delta = delta + ((aref_voltage / 1024.00f) * analogRead(A0));
	}
	delta = delta / 100.00f;
	//// Imposta Timer1 in modalità CTC
	TIMSK1 = (1 << OCIE1A);          // Abilita l'interrupt di confronto A
	//TCCR1A = 0b00000001; // fast pwm     
	// 
	TCCR1A = 0;              // Modalità CTC su Timer1 (WGM12 = 1 in TCCR1B)
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // WGM12 = 1, prescaler 64
	OCR1A = 163;//ATTENZIONE:: dovrebbe essere 250 ma troppo lento            // Imposta il valore di confronto per 1 ms
	pinMode(2, INPUT_PULLUP);
	attachInterrupt(0, activate_system_interrupt, FALLING);
}
void loop() {
#if DEBUG_SERIAL
	print_to_console();
	print_transmission_debug_if_pending();
#endif
	lcd_print_message();
	send_data();
}
#if DEBUG_SERIAL
void print_to_console() {
	static unsigned long lastPrint = 0;
	if (millis() - lastPrint < 1000) return;
	lastPrint = millis();
	noInterrupts();
	const float debug_volts = volts;
	const float debug_ampere = ampere;
	const float debug_total_watts_h = total_watts_H;
	interrupts();
	DEBUG_SERIAL_PRINTLN(F("---"));
	DEBUG_SERIAL_PRINT(F("volts : ")); DEBUG_SERIAL_PRINTLN(debug_volts);
	DEBUG_SERIAL_PRINT(F("ampere : ")); DEBUG_SERIAL_PRINTLN(debug_ampere);
	DEBUG_SERIAL_PRINT(F("total_watts_H : ")); DEBUG_SERIAL_PRINTLN(debug_total_watts_h);
	DEBUG_SERIAL_PRINT(F("watts : ")); DEBUG_SERIAL_PRINTLN(debug_ampere * debug_volts);
	DEBUG_SERIAL_PRINTLN(F("---"));
	
}
void print_transmission_debug_if_pending() {
	noInterrupts();
	const bool should_print = transmission_debug_pending;
	transmission_debug_pending = false;
	const float watts = total_watts_to_transmit;
	const float amps = amps_to_transmit;
	interrupts();

	if (!should_print) return;

	DEBUG_SERIAL_PRINTLN(F("----------    interrupt --------------"));
	DEBUG_SERIAL_PRINT(F("send watts: ")); DEBUG_SERIAL_PRINTLN(watts);
	DEBUG_SERIAL_PRINT(F("send amps: ")); DEBUG_SERIAL_PRINTLN(amps);
}
#endif
void lcd_print_message() {
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
void send_data() {
	sf.print('*');
	sf.print(amps_to_transmit);
	//sf.print(10.12f);
	sf.print('\0');
	sf.print(total_watts_to_transmit);
	//sf.print(1500.13f);
	delay(200);
}
void lcd_print_message(char message[8], uint8_t row, uint8_t col, bool clear, uint8_t clear_lenght = 0) {
	if (clear) {
		for (int i = 0; i < clear_lenght; i++) {
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
ISR(TIMER1_COMPA_vect) {
	ampere = (aref_step * analogRead(A0));
	volts = (aref_step * analogRead(A1));
	ampere = (ampere - delta) < 0.055f ? 0.00f : ((ampere - delta) / 0.080f);
	volts = (volts * ext_voltage_step) + 60.00f;
	total_watts_H += (ampere / 3600000) * volts;
}
void activate_system_interrupt() {
	TIMSK1 &= ~(1 << OCIE1A);  // Clear del bit OCIE1A per disabilitare l'interrupt del Timer1
	amps_to_transmit = ampere;
	total_watts_to_transmit = total_watts_H;
	/*amps_to_transmit = 50.00f;
	total_watts_to_transmit = 1500;*/
#if DEBUG_SERIAL
	transmission_debug_pending = true;
#endif
	TIMSK1 |= (1 << OCIE1A);  // Set del bit OCIE1A per riabilitare l'interrupt del Timer1
}






