#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#define TEMPERATURE_SAMPLING_INTERVAL   10
#define TEMPERATURE_UPPER_BOUND        40
#define TEMPERATURE_LOWER_BOUND        0

PROCESS_NAME(temperature_sensor_process);

extern process_event_t TEMPERATURE_SAMPLE_EVENT;
extern process_event_t TEMPERATURE_EVENT_SUB;
extern process_event_t TEMPERATURE_EVENT_ALERT;

#endif 