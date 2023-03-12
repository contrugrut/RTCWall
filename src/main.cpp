#include <Arduino.h>
#include <TimerOne.h>
#include <Wire.h>                       // For some strange reasons, Wire.h must be included here
#include <DS1307new.h>

uint16_t startAddr = 0x0000;            // Start address to store in the NV-RAM
uint16_t lastAddr;                      // new address for storing in NV-RAM
uint16_t TimeIsSet = 0xaa55;            // Helper that time must not set again

const char compileDate[] = __DATE__;
const char compileTime[] = __TIME__;
int hour, minute, second;

int year, day;
char month[3];

void timerIsr(void);
int getMonthFromAbbreviation(char*);

void setup() {
  Serial.begin(115200); // Inicializar el puerto serie a 9600 baudios

  RTC.setRAM(0, (uint8_t *)&startAddr, sizeof(uint16_t));
  TimeIsSet = 0xffff;
  RTC.setRAM(54, (uint8_t *)&TimeIsSet, sizeof(uint16_t));


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


  Timer1.initialize(1000000); // Configurar el temporizador para generar una interrupción cada segundo (1000000 microsegundos = 1 segundo)
  Timer1.attachInterrupt(timerIsr); // Asignar la rutina de interrupción al temporizador
}

void loop() {
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

void timerIsr() {
  Serial.println("Hola, mundo!"); // Mostrar un saludo por el puerto serie

  RTC.getTime();
  if (RTC.hour < 10)                    // correct hour if necessary
  {
    Serial.print("0");
    Serial.print(RTC.hour, DEC);
  }
  else
  {
    Serial.print(RTC.hour, DEC);
  }
  Serial.print(":");
  if (RTC.minute < 10)                  // correct minute if necessary
  {
    Serial.print("0");
    Serial.print(RTC.minute, DEC);
  }
  else
  {
    Serial.print(RTC.minute, DEC);
  }
  Serial.print(":");
  if (RTC.second < 10)                  // correct second if necessary
  {
    Serial.print("0");
    Serial.print(RTC.second, DEC);
  }
  else
  {
    Serial.print(RTC.second, DEC);
  }
  Serial.print(" ");
  if (RTC.day < 10)                    // correct date if necessary
  {
    Serial.print("0");
    Serial.print(RTC.day, DEC);
  }
  else
  {
    Serial.print(RTC.day, DEC);
  }
  Serial.print("-");
  if (RTC.month < 10)                   // correct month if necessary
  {
    Serial.print("0");
    Serial.print(RTC.month, DEC);
  }
  else
  {
    Serial.print(RTC.month, DEC);
  }
  Serial.print("-");
  Serial.print(RTC.year, DEC);          // Year need not to be changed
  Serial.println(" ");
  switch (RTC.dow)                      // Friendly printout the weekday
  {
    case 1:
      Serial.print("MON");
      break;
    case 2:
      Serial.print("TUE");
      break;
    case 3:
      Serial.print("WED");
      break;
    case 4:
      Serial.print("THU");
      break;
    case 5:
      Serial.print("FRI");
      break;
    case 6:
      Serial.print("SAT");
      break;
    case 7:
      Serial.print("SUN");
      break;
  }
}