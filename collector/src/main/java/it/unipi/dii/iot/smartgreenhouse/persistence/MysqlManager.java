package it.unipi.dii.iot.smartgreenhouse.persistence;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import it.unipi.dii.iot.smartgreenhouse.utils.Message;

/**
 * MySql Queries Managers
 */
public class MysqlManager {
    Connection conn;
    
    public MysqlManager(Connection conn) {this.conn = conn;}

    /**
     * Add a new Sample to MySql
     * @param msg The object Message which contains all information
     * @return true if operation is successfully executed, false otherwise
     */
    public boolean insertSample(Message msg) {
        String query = "INSERT INTO "+ msg.getTopic() + " (sample, unit, machineId) "
        + " VALUES ('"+msg.getSample()+"', '"+msg.getUnit()+"','"+msg.getMachineId()+"');";
        try{
            PreparedStatement ps = conn.prepareStatement(query);
			ps.executeUpdate();
        }catch(SQLException e){
            e.printStackTrace();
            return false;
        }
        return true;
    }

}


