#include "contiki.h"
#include "net/routing/routing.h"
#include "mqtt.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/button-hal.h"
#include "os/dev/leds.h"
#include "os/sys/log.h"
#include "mqtt-sensor.h"
#include "../sensors/humidity.h"
#include "../sensors/light.h"
#include "../sensors/temperature.h"
#include "../sensors/utils.h"


#include <string.h>
#include <strings.h>

#define OFF 0
#define ON 1

#define MQTT_HUMIDITY_OK                "0"
#define MQTT_HUMIDITY_ERROR             "1"
#define MQTT_TEMPERATURE_OK             "2"
#define MQTT_TEMPERATURE_ERROR          "3"
#define MQTT_LIGHT_OK                   "4"
#define MQTT_LIGHT_ERROR                "5"

#define HUMIDITY_OK      		    0
#define HUMIDITY_ERROR              1
#define TEMPERATURE_OK              2
#define TEMPERATURE_ERROR           3
#define LIGHT_OK      		        4
#define LIGHT_ERROR                 5

static uint8_t temperature_state;
static uint8_t humidity_state;
static uint8_t light_state;
/*---------------------------------------------------------------------------*/
#define LOG_MODULE "mqtt-sensor"
#define LOG_LEVEL LOG_LEVEL_APP

/*---------------------------------------------------------------------------*/
// MQTT broker address
#define MQTT_CLIENT_BROKER_IP_ADDR "fd00::1"

static const char *broker_ip = MQTT_CLIENT_BROKER_IP_ADDR;

// default config values
#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_PUBLISH_INTERVAL    (30 * CLOCK_SECOND)

// we assume that the broker does not require authentication

/*---------------------------------------------------------------------------*/
// various states
static uint8_t state;

#define STATE_INIT    		  0
#define STATE_NET_OK    	  1
#define STATE_CONNECTING      2
#define STATE_CONNECTED       3
#define STATE_SUBSCRIBED      4
#define STATE_DISCONNECTED    5

/*---------------------------------------------------------------------------*/
PROCESS_NAME(mqtt_client_process);
AUTOSTART_PROCESSES(&mqtt_client_process);
/*---------------------------------------------------------------------------*/

// maximum TCP segment size for outgoing segments of our socket
#define MAX_TCP_SEGMENT_SIZE    32
#define CONFIG_IP_ADDR_STR_LEN   64

static mqtt_status_t status;
static char broker_address[CONFIG_IP_ADDR_STR_LEN];
/*---------------------------------------------------------------------------*/
/* Buffers for Client ID and Topics
 * Make sure they are large enough to hold the entire respective string
 */
#define BUFFER_SIZE 64

static char client_id[BUFFER_SIZE];
static char humidity_topic[BUFFER_SIZE];
static char light_topic[BUFFER_SIZE];
static char temperature_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];

static int node_id;

// periodic timer to check the state of the MQTT client
#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)
static struct etimer periodic_timer;

/*---------------------------------------------------------------------------*/
/* The main MQTT buffers
 * We will need to increase if we start publishing more data
 */
#define APP_BUFFER_SIZE 512
static char humidity_buffer[APP_BUFFER_SIZE];
static char light_buffer[APP_BUFFER_SIZE];
static char temperature_buffer[APP_BUFFER_SIZE];
/*---------------------------------------------------------------------------*/
static struct mqtt_message *msg_ptr = 0;

static struct mqtt_connection conn;

/*---------------------------------------------------------------------------*/
PROCESS(mqtt_client_process, "MQTT Client");

