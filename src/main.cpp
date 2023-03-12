#include <Arduino.h>
// #include <TimerOne.h>
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
char buffer[20];

struct Horario {
  int hora;
  int minuto;
  int segundo;
  int duracion;
};

Horario horarios[] = {
    { 14, 59, 0, 10 }, // Encender a las 10:30 y durar 10 segundos
    { 15, 0, 0, 5 },  // Encender a las 14:45 y durar 5 segundos
    // Agregar más horarios según sea necesario
  };

void timerIsr(void);
int getMonthFromAbbreviation(char*);
void ejecutarHorarios(Horario [], int);

void setup() {
  pinMode(13, OUTPUT);

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
}

void loop() {
  RTC.getTime();

  int numHorarios = sizeof(horarios) / sizeof(horarios[0]);
  ejecutarHorarios(horarios, numHorarios);

  delay(200);
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
      digitalWrite(13, HIGH);
      sprintf(buffer, "%02d:%02d:%02d %02d-%02d-%d", RTC.hour, RTC.minute, RTC.second, RTC.day, RTC.month, RTC.year);
      Serial.println(buffer);
      delay(horario.duracion * 1000); // Convertir segundos en milisegundos
      digitalWrite(13, LOW);
    }
  }
}