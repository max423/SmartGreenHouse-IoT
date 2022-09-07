#include <stdbool.h>
#include "contiki.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "./temperature.h"
#include "./utils.h"
#include <stdio.h>

#define LOG_MODULE "sensor"
#define LOG_LEVEL LOG_LEVEL_APP

#define OFF 0
#define ON 1

process_event_t TEMPERATURE_SAMPLE_EVENT;
process_event_t TEMPERATURE_EVENT_SUB;
process_event_t TEMPERATURE_EVENT_ALERT;


PROCESS(temperature_sensor_process, "Temperature sensor process");

PROCESS_THREAD(temperature_sensor_process, ev, data){
    static struct etimer et;
    static struct process *subscriber;
    static int sample;
    static bool alert;
    static int num;
    static int sampleMAX;
    static int sampleMIN;

    PROCESS_BEGIN();

    LOG_INFO("Temperature process started\n");

    alert = OFF;
    TEMPERATURE_SAMPLE_EVENT = process_alloc_event();
    PROCESS_WAIT_EVENT_UNTIL(ev == TEMPERATURE_EVENT_SUB);
    sample = sensor_rand_int(TEMPERATURE_LOWER_BOUND, TEMPERATURE_UPPER_BOUND);
    subscriber = (struct process *)data;
    etimer_set(&et, CLOCK_SECOND*TEMPERATURE_SAMPLING_INTERVAL);
  
    while(true) {
        // temperature simulation
        PROCESS_YIELD();
        if(etimer_expired(&et)){

            if(alert == OFF){
                num = rand()%10;
                sampleMAX=sample+num;
                sampleMIN=sample-num;

                sample = sensor_rand_int(sampleMIN, sampleMAX);
                if (sample < -2)
                    sample = rand()%5;

            }else if(alert == ON){
                sample -= 1;
            }

            process_post(subscriber, TEMPERATURE_SAMPLE_EVENT, &sample);
            etimer_reset(&et);

        }else if(ev == TEMPERATURE_EVENT_ALERT){
            alert = data;
        }
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
