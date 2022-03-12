#ifndef _main_h
#define _main_h 

#define LCDADDRESS 0x27
#define RELAYPIN 1              //Define wiringPi Pin 1, Board 12
#define SETTEMP 80              //Define target temperature

#include <wiringPi.h>
#include "sensor.h"
#include "gnuplot_i.h"

void Cleanup(SensorList *sensorList, FILE * tempData, gnuplot_ctrl * plot );
void ReadTemperatureLoop(SensorList *sensorList, FILE * tempData);
void LogTemperature(Sensor *sensor, float temperature, FILE * tempData);
void PrintTemperatueToLCD1602(Sensor *sensor, int lineToPrintDataOn, float temperature);
void DriveRelay(const float temperature);
void DriveRelayWithPWM(const float temperature);
void gnuplotRead(FILE * tempData);

#endif /* _main_h */