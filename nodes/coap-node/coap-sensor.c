#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os/sys/log.h"
#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "os/sys/etimer.h"
#include "os/net/ipv6/uip-ds6.h"
#include "os/dev/leds.h"
#include "dev/button-hal.h"
#include "../sensors/temperature.h"
#include "../sensors/humidity.h"
#include "../sensors/light.h"
#include "./resources/res-temperature.h"
#include "./resources/res-humidity.h"
#include "./resources/res-light.h"
#include "./resources/res-alert.h"

#define OFF 0
#define ON 1

#define LOG_MODULE "coap-sensor"
#define LOG_LEVEL LOG_LEVEL_APP

#define SENSOR_ID_LENGTH 10
#define STATE_INIT    		  0
#define COAP_NODE_STATE_STARTED 1
#define COAP_NODE_STATE_OPERATIONAL 2

#define SERVER "coap://[fd00::1]:5683"

PROCESS_NAME(coap_server);
AUTOSTART_PROCESSES(&coap_server);

char *service_url = "coapReg";
static uint8_t state;
bool registered = false;
static int node_id;

/* Resources to be activated need to be imported through the extern keyword
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_alert;
extern coap_resource_t res_temperature;
extern coap_resource_t res_humidity;
extern coap_resource_t res_light;

/*---------------------------------------------------------------------------*/
// emulation of the sensors based on the event
static void sensors_emulation(process_event_t event, int sample){
    if (event == TEMPERATURE_SAMPLE_EVENT){
    LOG_INFO("Temperature: new measurement %d. Updating collector.\n", sample);
    res_temperature_update(sample, node_id);

    } else if (event == HUMIDITY_SAMPLE_EVENT) {
    LOG_INFO("Humidity: new measurement %d. Updating collector.\n", sample);
    res_humidity_update(sample, node_id);

    } else if (event == LIGHT_SAMPLE_EVENT) {
    LOG_INFO("Light: new measurement %d. Updating collector.\n", sample);
    res_light_update(sample, node_id);
    }
}

/*---------------------------------------------------------------------------*/
// This function is will be passed to COAP_BLOCKING_REQUEST() to handle responses
void client_chunk_handler(coap_message_t *response){
    const uint8_t *chunk;
    if(response == NULL) {
        return;
    }
    state = COAP_NODE_STATE_OPERATIONAL;
    int len = coap_get_payload(response, &chunk);
    LOG_INFO("|%.*s", len, (char *)chunk);
}

static bool have_connectivity(void){
    if(uip_ds6_get_global(ADDR_PREFERRED) == NULL || uip_ds6_defrt_choose() == NULL) {
        return false;
    }
    return true;
}

PROCESS(coap_server, "Coap Server");

// init the node
static void init_node(){
    state = STATE_INIT;

    //leds_single_on(LEDS_GREEN);
    //leds_single_on(LEDS_RED);
    leds_on(LEDS_ALL);

    res_temperature_activate();
    process_start(&temperature_sensor_process, NULL);
    process_post(&temperature_sensor_process, TEMPERATURE_EVENT_SUB, &coap_server);

    res_humidity_activate();
    process_start(&humidity_sensor_process, NULL);
    process_post(&humidity_sensor_process, HUMIDITY_EVENT_SUB, &coap_server);

    res_light_activate();
    process_start(&light_sensor_process, NULL);
    process_post(&light_sensor_process, LIGHT_EVENT_SUB, &coap_server);

    res_alert_activate();
}

static bool sensor_event(process_event_t event){
    if(event == TEMPERATURE_SAMPLE_EVENT || event == HUMIDITY_SAMPLE_EVENT || event == LIGHT_SAMPLE_EVENT) {
        return true;
    }
    return false;
}

PROCESS_THREAD(coap_server, ev, data){
    static coap_endpoint_t server;
    static coap_message_t request[1];
    PROCESS_BEGIN();
    init_node();

    node_id = linkaddr_node_addr.u8[7];

    // registration to the collector
    coap_endpoint_parse(SERVER, strlen(SERVER), &server);
    coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
    coap_set_header_uri_path(request, service_url);
  
    while(true) {
        PROCESS_WAIT_EVENT();

        if(state==STATE_INIT){
            if(have_connectivity()==true)
            state = COAP_NODE_STATE_STARTED;
            //leds_single_off(LEDS_RED);
            leds_off(LEDS_ALL);
            leds_on(LEDS_GREEN);
        }

        if(state == COAP_NODE_STATE_STARTED){
			COAP_BLOCKING_REQUEST(&server, request, client_chunk_handler);
        }

        if(sensor_event(ev) && state == COAP_NODE_STATE_OPERATIONAL){
            sensors_emulation(ev, *((int *)data));
        }
        else if(ev == button_hal_press_event && state == COAP_NODE_STATE_OPERATIONAL){
            process_post(&humidity_sensor_process, HUMIDITY_EVENT_ALERT, (int*) ON);
        }
    }

    PROCESS_END();
}
