package it.unipi.dii.iot.smartgreenhouse.coap;

import org.eclipse.californium.core.CoapServer;

public class CoapCollectorServer extends CoapServer {
    //Creates a new CoapServer.
    public CoapCollectorServer() {
        this.add(new CoapRegistration("coapReg"));
		this.start();
    }
}
