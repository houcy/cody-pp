/*
Ausgangstreiber f�r ESP32-Fischertechnik-Anbindung
Autor: Johannes Marquart
*/
#include "ft_ESP32_IOobjects.h"

/*
//Prototypen zur Simulation f�r VisualStudio
void ledcAttachPin(int, int) {}
void ledcSetup(int, int, int) {}
void ledcWrite(int, int) {}
void pinMode(int, int) {}
void digitalWrite(int, int) {}
unsigned int digitalRead(unsigned int) { return 0; }
unsigned int analogRead(unsigned int) { return 0; }
void delay(double) {}
//Ende der Prototypen f�r VS
*/

Motor::Motor()
{
	//Abschalten des Motortreibers, welcher von diesem Objekt versorgt wird.
	//Evtl. noch undefinierte Pins k�nnen so kein falsches Signal an den Motortreiber geben
	pinMode(PIN_M_INH, OUTPUT);
	digitalWrite(PIN_M_INH, LOW);
	
	mMotorNr = 0;
	mPortNrPWM = 0;
	mPortNrDir = 0;
	mRechtslauf = true;
	mDrehzahl = 0;
}

Motor::Motor(unsigned int motorNr)
{
	//Abschalten des Motortreibers, evtl. noch undefinierte Pins k�nnen so kein falsches Signal an den Motortreiber geben
	pinMode(PIN_M_INH, OUTPUT);
	digitalWrite(PIN_M_INH, LOW);
	
	//Initialisieren des Motorobjektes
	mMotorNr = motorNr;
	
	mPortNrPWM = PORT_M_PWM[mMotorNr];
	mPortNrDir = PORT_M_DIR[mMotorNr];
	mRechtslauf = true;
	mDrehzahl = 0;

	//Zuweisen PWM-Generator zu Pin. Generator 0,2,4,6 f�r Drehzahl
	ledcAttachPin(mPortNrPWM, mMotorNr*2);	//Pin-Nr f�r Drehzahl, PWM-Generator Nr
	ledcSetup(mMotorNr*2, 21700, 8);	//PWM-Generator Nr, 21.7 kHz PWM, 8-bit resolution (0..255)
	
	//pinMode(mPortNrDir, OUTPUT);	//Pin-Nr f�r Richtungsangabe, Ausgang
	//digitalWrite(mPortNrDir, HIGH);	//fr�hzeitiges Definieren des Dir-Pins	//Umwandeln in PWM-Signal, bessere "Kooperation" mit Lampen (gleiche ansteuerung)!!!!!!!!
	//Zuweisen PWM-Generator zu Pin. Generator 1,3,5,7 f�r Richtung
	ledcAttachPin(mPortNrDir, (mMotorNr*2)+1);	//Pin-Nr f�r Richtungsangabe, PWM-Generator Nr
	ledcSetup(mMotorNr*2+1, 21700, 8);
	
	ledcWrite(mMotorNr*2, 0);	//fr�hzeitiges Definieren des PWM-Generators (PWM-Generator Nr., PWM-Wert (0..255))
	ledcWrite(mMotorNr*2+1,255);	//fr�hzeitiges Definieren des Dir-Pins
	
}

void Motor::setValues(bool rechtslauf, unsigned int drehzahl)
{
	mRechtslauf = rechtslauf;
	mDrehzahl = drehzahl;
	
	//Serial.begin(9600); -> sollte in der aufrufenden Instanz schon initialisiert sein
	Serial.print("Motor ");
	Serial.print(mMotorNr);
	Serial.print(" dreht in Richtung ");
	Serial.print(mRechtslauf);
	Serial.print(" mit Drehzahl ");
	Serial.println(mDrehzahl);

	//Berechnen der PWM-Werte
	int drehzahl_pwm;
	if (mDrehzahl < 1)
	{
		drehzahl_pwm = 0;
	}
	else if (mDrehzahl >7)
	{
		drehzahl_pwm = 255;
	}
	else
	{
		drehzahl_pwm = mDrehzahl * 256 / 8;
	}

	//Zuweisen der Richtung an den richtigen Pin entsprechend der Motornr.
	if (mRechtslauf)
	{
		//digitalWrite(mPortNrDir, HIGH);
		ledcWrite(mMotorNr*2+1, 255);	//Generator f�r Richtung wird auf max. (255) gesetzt
	}
	else
	{
		//digitalWrite(mPortNrDir, LOW);
		ledcWrite(mMotorNr*2+1, 0);	//Generator f�r Richtung wird auf 0 gesetzt
		//!!! Unbedingt im Datenblatt des Motortreibers nachsehen, wie PWM und Richtung zusammenh�ngen !!!
		drehzahl_pwm = 255 - drehzahl_pwm;	//wenn der Motor r�ckw�rts l�uft, ist die PWM invertiert (255 = min, 0 = max)
	}

	//Zuweisen des PWM-Werts an den richtigen Generator entsprechend der Motornr.
	ledcWrite(mMotorNr*2, drehzahl_pwm);
	
	digitalWrite(PIN_M_INH, HIGH);	//Einschalten Motortreiber
	
	Serial.print("Dir: ");
	Serial.print(mPortNrDir);
	Serial.print(" PWM: ");
	Serial.print(mPortNrPWM);
	Serial.print(" Val: ");
	Serial.println(drehzahl_pwm);
}

