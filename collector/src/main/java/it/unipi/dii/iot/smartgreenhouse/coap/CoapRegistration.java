package it.unipi.dii.iot.smartgreenhouse.coap;

import java.net.InetAddress;
import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.server.resources.CoapExchange;
import it.unipi.dii.iot.smartgreenhouse.utils.Sensor;

// implements the CoapRegistration resource
public class CoapRegistration extends CoapResource{

    public CoapRegistration(String name) {
        super(name);
    }
    
    public void handleGET(CoapExchange exchange){
         
        exchange.accept();
        InetAddress sensorIp = exchange.getSourceAddress();
        System.out.println("Registration request from: " +sensorIp);

        Sensor temperatureSensor = new Sensor(sensorIp.getHostAddress(), "temperature");
        observe(temperatureSensor);
        Sensor lightSensor = new Sensor(sensorIp.getHostAddress(), "light");
        observe(lightSensor);
        Sensor humiditySensor = new Sensor(sensorIp.getHostAddress(), "humidity");
        observe(humiditySensor);
    }

    private static void observe(Sensor s) {
		CoapCollector observer = new CoapCollector(s);
        System.out.println("CoapCollector created for " + s.getResourcePath());
		observer.startObserving();
	}
}