/*---------------------------------------------------------------------------*/
static void pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len){
    if(strcmp(topic, sub_topic) == 0) {
        // LEDS management
        if(strcmp((const char *)chunk, MQTT_TEMPERATURE_ERROR)==0){
            LOG_INFO("WARNING : MQTT_TEMPERATURE_ERROR\n");
    	    LOG_INFO("Switch ON ventilation system  \n");
            temperature_state = TEMPERATURE_ERROR;
            //leds_single_on(LEDS_RED);
            //leds_single_off(LEDS_GREEN);
            leds_off(LEDS_ALL);
            leds_on(LEDS_RED);
            process_post(&temperature_sensor_process, TEMPERATURE_EVENT_ALERT, (int*) ON);

        }else if (strcmp((const char *)chunk, MQTT_TEMPERATURE_OK)==0){
            LOG_INFO("STATE : MQTT_TEMPERATURE_OK\n");
   	    LOG_INFO("Switch OFF ventilation system  \n");
            if (humidity_state == HUMIDITY_OK && light_state == LIGHT_OK) {
                //leds_single_off(LEDS_RED);
                //leds_single_on(LEDS_GREEN);
                leds_off(LEDS_ALL);
                leds_on(LEDS_GREEN);
            }
            temperature_state = TEMPERATURE_OK;
            process_post(&temperature_sensor_process, TEMPERATURE_EVENT_ALERT, (int*) OFF);

        }else if (strcmp((const char *)chunk, MQTT_HUMIDITY_ERROR)==0){
            LOG_INFO("WARNING : MQTT_HUMIDITY_ERROR\n");
    	    LOG_INFO("Waiting for irrigation command \n");
            //leds_single_on(LEDS_RED);
            //leds_single_off(LEDS_GREEN);
            leds_off(LEDS_ALL);
            leds_on(LEDS_RED);
            humidity_state = HUMIDITY_ERROR;

        }else if (strcmp((const char *)chunk, MQTT_HUMIDITY_OK)==0){
            LOG_INFO("STATE : MQTT_HUMIDITY_OK\n");
            LOG_INFO("Irrigation completed \n");
            if (temperature_state == TEMPERATURE_OK && light_state == LIGHT_OK) {
                //leds_single_off(LEDS_RED);
                //leds_single_on(LEDS_GREEN);
                leds_off(LEDS_ALL);
                leds_on(LEDS_GREEN);
            }
            humidity_state = HUMIDITY_OK;
         

        }else if (strcmp((const char *)chunk, MQTT_LIGHT_ERROR)==0){
            LOG_INFO("WARNING : MQTT_LIGHT_ERROR\n");
    	    LOG_INFO("Turn ON lighting system \n");
            //leds_single_on(LEDS_RED);
            //leds_single_off(LEDS_GREEN);
            leds_off(LEDS_ALL);
            leds_on(LEDS_RED);
            light_state = LIGHT_ERROR;

        }else if (strcmp((const char *)chunk, MQTT_LIGHT_OK)==0){
            LOG_INFO("STATE : MQTT_LIGHT_OK\n");
            LOG_INFO("Turn OFF lighting system  \n");
            if(temperature_state == TEMPERATURE_OK && humidity_state == HUMIDITY_OK) {
                //leds_single_off(LEDS_RED);
                //leds_single_on(LEDS_GREEN);
                leds_off(LEDS_ALL);
                leds_on(LEDS_GREEN);
            }
            light_state = LIGHT_OK;

        }else{
            LOG_INFO("UNKNOWN COMMAND\n");
        }
        return;
    }
}
/*---------------------------------------------------------------------------*/
// publish in the specific topic
static void publish(char* topic, char* buffer){
    LOG_INFO("Publishing %s in the topic %s.\n", buffer, topic);
    int status = mqtt_publish(&conn, NULL, topic, (uint8_t *)buffer, strlen(buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
  
    switch(status) {
        case MQTT_STATUS_OK:
        return;

        case MQTT_STATUS_NOT_CONNECTED_ERROR: {
            LOG_ERR("Publishing failed. Error: MQTT_STATUS_NOT_CONNECTED_ERROR.\n");
            state = STATE_DISCONNECTED;
            return;
        }

        case MQTT_STATUS_OUT_QUEUE_FULL: {
            LOG_ERR("Publishing failed. Error: MQTT_STATUS_OUT_QUEUE_FULL.\n");
            mqtt_disconnect(&conn);
            state = STATE_DISCONNECTED;
            return;
        }

        default:
            LOG_ERR("Publishing failed. Error: unknown.\n");
            return;
    }
}

static void sensors_emulation(process_event_t event, int sample){
    if(event == TEMPERATURE_SAMPLE_EVENT){
        json_sample(temperature_buffer, APP_BUFFER_SIZE, "temperature", sample, "C", node_id);
        publish(temperature_topic, temperature_buffer);

    }else if(event == HUMIDITY_SAMPLE_EVENT){
        json_sample(humidity_buffer, APP_BUFFER_SIZE, "humidity", sample, "RH", node_id);
        publish(humidity_topic, humidity_buffer);

    }else if(event == LIGHT_SAMPLE_EVENT){
        json_sample(light_buffer, APP_BUFFER_SIZE, "light", sample, "LM", node_id);
        publish(light_topic, light_buffer);
    }
}

static void load_sensors_processes(){

    process_start(&temperature_sensor_process, NULL);
    process_post(&temperature_sensor_process, TEMPERATURE_EVENT_SUB, &mqtt_client_process);

    process_start(&humidity_sensor_process, NULL);
    process_post(&humidity_sensor_process, HUMIDITY_EVENT_SUB, &mqtt_client_process);

    process_start(&light_sensor_process, NULL);
    process_post(&light_sensor_process, LIGHT_EVENT_SUB, &mqtt_client_process);
  
    temperature_state = TEMPERATURE_OK;
	humidity_state = HUMIDITY_OK;
    light_state = LIGHT_OK;
    
}

static bool sensor_event(process_event_t event){
    if(event == TEMPERATURE_SAMPLE_EVENT || event == HUMIDITY_SAMPLE_EVENT || event == LIGHT_SAMPLE_EVENT) {
        return true;
    }
    return false;
}

/*---------------------------------------------------------------------------*/

static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data){
    switch(event) {

        case MQTT_EVENT_CONNECTED: {
            LOG_INFO("Application has a MQTT connection\n");
            state = STATE_CONNECTED;
            break;
        }

        case MQTT_EVENT_DISCONNECTED: {
            LOG_INFO("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));
            state = STATE_DISCONNECTED;
            process_poll(&mqtt_client_process);
            break;
        }

        case MQTT_EVENT_PUBLISH: {
            msg_ptr = data;
            pub_handler(msg_ptr->topic, strlen(msg_ptr->topic), msg_ptr->payload_chunk, msg_ptr->payload_length);
            break;
        }

        case MQTT_EVENT_SUBACK: {
            #if MQTT_311
            mqtt_suback_event_t *suback_event = (mqtt_suback_event_t *)data;

            if(suback_event->success) {
                LOG_INFO("Application is subscribed to topic successfully\n");
            } else {
                LOG_INFO("Application failed to subscribe to topic (ret code %x)\n", suback_event->return_code);
            }
            #else
            LOG_INFO("Application is subscribed to topic successfully\n");
            #endif
            break;
        }

        case MQTT_EVENT_UNSUBACK: {
            LOG_INFO("Application is unsubscribed to topic successfully\n");
            break;
        }

        case MQTT_EVENT_PUBACK: {
            LOG_INFO("Publishing complete.\n");
            break;
        }

        default:
            LOG_INFO("Application got a unhandled MQTT event: %i\n", event);
            break;
    }
}

static bool have_connectivity(void){
    if(uip_ds6_get_global(ADDR_PREFERRED) == NULL || uip_ds6_defrt_choose() == NULL) {
        return false;
    }
    return true;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mqtt_client_process, ev, data){

    PROCESS_BEGIN();

    LOG_INFO("MQTT Client Process\n");
    // initialize the ClientID as MAC address
    snprintf(client_id, BUFFER_SIZE, "%02x%02x%02x%02x%02x%02x",
                     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

    // broker registration
    mqtt_register(&conn, &mqtt_client_process, client_id, mqtt_event, MAX_TCP_SEGMENT_SIZE);

	  
    node_id = linkaddr_node_addr.u8[7];
  
    load_sensors_processes();
  
    state=STATE_INIT;

    //leds_single_on(LEDS_GREEN);
    //leds_single_on(LEDS_RED);
    leds_on(LEDS_ALL);

    // initialize periodic timer to check the status
    etimer_set(&periodic_timer, STATE_MACHINE_PERIODIC);

    // main loop
    while(1) {

        PROCESS_YIELD();
        if((ev == PROCESS_EVENT_TIMER && data == &periodic_timer) || ev == PROCESS_EVENT_POLL){

            if(state==STATE_INIT){
	            if(have_connectivity()==true)
		            state = STATE_NET_OK;
		    }
		  
		    if(state == STATE_NET_OK){
	            // connect to MQTT server
			    LOG_INFO("Connecting!\n");
			  
			    memcpy(broker_address, broker_ip, strlen(broker_ip));
			  
			    mqtt_connect(&conn, broker_address, DEFAULT_BROKER_PORT, (DEFAULT_PUBLISH_INTERVAL * 3) / CLOCK_SECOND, MQTT_CLEAN_SESSION_ON);
			    state = STATE_CONNECTING;
		    }
		  
		    if(state==STATE_CONNECTED){
	            // subscribe to a topic
                char topic[64];
                sprintf(topic, "alarm_%d", node_id);
                strcpy(sub_topic,topic);
                strcpy(humidity_topic, "humidity");
                strcpy(light_topic, "light");
                strcpy(temperature_topic, "temperature");
			    status = mqtt_subscribe(&conn, NULL, sub_topic, MQTT_QOS_LEVEL_0);

			    LOG_INFO("Subscribing!\n");

			    if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
		            LOG_ERR("Tried to subscribe but command queue was full!\n");
			    	PROCESS_EXIT();
			    }
			  
			    state = STATE_SUBSCRIBED;
                //leds_single_off(LEDS_RED);
                leds_off(LEDS_ALL);
                leds_on(LEDS_GREEN);

		    }
            else if ( state == STATE_DISCONNECTED ){
                LOG_ERR("Disconnected form MQTT broker\n");
                state = STATE_INIT;
                // recover from error
            }
      
        etimer_set(&periodic_timer, STATE_MACHINE_PERIODIC);
      
        }else if(sensor_event(ev) && state == STATE_SUBSCRIBED){
            sensors_emulation(ev, *((int *)data));
        }

        else if(ev == button_hal_press_event && state == STATE_SUBSCRIBED){
            process_post(&humidity_sensor_process, HUMIDITY_EVENT_ALERT, (int*) ON);
        }
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