void Motor::reRun()
{
	if(mDrehzahl > 0)
	{
		setValues(mRechtslauf, mDrehzahl);
	}
}

Lampe::Lampe()
{
	//Abschalten des Lampentreibers, welcher von diesem Objekt versorgt wird.
	//Evtl. noch undefinierte Pins k�nnen so kein falsches Signal an den Lampentreiber geben
	pinMode(PIN_L_INH, OUTPUT);
	digitalWrite(PIN_L_INH, LOW);
	
	mLampeNr = 0;
	mPortNrPWM = 0;
	mHelligkeit = 0;
}

Lampe::Lampe(unsigned int lampeNr)
{
	//Abschalten des Motortreibers, evtl. noch undefinierte Pins k�nnen so kein falsches Signal an den Motortreiber geben
	pinMode(PIN_L_INH, OUTPUT);
	digitalWrite(PIN_L_INH, LOW);
	
	//Initialisieren des Lampenobjektes
	mLampeNr = lampeNr;
	
	//Folgender Abschnitt erlaubt es pro 'pololu a4990 dual-motor-driver' 4 Lampen unabh�ngig voneinander anzusteuern
	if(mLampeNr % 2 == 0)	//Lampen 0,2,4,... werden durch Pins der PWM-Reihe angesteuert
	{
		mPortNrPWM = PORT_L_PWM[mLampeNr/2];
	}
	else	//Lampen 1,3,5,... werden durch Pins der DIR-Reihe angesteuert
	{
		mPortNrPWM = PORT_M_DIR[mLampeNr/2];
	}
	mHelligkeit = 0;

	//Zuweisen des PWM-Generators an einen Port entsprechend der Lampennummer...
	ledcAttachPin(mPortNrPWM, mLampeNr);	//Pin-Nr, PWM-Generator Nr
	ledcSetup(mLampeNr, 21700, 8);	//PWM-Generator Nr, 21.7 kHz PWM, 8-bit resolution (0..255)
	//pinMode(PORT_M_DIR[mLampeNr], OUTPUT);	//Pin-Nr f�r "Richtungsangabe", Ausgang
	/*da hier Lampen am selben Treiber angeschlossen sind wie die Motoren, ben�tigen sie eine "Richtung"
	f�r eine korrekte Interpretation der PWM-Signale
	*/
	
	//digitalWrite(PORT_M_DIR[mLampeNr], HIGH);	//fr�hzeitiges Definieren des Dir-Pins
	ledcWrite(mLampeNr, 0);	//fr�hzeitiges Definieren des PWM-Generators (PWM-Generator Nr., PWM-Wert (0..255))
	
}

void Lampe::setValues(unsigned int helligkeit)
{
	//Serial.begin(9600);
	mHelligkeit = helligkeit;
	Serial.print("Lampe ");
	Serial.print(mLampeNr);
	Serial.print(" leuchtet mit Helligkeit ");
	Serial.println(mHelligkeit);
	
	//Berechnen der PWM - Werte
	int helligkeit_pwm;
	if (mHelligkeit > 7)
	{
		helligkeit_pwm = 255;
	}
	else if (mHelligkeit < 1)
	{
		helligkeit_pwm = 0;
	}
	else
	{
		helligkeit_pwm = mHelligkeit * 256 / 8;
	}

	if(mLampeNr%2 == 1)	//bei den Lampen 1,3,5,7 sind laut Datenblatt die PWM invertiert (255 = min, 0 = max)
	{
		helligkeit_pwm = 255 - helligkeit_pwm;
	}
	
	//Zuweisen des PWM-Werts an den richtigen Port entsprechend der Lampennummer
	ledcWrite(mLampeNr, helligkeit_pwm);
	//digitalWrite(PORT_M_DIR[mLampeNr], HIGH);	//Richtungsangabe, siehe Beschreibung im Konstruktor
	
	digitalWrite(PIN_L_INH, HIGH);	//Einschalten Lampentreiber
	
	Serial.print("PWM: ");
	Serial.print(mPortNrPWM);
	Serial.print(" Val: ");
	Serial.println(helligkeit_pwm);
}

void Lampe::reRun()
{
	if(mHelligkeit > 0)
	{
		setValues(mHelligkeit);
	}
}

DigitalAnalogIn::DigitalAnalogIn()
{
	mInputNummer = 0;
}

DigitalAnalogIn::DigitalAnalogIn(unsigned int inputNummer)
{
	mInputNummer = inputNummer;
	mInputPortNr = PORT_IN[mInputNummer];
}

unsigned int DigitalAnalogIn::getValueDigital()
{
	pinMode(mInputPortNr, INPUT_PULLUP);	//Pin-Modus einrichten: Input mit Pull-Up Widerstand
	bool eingabe = !digitalRead(mInputPortNr);	//Inverse Logik: Schalter gedr�ckt = 1 (= Port liegt auf Masse)
	return (unsigned int) eingabe;
}

unsigned int DigitalAnalogIn::getValueAnalog()
{
	pinMode(mInputPortNr, INPUT);	//Pin-Modus einrichten: Input ohne Pull-Up Widerstand
	unsigned int eingabe = analogRead(mInputPortNr);
	return eingabe;
}