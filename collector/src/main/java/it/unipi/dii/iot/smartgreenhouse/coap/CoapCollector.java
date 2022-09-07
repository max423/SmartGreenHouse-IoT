package it.unipi.dii.iot.smartgreenhouse.coap;

import java.net.InetAddress;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapHandler;
import org.eclipse.californium.core.CoapObserveRelation;
import org.eclipse.californium.core.CoapResponse;
import org.eclipse.californium.core.coap.CoAP.*;
import org.eclipse.californium.core.coap.MediaTypeRegistry;
import com.google.gson.Gson;
import com.google.gson.JsonParseException;

import it.unipi.dii.iot.smartgreenhouse.persistence.MysqlDriver;
import it.unipi.dii.iot.smartgreenhouse.persistence.MysqlManager;
import it.unipi.dii.iot.smartgreenhouse.utils.Message;
import it.unipi.dii.iot.smartgreenhouse.utils.Sensor;

// CoAP collector collects and saves telemetry data in a database,
// and sends allert messages to nodes

public class CoapCollector {
    CoapClient client;
    CoapClient alertClient;
    MysqlManager mysqlMan;
    CoapObserveRelation relation;
    Boolean temperatureMaxValueExceeded;
    Boolean humidityMinValueExceeded;
    Boolean lightMinValueExceeded;
    String resource;
    String nodeIp;
    int sensorState;
    String loggingColor;

    public static final int HUMIDITY_THRESHOLD = 40;
    public static final int TEMPERATURE_THRESHOLD = 26;
    public static final int LIGHT_THRESHOLD = 20;

    public static final int HUMIDITY_OK = 0;
    public static final int HUMIDITY_ERROR = 1;
    public static final int TEMPERATURE_OK = 2;
    public static final int TEMPERATURE_ERROR = 3;
    public static final int LIGHT_OK = 4;
    public static final int LIGHT_ERROR = 5;
    public static final String[] colors = {"\u001B[91m", "\u001B[92m", "\u001B[93m", "\u001B[94m"}; //red, green, yellow, blue
    public static final String ANSI_RESET = "\u001B[0m";

    public CoapCollector(Sensor s){
        // create new coap client
        client = new CoapClient(s.getUri());
        nodeIp = s.getNodeIp();
        mysqlMan = new MysqlManager(MysqlDriver.getInstance().openConnection());
        alertClient = new CoapClient("coap://[" + s.getNodeIp() + "]/alert");

        temperatureMaxValueExceeded = false;
        humidityMinValueExceeded = false;
        lightMinValueExceeded = false;

        resource = s.getResourcePath();
    }

    public void onError() {
        System.err.println("Failed");
    }

