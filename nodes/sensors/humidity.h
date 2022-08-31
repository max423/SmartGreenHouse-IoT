#ifndef HUMIDITY_H_
#define HUMIDITY_H_

#define HUMIDITY_SAMPLING_INTERVAL  10
#define HUMIDITY_UPPER_BOUND        60
#define HUMIDITY_LOWER_BOUND        20

PROCESS_NAME(humidity_sensor_process);

extern process_event_t HUMIDITY_SAMPLE_EVENT;
extern process_event_t HUMIDITY_EVENT_SUB;
extern process_event_t HUMIDITY_EVENT_ALERT;

#endif 