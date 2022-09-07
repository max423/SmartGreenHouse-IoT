#include "sys/log.h"
#include "./humidity.h"
#include "./utils.h"
#include <stdio.h>

#define LOG_MODULE "sensor"
#define LOG_LEVEL LOG_LEVEL_APP

#define OFF 0
#define ON 1


process_event_t HUMIDITY_SAMPLE_EVENT;
process_event_t HUMIDITY_EVENT_SUB;
process_event_t HUMIDITY_EVENT_ALERT;

PROCESS(humidity_sensor_process, "Humidity level sensor process");

PROCESS_THREAD(humidity_sensor_process, ev, data){
    static struct etimer et;
    static struct etimer etm;
    static struct process *subscriber;
    static int sample;
    static int num;
    PROCESS_BEGIN();

    LOG_INFO("Humidity level process started\n");
  
    etimer_set(&etm, 3*CLOCK_SECOND);
    HUMIDITY_SAMPLE_EVENT = process_alloc_event();
    PROCESS_WAIT_EVENT_UNTIL(ev == HUMIDITY_EVENT_SUB);
    sample = sensor_rand_int(HUMIDITY_LOWER_BOUND, HUMIDITY_UPPER_BOUND);
    subscriber = (struct process *)data;
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etm));
    etimer_set(&et, CLOCK_SECOND*HUMIDITY_SAMPLING_INTERVAL);
    while(true) {
        // humidity simulation
        PROCESS_YIELD();
        if(etimer_expired(&et)){
            
            num = rand()%2;
            sample = sample - num;

            if (sample < 0)
               sample = 0;

            process_post(subscriber, HUMIDITY_SAMPLE_EVENT, &sample);
            etimer_reset(&et);

        }else if(ev == HUMIDITY_EVENT_ALERT){
            num = rand()%5;
            sample = 60 + num;
        
        }
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
