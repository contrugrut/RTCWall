#include <Arduino.h>
#include <SoftwareSerial.h>
// #include <TimerOne.h>
#include <Wire.h>                       // For some strange reasons, Wire.h must be included here
#include <DS1307new.h>


#define RX_PIN 4
#define TX_PIN 3

SoftwareSerial RelayModule(RX_PIN, TX_PIN);

uint16_t startAddr = 0x0000;            // Start address to store in the NV-RAM
uint16_t lastAddr;                      // new address for storing in NV-RAM
uint16_t TimeIsSet = 0xaa55;            // Helper that time must not set again

const char compileDate[] = __DATE__;
const char compileTime[] = __TIME__;
int hour, minute, second;

int year, day;
char month[3];
char buffer[20];

int numHorarios;
struct Horario {
  int hora;
  int minuto;
  int segundo;
  int duracion;
};

byte moduleBuffer[4];

byte i=0;

/* 
1) 5:00- 20 min
2) 10:00 - 10 min
3) 3:00 pm - 10 min 
4) 6:00 pm - 10 min 
*/
Horario horarios[] = {
    {  5, 0, 0, 20 }, // Encender a las 10:30 y durar 10 segundos
    { 10, 0, 0, 10 },  // Encender a las 14:45 y durar 5 segundos
    { 15, 0, 0, 10 }, // Agregar más horarios según sea necesario
    { 18, 25, 30, 10 }
  };

void timerIsr(void);
int getMonthFromAbbreviation(char*);
void ejecutarHorarios(Horario [], int);
void fakeATFirmware(void);
void riego(void);
void fin(void);

void setup() {
  pinMode(13, OUTPUT);

  Serial.begin(115200); // Inicializar el puerto serie a 9600 baudios
  RelayModule.begin(115200);

  RTC.setRAM(0, (uint8_t *)&startAddr, sizeof(uint16_t));
  // TimeIsSet = 0xffff;
  // RTC.setRAM(54, (uint8_t *)&TimeIsSet, sizeof(uint16_t));


  RTC.getRAM(54, (uint8_t *)&TimeIsSet, sizeof(uint16_t));
  Serial.println(compileTime);
  if (TimeIsSet != 0xaa55) {
    RTC.stopClock();

    // month = getMonthFromAbbreviation(compileDate);
    // Establecer la fecha del RTC
    sscanf(compileDate, "%3s %d %d", month, &day, &year);
    int mes = getMonthFromAbbreviation(month);
    RTC.fillByYMD(year, mes, day);

    // RTC.fillByYMD(2011,4,8);
    sscanf(compileTime, "%d:%d:%d", &hour, &minute, &second);
    RTC.fillByHMS(hour,minute,second);

    RTC.setTime();
    TimeIsSet = 0xaa55;
    RTC.setRAM(54, (uint8_t *)&TimeIsSet, sizeof(uint16_t));
    RTC.startClock();
  } else {
    RTC.getTime();
  }

/*
   Control Register for SQW pin which can be used as an interrupt.
*/
  RTC.ctrl = 0x00;                      // 0x00=disable SQW pin, 0x10=1Hz,
                                        // 0x11=4096Hz, 0x12=8192Hz, 0x13=32768Hz
  RTC.setCTRL();
  RTC.getTime();
  fakeATFirmware();
  fin();
  
}

void loop() {
  RTC.getTime();
  // Serial.println("iniciar");
  // delay(100);
  // fakeATFirmware();
  // riego();
  // delay(1000);
  // fin();
  numHorarios = sizeof(horarios) / sizeof(horarios[0]);
  ejecutarHorarios(horarios, numHorarios);

  sprintf(buffer, "%02d:%02d:%02d %02d-%02d-%d", RTC.hour, RTC.minute, RTC.second, RTC.day, RTC.month, RTC.year);
  Serial.println(buffer);
  delay(200);
}

void fakeATFirmware() {
	// pretend to be an AT device here
	if(RelayModule.available()) {
		String stringIn=RelayModule.readStringUntil('\r');
		RelayModule.flush();// flush what's left '\n'?

		if(stringIn!="") {
			// logger.debug("Serial received: %s", stringIn);

			if(stringIn.indexOf("AT+")>-1)
			RelayModule.println("OK");

			if(stringIn.indexOf("AT+RST")>-1) {
				// pretend we reset (wait a bit then send the WiFi connected message)
				delay(1);
				RelayModule.println("WIFI CONNECTED\r\nWIFI GOT IP");
			}
		}
	}
}

int getMonthFromAbbreviation(char monthAbbreviation[3]) {
  // Función auxiliar para convertir la abreviatura del mes en el número del mes
  const char *monthNames[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  for (int i = 1; i <= 12; i++) {
    if (strcmp(monthAbbreviation, monthNames[i]) == 0) {
      return i;
    }
  }
  return 0;  // Si no se encuentra el mes, se devuelve 0
}

void ejecutarHorarios(Horario horarios[], int numHorarios) {
  for (int i = 0; i < numHorarios; i++) {
    Horario horario = horarios[i];
    if (horario.hora == RTC.hour && horario.minuto == RTC.minute && horario.segundo == RTC.second) {
      fakeATFirmware();
      digitalWrite(13, HIGH);
      riego();
      sprintf(buffer, "%02d:%02d:%02d %02d-%02d-%d", RTC.hour, RTC.minute, RTC.second, RTC.day, RTC.month, RTC.year);
      Serial.println(buffer);
      delay(horario.duracion * 60000);
      fakeATFirmware();
      fin();
      digitalWrite(13, HIGH);
    }
  }
}

void riego() {
	moduleBuffer[0] = 0xA0;
	moduleBuffer[2] = 0x01;
	for (i = 1; i <= 4; i++) {
		moduleBuffer[1] = i;
		moduleBuffer[3] = moduleBuffer[0] + moduleBuffer[1] + moduleBuffer[2];
		delay(100);
		RelayModule.write(moduleBuffer, sizeof(moduleBuffer));
	}
}

void fin() {
	moduleBuffer[0] = 0xA0;
	moduleBuffer[2] = 0x00;
	for (i = 1; i <= 4; i++) {
		moduleBuffer[1] = i;
		moduleBuffer[3] = moduleBuffer[0] + moduleBuffer[1] + moduleBuffer[2];
		delay(100);
		RelayModule.write(moduleBuffer, sizeof(moduleBuffer));
	}
}