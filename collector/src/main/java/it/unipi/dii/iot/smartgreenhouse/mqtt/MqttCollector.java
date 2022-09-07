package it.unipi.dii.iot.smartgreenhouse.mqtt;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import com.google.gson.Gson;
import com.google.gson.JsonParseException;

import it.unipi.dii.iot.smartgreenhouse.persistence.MysqlDriver;
import it.unipi.dii.iot.smartgreenhouse.persistence.MysqlManager;
import it.unipi.dii.iot.smartgreenhouse.utils.Message;
import it.unipi.dii.iot.smartgreenhouse.utils.Utils;

//MQTT collector collects and saves telemetry data in a database,
//and send alert messages to nodes
public class MqttCollector implements MqttCallback{
    String topic;
    String broker;
    String clientId;
    MqttClient mqttClient;
    MysqlManager mysqlMan;
    String loggingColor;
    List<Integer> temperatureWarningNodes = new ArrayList<>();
    List<Integer> humidityWarningNodes = new ArrayList<>();
    List<Integer> lightWarningNodes = new ArrayList<>();

    public static final int HUMIDITY_THRESHOLD = 40;
    public static final int TEMPERATURE_THRESHOLD = 26;
    public static final int LIGHT_THRESHOLD = 20;
    public static final String HUMIDITY_OK = "0";
    public static final String HUMIDITY_ERROR = "1";
    public static final String TEMPERATURE_OK = "2";
    public static final String TEMPERATURE_ERROR = "3";
    public static final String LIGHT_OK = "4";
    public static final String LIGHT_ERROR = "5";

    public static final String[] colors = {"\u001B[95m", "\u001B[96m"}; //purple, cyan
    public static final String ANSI_RESET = "\u001B[0m";
    // create a new MQTT collector
    public MqttCollector(){
        Properties configurationParameters = Utils.readConfigurationParameters();
        topic = "#";
        broker = "tcp://"+configurationParameters.getProperty("mqttBrokerIp")+":"+configurationParameters.getProperty("mqttBrokerPort");
        clientId = "JavaCollector";
        mysqlMan = new MysqlManager(MysqlDriver.getInstance().openConnection());
    }

     // start the MQTT collector and subscribe to the topics of interest
    public void start(){
        try {
            mqttClient = new MqttClient(broker, clientId);
            System.out.println("Connecting to broker: "+ broker);
            mqttClient.setCallback(this);
            mqttClient.connect();
            mqttClient.subscribe("humidity");
            mqttClient.subscribe("light");
            mqttClient.subscribe("temperature");
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    public void stop(){
        try {
            mqttClient.close(true);
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

	// publish the mqtt message
    public void publish(String content, int node){
        try{
            MqttMessage message = new MqttMessage(content.getBytes());
            this.mqttClient.publish("alarm_" + node, message);
        }catch(MqttException e){
            e.printStackTrace();
        }
    }

    public void messageArrived(String topic, MqttMessage message) throws Exception {
        try{
		// read response JSON
            String jsonMessage = new String(message.getPayload());
            Gson gson = new Gson();
            Message msg = gson.fromJson(jsonMessage, Message.class);

            loggingColor = colors[msg.getMachineId()%colors.length];
	        printToConsole("Machine: " + msg.getMachineId() +"] value is " + msg.getSample() + msg.getUnit() +" measurement on "+ topic );
            
		// ADD to DB
		mysqlMan.insertSample(msg);

		// verify threshold
            switch(msg.getTopic()) {
                case "temperature":
                    if(msg.getSample() > TEMPERATURE_THRESHOLD && !temperatureWarningNodes.contains((Integer) msg.getMachineId())){
                        printToConsole("WARNING High temperature detected, max threshold exceeded! Sending alarm msg to node: " + msg.getMachineId());
                        publish(TEMPERATURE_ERROR, msg.getMachineId());
                        temperatureWarningNodes.add((Integer) msg.getMachineId());
                    }else if(msg.getSample() <= TEMPERATURE_THRESHOLD && temperatureWarningNodes.contains((Integer) msg.getMachineId())){
                        printToConsole("Temperature has returned to normal below the threshold");
                        publish(TEMPERATURE_OK, msg.getMachineId());
                        temperatureWarningNodes.remove((Integer) msg.getMachineId());
                    }
                    break;
                case "humidity":
                    if(msg.getSample() < HUMIDITY_THRESHOLD && !humidityWarningNodes.contains((Integer) msg.getMachineId())){
                        printToConsole("WARNING low Ground humidity detected, min threshold exceeded! Sending alarm msg to node: " + msg.getMachineId());
                        publish(HUMIDITY_ERROR, msg.getMachineId());
                        humidityWarningNodes.add((Integer) msg.getMachineId());
                    }else if(msg.getSample() >= HUMIDITY_THRESHOLD && humidityWarningNodes.contains((Integer) msg.getMachineId())){
			printToConsole("Ground humidity has returned to normal above the threshold");
                        publish(HUMIDITY_OK, msg.getMachineId());
                        humidityWarningNodes.remove((Integer) msg.getMachineId());
                    }
                    break;
                case "light":
                    if(msg.getSample() < LIGHT_THRESHOLD && !lightWarningNodes.contains((Integer) msg.getMachineId())){
                        printToConsole("WARNING low sunlight detected, min threshold exceeded! Sending alarm msg to node: " + msg.getMachineId());
                        publish(LIGHT_ERROR, msg.getMachineId());
                        lightWarningNodes.add((Integer) msg.getMachineId());
                    }else if(msg.getSample() >= LIGHT_THRESHOLD && lightWarningNodes.contains((Integer) msg.getMachineId())){
                        printToConsole("Sunlight has returned to normal above the threshold");
                        publish(LIGHT_OK, msg.getMachineId());
                        lightWarningNodes.remove((Integer) msg.getMachineId());
                    }
                    break;
                default:
                    break;
            }
        }catch(JsonParseException e){
            System.out.println(e);
        }
         
	}

    @Override
    public void connectionLost(Throwable cause) {
        // TODO Auto-generated method stub
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // TODO Auto-generated method stub
    }

    public void printToConsole(String log) {
        System.out.println(loggingColor + "MQTT - " + log + ANSI_RESET);
    }
}
