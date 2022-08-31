package it.unipi.dii.iot.smartgreenhouse.utils;

public class Message {
    String n;
    int v;
    String u;
    int id;

    public String getTopic() {
        return n;
    }
    public int getSample() {
        return v;
    }
    public String getUnit() {
        return u;
    }
    public int getMachineId() {
        return id;
    }
}
