#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "os/dev/leds.h"
#include "os/sys/log.h"
#include "../../sensors/utils.h"
#include "../../sensors/humidity.h"
#include "../../sensors/light.h"
#include "../../sensors/temperature.h"

#define LOG_MODULE "coap-sensor"
#define LOG_LEVEL LOG_LEVEL_APP

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*---------------------------------------------------------------------------*/
static uint8_t temperature_state;
static uint8_t humidity_state;
static uint8_t light_state;

#define HUMIDITY_OK      		    0
#define HUMIDITY_ERROR              1
#define TEMPERATURE_OK              2
#define TEMPERATURE_ERROR           3
#define LIGHT_OK      		        4
#define LIGHT_ERROR                 5

#define OFF 0
#define ON 1

#define COAP_HUMIDITY_OK            0
#define COAP_HUMIDITY_ERROR         1
#define COAP_TEMPERATURE_OK         2
#define COAP_TEMPERATURE_ERROR      3
#define COAP_LIGHT_OK               4
#define COAP_LIGHT_ERROR            5

/* A handler function named [resource name]_handler must be implemented for each RESOURCE
 * A buffer for the response payload is provided through the buffer pointer. Simple resources can ignore
 * preferred_size and offset, but must respect the REST_MAX_CHUNK_SIZE limit for the buffer
 * If a smaller block size is requested for CoAP, the REST framework automatically splits the data.
*/

RESOURCE(res_alert,
         "title =\"Alert\" POST/PUTstate=<state>;rt=\"Control\"",
         NULL,
         res_post_handler,
         res_post_handler,
         NULL);

// POST request received from the collector
static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    const char *value = NULL;
    int state;
    state = -1;
    int success = 1;

    LOG_INFO("POST request received from the Collector\n");

    if (coap_get_post_variable(request, "state", &value)) {
        state = atoi(value);
        // LEDS management
        switch (state)
        {
            case COAP_TEMPERATURE_OK:
                if (temperature_state == TEMPERATURE_ERROR) {
                    process_post(&temperature_sensor_process, TEMPERATURE_EVENT_ALERT, (int*) OFF);
                }
                if (humidity_state == HUMIDITY_OK && light_state == LIGHT_OK) {
                    //leds_single_off(LEDS_RED);
                    //leds_single_on(LEDS_GREEN);
                    leds_off(LEDS_ALL);
                    leds_on(LEDS_GREEN);
                }
              
                LOG_INFO("STATE : Temperature value has returned to normal \n");
   	            LOG_INFO("Ventilation system OFF \n");
                temperature_state = TEMPERATURE_OK;
                break;

            case COAP_TEMPERATURE_ERROR:
                process_post(&temperature_sensor_process, TEMPERATURE_EVENT_ALERT, (int*) ON);
                //leds_single_on(LEDS_RED);
                //leds_single_off(LEDS_GREEN);
                leds_off(LEDS_ALL);
                leds_on(LEDS_RED);
                LOG_INFO("WARNING: Temperature max threshold exceeded! Changing led color to red\n");
    	        LOG_INFO("Ventilation system ON \n");
                temperature_state = TEMPERATURE_ERROR;
                break;

            case COAP_HUMIDITY_OK:                
                if (temperature_state == TEMPERATURE_OK && light_state == LIGHT_OK) {
                    //leds_single_off(LEDS_RED);
                    //leds_single_on(LEDS_GREEN);
                    leds_off(LEDS_ALL);
                    leds_on(LEDS_GREEN);
                }
               
                LOG_INFO("STATE : Humidity value has returned to normal \n");
   	            LOG_INFO("Irrigation completed \n");
                humidity_state = HUMIDITY_OK;
                break;

            case COAP_HUMIDITY_ERROR:
                //leds_single_on(LEDS_RED);
                //leds_single_off(LEDS_GREEN);
                leds_off(LEDS_ALL);
                leds_on(LEDS_RED);
                LOG_INFO("WARNING: Humidity min threshold exceeded! Changing led color to red\n");
                LOG_INFO("Waiting for irrigation command \n");
                humidity_state = HUMIDITY_ERROR;
                break;

            case COAP_LIGHT_OK:
                if(temperature_state == TEMPERATURE_OK && humidity_state == HUMIDITY_OK) {
                    //leds_single_off(LEDS_RED);
                    //leds_single_on(LEDS_GREEN);
                    leds_off(LEDS_ALL);
                    leds_on(LEDS_GREEN);
                }                
                LOG_INFO("STATE : Light value has returned to normal \n");
                LOG_INFO("Lighting system OFF \n");
                light_state = LIGHT_OK;
                break;

            case COAP_LIGHT_ERROR:
                //leds_single_on(LEDS_RED);
                //leds_single_off(LEDS_GREEN);
                leds_off(LEDS_ALL);
                leds_on(LEDS_RED);
                LOG_INFO("WARNING: Light min threshold exceeded! Changing led color to red\n");
                LOG_INFO("Lighting system ON\n");
                light_state = LIGHT_ERROR;
                break;

            default:
                success = 0;
                break;
        }
    } else {
        success = 0;
    }

    if(!success) {
        coap_set_status_code(response, BAD_REQUEST_4_00);
        LOG_DBG("400 BAD REQUEST\n");
    } else {
        char res[32] = "";
        snprintf(res, sizeof(res), "%d", state);
        int length = strlen(res);
        memcpy(buffer, res, length);
        LOG_DBG("Post request processed successfully\n");
        coap_set_header_content_format(response, TEXT_PLAIN);
        coap_set_header_etag(response, (uint8_t *)&length, 1);
        coap_set_payload(response, buffer, length);
    }
}


void res_alert_activate(void){
    temperature_state = TEMPERATURE_OK;
    humidity_state = HUMIDITY_OK;
    light_state = LIGHT_OK;
    coap_activate_resource(&res_alert, "alert");
}
