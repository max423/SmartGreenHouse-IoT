#include <stdbool.h>
#include "contiki.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "./light.h"
#include "./utils.h"
#include <stdio.h>

#define LOG_MODULE "sensor"
#define LOG_LEVEL LOG_LEVEL_APP

#define OFF 0
#define ON 1

process_event_t LIGHT_SAMPLE_EVENT;
process_event_t LIGHT_EVENT_SUB;

PROCESS(light_sensor_process, "Light sensor process");

PROCESS_THREAD(light_sensor_process, ev, data){
    static struct etimer et;
    static struct etimer etm;
    static struct process *subscriber;
    static int sample;
    static int num = 1;

    PROCESS_BEGIN();

    LOG_INFO("Light process started\n");

    etimer_set(&etm, 6*CLOCK_SECOND);
    LIGHT_SAMPLE_EVENT = process_alloc_event();
    PROCESS_WAIT_EVENT_UNTIL(ev == LIGHT_EVENT_SUB);
    sample = sensor_rand_int(LIGHT_LOWER_BOUND, LIGHT_UPPER_BOUND);
    subscriber = (struct process *)data;
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etm));
    etimer_set(&et, CLOCK_SECOND*LIGHT_SAMPLING_INTERVAL);

    while(true) {
        // light simulation
        PROCESS_YIELD();
        if(etimer_expired(&et)){
            if (sample == 40)
                num = -rand()%2;
            if (sample == 5)
                num = rand()%2;

            sample = sample + num;

            process_post(subscriber, LIGHT_SAMPLE_EVENT, &sample);
            etimer_reset(&et);
        }
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
