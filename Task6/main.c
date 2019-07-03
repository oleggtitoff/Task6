#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define INPUT_FILE_NAME "TestSound5.wav"
#define OUTPUT_FILE_NAME "Output.wav"
#define FILE_HEADER_SIZE 44
#define BYTES_PER_SAMPLE 2
#define DATA_BUFF_SIZE 1000
#define SAMPLE_RATE 48000
#define CHANNELS 2

#define FREQUENCY 600

#define PI 3.14159265358979323846


typedef struct {
	int16_t x;
	int16_t y;
} AllpassBuff;


int32_t doubleToFixed31(double x);

FILE * openFile(char *fileName, _Bool mode);		//if 0 - read, if 1 - write
void readHeader(uint8_t *headerBuff, FILE *inputFilePtr);
void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr);

void initializeBuff(AllpassBuff *buff);
int32_t calculateCoeff(double Fc);

int16_t allpassFilter(int16_t sample, AllpassBuff *buff, int32_t coeff);
int16_t allpassLPF(int16_t sample, AllpassBuff *buff, int32_t coeff);
int16_t allpassHPF(int16_t sample, AllpassBuff *buff, int32_t coeff);
void run(FILE *inputFilePtr, FILE *outputFilePtr, AllpassBuff *buff, int32_t coeff);


int main()
{
	FILE *inputFilePtr = openFile(INPUT_FILE_NAME, 0);
	FILE *outputFilePtr = openFile(OUTPUT_FILE_NAME, 1);
	uint8_t headerBuff[FILE_HEADER_SIZE];
	AllpassBuff buff[2];
	initializeBuff(&buff[0]);
	initializeBuff(&buff[1]);

	readHeader(headerBuff, inputFilePtr);
	writeHeader(headerBuff, outputFilePtr);
	run(inputFilePtr, outputFilePtr, buff, calculateCoeff(FREQUENCY));
	fclose(inputFilePtr);
	fclose(outputFilePtr);

	return 0;
}


int32_t doubleToFixed31(double x)
{
	if (x >= 1)
	{
		return INT32_MAX;
	}
	else if (x < -1)
	{
		return INT32_MIN;
	}

	return (int32_t)(x * (double)(1LL << 31));
}

FILE * openFile(char *fileName, _Bool mode)		//if 0 - read, if 1 - write
{
	FILE *filePtr;

	if (mode == 0)
	{
		if ((filePtr = fopen(fileName, "rb")) == NULL)
		{
			printf("Error opening input file\n");
			system("pause");
			exit(0);
		}
	}
	else
	{
		if ((filePtr = fopen(fileName, "wb")) == NULL)
		{
			printf("Error opening output file\n");
			system("pause");
			exit(0);
		}
	}

	return filePtr;
}

void readHeader(uint8_t *headerBuff, FILE *inputFilePtr)
{
	if (fread(headerBuff, FILE_HEADER_SIZE, 1, inputFilePtr) != 1)
	{
		printf("Error reading input file (header)\n");
		system("pause");
		exit(0);
	}
}

void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr)
{
	if (fwrite(headerBuff, FILE_HEADER_SIZE, 1, outputFilePtr) != 1)
	{
		printf("Error writing output file (header)\n");
		system("pause");
		exit(0);
	}
}

void initializeBuff(AllpassBuff *buff)
{
	buff->x = 0;
	buff->y = 0;
}

int32_t calculateCoeff(double Fc)
{
	double tang = tan(PI * Fc / SAMPLE_RATE);
	return doubleToFixed31((tang - 1) / (tang + 1));
}

int16_t allpassFilter(int16_t sample, AllpassBuff *buff, int32_t coeff)
{
	int64_t acc = (int64_t)coeff * sample + ((int64_t)buff->x << 31) - (int64_t)coeff * buff->y;

	buff->x = sample;
	buff->y = (int16_t)((acc + (1LL << 30)) >> 31);

	return buff->y;
}

int16_t allpassLPF(int16_t sample, AllpassBuff *buff, int32_t coeff)
{
	return (int16_t)(((int32_t)sample + allpassFilter(sample, buff, coeff)) >> 1);
}

int16_t allpassHPF(int16_t sample, AllpassBuff *buff, int32_t coeff)
{
	return (int16_t)(((int32_t)sample - allpassFilter(sample, buff, coeff)) >> 1);

}

void run(FILE *inputFilePtr, FILE *outputFilePtr, AllpassBuff *buff, int32_t coeff)
{
	int16_t dataBuff[DATA_BUFF_SIZE];
	size_t samplesRead;
	uint32_t i;

	while (1)
	{
		samplesRead = fread(dataBuff, BYTES_PER_SAMPLE, DATA_BUFF_SIZE, inputFilePtr);

		if (!samplesRead)
		{
			break;
		}

		for (i = 0; i < samplesRead - 1; i += 2)
		{
			dataBuff[i] = allpassLPF(dataBuff[i], &buff[0], coeff);
			dataBuff[i + 1] = allpassHPF(dataBuff[i + 1], &buff[1], coeff);
		}

		fwrite(dataBuff, BYTES_PER_SAMPLE, samplesRead, outputFilePtr);
	}
}