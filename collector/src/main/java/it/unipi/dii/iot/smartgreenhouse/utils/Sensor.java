package it.unipi.dii.iot.smartgreenhouse.utils;

public class Sensor {
    private String nodeIp;
    private String resourcePath;
    private String uri;

    public Sensor(String nodeIp, String resourcePath){
        this.nodeIp = nodeIp;
        this.resourcePath = resourcePath;
        this.uri = "coap://[" + this.nodeIp + "]/" + this.resourcePath;
    }

    public String getNodeIp() {
        return nodeIp;
    }

    public void setNodeIp(String nodeIp) {
        this.nodeIp = nodeIp;
    }

    public String getResourcePath() {
        return resourcePath;
    }

    public void setResourcePath(String resourcePath) {
        this.resourcePath = resourcePath;
    }

    public String getUri() {
        return uri;
    }

}
