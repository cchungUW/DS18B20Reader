/***********NOT ENTIRELY MY CODE************/
// This code is adapted from Albert Herd:
// https://github.com/albertherd/DS18B20Reader
// https://github.com/albertherd/LCD1602
//
// This software also uses the gnuplot_i library written by N.Devillard:
// http://ndevilla.free.fr/gnuplot/

#include <stdio.h> 
#include <time.h>
#include <signal.h>
#include <wiringPi.h>

#include "main.h"
#include "sensor.h"
#include "lcd1602.h"
#include "gnuplot_i.h"

volatile sig_atomic_t sigintFlag = 0;

void onSigInt(int signum){
    sigintFlag = 1;
}

void InitializeLCD()
{
    int rc;
	rc = lcd1602Init(1, LCDADDRESS);
	if (rc)
	{
		printf("Initialization failed; aborting...\n");
		return;
	}
}

int main(int argc, char **argv)
{ 
    signal(SIGINT, onSigInt);
    InitializeLCD();
    if (wiringPiSetup() == -1)                                  //Added code to initialize wiringPi
    {                                                           //
        printf("ERROR: WiringPi Initialization Failed");        //
        return 1;                                               //
    }                                                           //
    pinMode(RELAYPIN, OUTPUT);                                  //Set wiringPi Pin 0 (GPIO 17) to output

    gnuplot_ctrl * plot ;                                          //Initialize gnuplot session
    plot = gnuplot_init() ;                                        //Ptr h is handler for session
    gnuplot_cmd(plot, "plot \"tempData.dat\" using 1:2");

    FILE * tempData = fopen("tempData.dat", "r+");              //Open a .dat file for read & write
    if (tempData == NULL)                                       //Throw error if file can't be opened
    {
        printf("ERROR: Cannot open data file");
        return 1;
    }

    char **sensorNames;
    int sensorNamesCount;
    if(argc > 1)
    {
        sensorNames = argv + 1;
        sensorNamesCount = argc - 1;
    }    
    else
    {
        sensorNames = NULL;
        sensorNamesCount = 0;
    }    

    SensorList *sensorList = GetSensors(sensorNames, sensorNamesCount);
    if(sensorList->SensorCount == 0)
    {
        printf("No sensors found - exiting.\n");
        return 0;
    }
    printf("Attached Sensors: %d\n", sensorList->SensorCount);

    ReadTemperatureLoop(sensorList, tempData);        
    Cleanup(sensorList, tempData, plot);   
}


void Cleanup(SensorList *sensorList, FILE * tempData, gnuplot_ctrl * plot )
{
    printf("Exiting...\n");
    FreeSensors(sensorList);
    lcd1602Shutdown();
    digitalWrite(RELAYPIN, LOW);       //Ensures relay opens upon shutdown
    gnuplot_close(plot);                    //Closes gnuplot session
    fclose(tempData);                  //Close tempData file
}

void ReadTemperatureLoop(SensorList *sensorList, FILE * tempData)      
{
    while(!sigintFlag)
    {
        for(int i = 0; i < sensorList->SensorCount; i++)
        {
            float temperature = ReadTemperature(sensorList->Sensors[i]);
            DriveRelay(temperature);
            //DriveRelayWithPWM(temperature);
            PrintTemperatueToLCD1602(sensorList->Sensors[i], i % LCD1602LINES, temperature);
            LogTemperature(sensorList->Sensors[i], temperature, tempData);
            //GnuplotRead(tempData);
        }       
    }
}

void LogTemperature(Sensor *sensor, float temperature, FILE * tempData)
{
    time_t currentTime;
    time(&currentTime);

    char dateTimeStringBuffer[32];
    strftime(dateTimeStringBuffer, 32, "%Y-%m-%d %H:%M:%S", localtime(&currentTime));

    printf("%s - %s - %.2fF\n", dateTimeStringBuffer, sensor->SensorName, temperature);
    //TODO: Write time and temperature to .dat file for plot
    //fprintf(tempData, "%s", temperature);
}

void PrintTemperatueToLCD1602(Sensor *sensor, int lineToPrintDataOn, float temperature)
{
    char temperatureString[LCD1602CHARACTERS + 1];
    snprintf(temperatureString, LCD1602CHARACTERS + 1, "%s : %.2fF", sensor->SensorName, temperature);

    lcd1602SetCursor(0, lineToPrintDataOn);
	lcd1602WriteString(temperatureString);
}

//Drive Relay with a simple conditional statement
void DriveRelay(const float temperature)
{
    if (temperature < SETTEMP - 2) digitalWrite(RELAYPIN, HIGH);
    if (temperature > SETTEMP ) digitalWrite(RELAYPIN, LOW);
}

//Drive Relay with a PID controller
void DriveRelayWithPWM(const float temperature)
}
    /*PID constants*/
    Kp = 20;
    Ki = .01;
    Kd = 150;

    int output = 0;
    static float prevError;                   
    float error = SETTEMP - temperature;

    time_t currentTime;
    time(&currentTime);
    struct timer tm = localtime(&currentTime);
    float dt = now - prevUpdate;      //change in time
    now = tm->tm_sec;                 //Method should be in less than 1 min. dt should only be in seconds?
    static float prevUpdate;
    
    float integral = integral + dt * error;
    derivative = (error - prevError) / dt;
    int output = Kp * error + Ki * integral + Kd * derivative;

    prevError = error;
    prevUpdate = now;

    pwmWrite(RELAYPIN, output);     //Write PID output as PWM signal

}

/*
//Reads temperature data stream and graphs it
void GnuplotRead(FILE * tempData)
{
    gnuplot_cmd(plot, "set xdata time");
    gnuplot_cmd(plot, "set timefmt \"%H:%M:%S\"");
    gnuplot_cmd(plot, "set format x \"%H:%M:%S\"");
    gnuplot_cmd(plot, "plot \"tempData.dat\" using 1:2 with line title \"Sensor 1\"");
}*/