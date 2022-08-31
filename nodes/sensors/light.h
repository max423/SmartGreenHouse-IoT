#ifndef LIGHT_H_
#define LIGHT_H_

#define LIGHT_SAMPLING_INTERVAL   10
#define LIGHT_UPPER_BOUND        40
#define LIGHT_LOWER_BOUND        5

PROCESS_NAME(light_sensor_process);

extern process_event_t LIGHT_SAMPLE_EVENT;
extern process_event_t LIGHT_EVENT_SUB;

#endif