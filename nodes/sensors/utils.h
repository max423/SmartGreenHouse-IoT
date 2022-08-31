#ifndef UTILS_H_
#define UTILS_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TEMPERATURE_UNIT        "C"
#define HUMIDITY_UNIT           "RH"
#define LIGHT_UNIT              "LM"

int sensor_rand_int(int min, int max);

void json_sample(char *message_buffer, size_t size, char* topic, int sample, char* unit, int id);

#endif