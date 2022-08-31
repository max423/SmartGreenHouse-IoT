package it.unipi.dii.iot.smartgreenhouse;

import it.unipi.dii.iot.smartgreenhouse.coap.CoapCollectorServer;
import it.unipi.dii.iot.smartgreenhouse.mqtt.MqttCollector;
public final class Collector {

    public static void main(String[] args) {
        //Create and start the mqtt collector
        MqttCollector mqttcollector = new MqttCollector();
        mqttcollector.start();

        //create and start the server collector
        CoapCollectorServer server = new CoapCollectorServer();
        server.start();
    }
}
