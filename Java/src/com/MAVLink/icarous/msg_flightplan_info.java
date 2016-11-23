/* AUTO-GENERATED FILE.  DO NOT MODIFY.
 *
 * This class was automatically generated by the
 * java mavlink generator tool. It should not be modified by hand.
 */

// MESSAGE FLIGHTPLAN_INFO PACKING
package com.MAVLink.icarous;
import com.MAVLink.MAVLinkPacket;
import com.MAVLink.Messages.MAVLinkMessage;
import com.MAVLink.Messages.MAVLinkPayload;
        
/**
* Flight plan information 
*/
public class msg_flightplan_info extends MAVLinkMessage{

    public static final int MAVLINK_MSG_ID_FLIGHTPLAN_INFO = 221;
    public static final int MAVLINK_MSG_LENGTH = 17;
    private static final long serialVersionUID = MAVLINK_MSG_ID_FLIGHTPLAN_INFO;


      
    /**
    * Standoff distance
    */
    public float standoffDist;
      
    /**
    * Maximum horizontal deviation from flight plan
    */
    public float maxHorDev;
      
    /**
    * Maximum vertical deviation from flight plan
    */
    public float maxVerDev;
      
    /**
    * Number of waypoints
    */
    public int numWaypoints;
      
    /**
    * Message type, 0-new flight plan, 1-update existing flight plan
    */
    public short msgType;
    

    /**
    * Generates the payload for a mavlink message for a message of this type
    * @return
    */
    public MAVLinkPacket pack(){
        MAVLinkPacket packet = new MAVLinkPacket(MAVLINK_MSG_LENGTH);
        packet.sysid = 255;
        packet.compid = 190;
        packet.msgid = MAVLINK_MSG_ID_FLIGHTPLAN_INFO;
              
        packet.payload.putFloat(standoffDist);
              
        packet.payload.putFloat(maxHorDev);
              
        packet.payload.putFloat(maxVerDev);
              
        packet.payload.putInt(numWaypoints);
              
        packet.payload.putUnsignedByte(msgType);
        
        return packet;
    }

    /**
    * Decode a flightplan_info message into this class fields
    *
    * @param payload The message to decode
    */
    public void unpack(MAVLinkPayload payload) {
        payload.resetIndex();
              
        this.standoffDist = payload.getFloat();
              
        this.maxHorDev = payload.getFloat();
              
        this.maxVerDev = payload.getFloat();
              
        this.numWaypoints = payload.getInt();
              
        this.msgType = payload.getUnsignedByte();
        
    }

    /**
    * Constructor for a new message, just initializes the msgid
    */
    public msg_flightplan_info(){
        msgid = MAVLINK_MSG_ID_FLIGHTPLAN_INFO;
    }

    /**
    * Constructor for a new message, initializes the message with the payload
    * from a mavlink packet
    *
    */
    public msg_flightplan_info(MAVLinkPacket mavLinkPacket){
        this.sysid = mavLinkPacket.sysid;
        this.compid = mavLinkPacket.compid;
        this.msgid = MAVLINK_MSG_ID_FLIGHTPLAN_INFO;
        unpack(mavLinkPacket.payload);        
    }

              
    /**
    * Returns a string with the MSG name and data
    */
    public String toString(){
        return "MAVLINK_MSG_ID_FLIGHTPLAN_INFO -"+" standoffDist:"+standoffDist+" maxHorDev:"+maxHorDev+" maxVerDev:"+maxVerDev+" numWaypoints:"+numWaypoints+" msgType:"+msgType+"";
    }
}
        