    public void startObserving(){
        relation = client.observe(new CoapHandler() {
            public void onLoad(CoapResponse response) {
                try{
                    // read responde JSON
                    String jsonMessage = new String(response.getResponseText());
                    Gson gson = new Gson();
                    Message msg = gson.fromJson(jsonMessage, Message.class);
                    if (msg.getSample() == -1) {
                        return;
                    }
                    loggingColor = colors[msg.getMachineId()%colors.length];
                    printToConsole("Machine: " + msg.getMachineId() +"] value is " + msg.getSample() + msg.getUnit() +" measurement on "+ resource);
                    // ADD to DB
                    mysqlMan.insertSample(msg);

                    String topic = msg.getTopic();
                    int sample = msg.getSample();
                    sensorState = -1;

                    // verify threshold
                    switch (topic) {
                        case "humidity":
                            if (sample < HUMIDITY_THRESHOLD) {
                                printToConsole("WARNING Low humidity detected, mim threshold exceeded!");
                                humidityMinValueExceeded = true;
                                sensorState = HUMIDITY_ERROR;
                            } else if (humidityMinValueExceeded){
                                printToConsole("Groung humidity has returned to normal above the threshold");
                                humidityMinValueExceeded = false;
                                sensorState = HUMIDITY_OK;
                            }
                            break;
                        case "temperature":
                            if (sample > TEMPERATURE_THRESHOLD) {
                                printToConsole("WARNING high temperature detected, max threshold exceeded!");
                                temperatureMaxValueExceeded = true;
                                sensorState = TEMPERATURE_ERROR;
                            } else if (temperatureMaxValueExceeded) {
                                printToConsole("Temperature has returned to normal under the threshold");
                                temperatureMaxValueExceeded = false;
                                sensorState = TEMPERATURE_OK;
                            }
                            break;
                        case "light":
                            if (sample < LIGHT_THRESHOLD) {
                                printToConsole("WARNING low sunlight detected, mim threshold exceeded!");
                                lightMinValueExceeded = true;
                                sensorState = LIGHT_ERROR;
                            } else if (lightMinValueExceeded){
                                printToConsole("Sunlight has returned to normal above the threshold");
                                lightMinValueExceeded = false;
                                sensorState = LIGHT_OK;
                            }
                            break;

                        default:
                            break;
                    }

                    if (sensorState != -1) {

                         switch (sensorState) {
                            case HUMIDITY_OK:
                                printToConsole("State has changed, sending POST request to the node with state= HUMIDITY_OK to " + alertClient.getURI());
                                break;
                            case HUMIDITY_ERROR:
                                printToConsole("State has changed, sending POST request to the node with state= HUMIDITY_ERROR to " + alertClient.getURI());
                                break;
                            case TEMPERATURE_OK:
                                printToConsole("State has changed, sending POST request to the node with state= TEMPERATURE_OK to " + alertClient.getURI());
                                break;
                            case TEMPERATURE_ERROR:
                                printToConsole("State has changed, sending POST request to the node with state= TEMPERATURE_ERROR to " + alertClient.getURI());
                                break;
                            case LIGHT_OK:
                                printToConsole("State has changed, sending POST request to the node with state= LIGHT_OK to " + alertClient.getURI());
                                break;
                            case LIGHT_ERROR:
                                printToConsole("State has changed, sending POST request to the node with state= LIGHT_ERROR to " + alertClient.getURI());
                                break;
                            default:
                                break;
                        }
                         // send post request to the node
                         alertClient.post(new CoapHandler() {

                            @Override
                            public void onLoad(CoapResponse response) {
                                String message = response.getResponseText();
                                ResponseCode code = response.getCode();
                                if (code != ResponseCode.BAD_REQUEST) {
                                    int responseSensorState;
                                    try {
                                        responseSensorState = Integer.parseInt(message);
                                    } catch (NumberFormatException e) {
                                        responseSensorState = -1;
                                    }
                                    if (responseSensorState != sensorState){
                                        printToConsole("Unable to change status on node with uri " + alertClient.getURI());
                                    } else {
                                      
                                        switch (sensorState) {
                                                                    case HUMIDITY_OK:
                                                                        printToConsole("Changed status to state= HUMIDITY_OK on node with uri " + alertClient.getURI());
                                                                        break;
                                                                    case HUMIDITY_ERROR:
                                                                        printToConsole("Changed status to state= HUMIDITY_ERROR on node with uri " + alertClient.getURI());
                                                                        break;
                                                                    case TEMPERATURE_OK:
                                                                        printToConsole("Changed status to state= TEMPERATURE_OK on node with uri " + alertClient.getURI());
                                                                        break;
                                                                    case TEMPERATURE_ERROR:
                                                                        printToConsole("Changed status to state= TEMPERATURE_ERROR  on node with uri " + alertClient.getURI());
                                                                        break;
                                                                    case LIGHT_OK:
                                                                        printToConsole("Changed status to state= LIGHT_OK on node with uri " + alertClient.getURI());
                                                                        break;
                                                                    case LIGHT_ERROR:
                                                                        printToConsole("Changed status to state= LIGHT_ERROR on node with uri " + alertClient.getURI());
                                                                        break;
                                                                    default:
                                                                        break;
                                                                }
                                    }
                                } else {
                                    printToConsole("400 BAD REQUEST on node with uri " + alertClient.getURI());
                                }
                            }

                            @Override
                            public void onError() {
                                printToConsole("Failed");
                            }

                        }, "state=" + sensorState, MediaTypeRegistry.TEXT_PLAIN);
                    }
                }catch(JsonParseException e){
                    System.out.println("***JsonParseError***");
                }
            }

            public void onError() {
                System.err.println("Failed");
            }
        });
    }

    public void cancelObserving(){
        relation.proactiveCancel();
    }

    public void printToConsole(String log) {
        System.out.println(loggingColor + "COAP - " + log + ANSI_RESET);
    }
}
