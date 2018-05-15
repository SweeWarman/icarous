//
// Created by Swee Balachandran on 12/22/17.
//
#include <Icarous_msg.h>
#include <CWrapper/TrafficMonitor_proxy.h>
#include "traffic.h"

CFE_EVS_BinFilter_t  TRAFFIC_EventFilters[] =
        {  /* Event ID    mask */
                {TRAFFIC_STARTUP_INF_EID,       0x0000},
                {TRAFFIC_COMMAND_ERR_EID,       0x0000},
        }; /// Event ID definitions

/* Application entry points */
void TRAFFIC_AppMain(void){

    int32 status;
    uint32 RunStatus = CFE_ES_APP_RUN;

    TRAFFIC_AppInit();

    while(CFE_ES_RunLoop(&RunStatus) == TRUE){
        status = CFE_SB_RcvMsg(&TrafficAppData.Traffic_MsgPtr, TrafficAppData.Traffic_Pipe, 10);

        if (status == CFE_SUCCESS)
        {
            TRAFFIC_ProcessPacket();
        }
    }

    TRAFFIC_AppCleanUp();

    CFE_ES_ExitApp(RunStatus);
}

void TRAFFIC_AppInit(void) {

    memset(&TrafficAppData, 0, sizeof(TrafficAppData_t));

    int32 status;

    // Register the app with executive services
    CFE_ES_RegisterApp();

    // Register the events
    CFE_EVS_Register(TRAFFIC_EventFilters,
                     sizeof(TRAFFIC_EventFilters) / sizeof(CFE_EVS_BinFilter_t),
                     CFE_EVS_BINARY_FILTER);

    // Create pipe to receive SB messages
    status = CFE_SB_CreatePipe(&TrafficAppData.Traffic_Pipe, /* Variable to hold Pipe ID */
                               TRAFFIC_PIPE_DEPTH,       /* Depth of Pipe */
                               TRAFFIC_PIPE_NAME);       /* Name of pipe */

    //Subscribe to plexil output messages from the SB
    CFE_SB_Subscribe(SERVICE_TRAFFIC_MID, TrafficAppData.Traffic_Pipe);
    CFE_SB_Subscribe(ICAROUS_TRAFFIC_MID,TrafficAppData.Traffic_Pipe);
    CFE_SB_Subscribe(ICAROUS_POSITION_MID,TrafficAppData.Traffic_Pipe);

    // Initialize all messages that this App generates
    CFE_SB_InitMsg(&trafficServiceResponse, SERVICE_TRAFFIC_RESPONSE_MID, sizeof(service_t), TRUE);
    CFE_SB_InitMsg(&trackBands, ICAROUS_VISBAND_MID, sizeof(visbands_t), TRUE);

    // Send event indicating app initialization
    CFE_EVS_SendEvent(TRAFFIC_STARTUP_INF_EID, CFE_EVS_INFORMATION,
                      "TRAFFIC App Initialized. Version %d.%d",
                      TRAFFIC_MAJOR_VERSION,
                      TRAFFIC_MINOR_VERSION);

    TrafficAppData.fdata = new_FlightData("../ram/icarous.txt");
    TrafficAppData.tfMonitor = new_TrafficMonitor(TrafficAppData.fdata);

}

void TRAFFIC_AppCleanUp(){
    // Do clean up here
    delete_TrafficMonitor(TrafficAppData.tfMonitor);
    delete_FlightData(TrafficAppData.fdata);
}

void TRAFFIC_ProcessPacket(){

    CFE_SB_MsgId_t  MsgId;
    MsgId = CFE_SB_GetMsgId(TrafficAppData.Traffic_MsgPtr);

    switch(MsgId){

        case ICAROUS_TRAFFIC_MID:{
            object_t* msg;
            msg = (object_t*) TrafficAppData.Traffic_MsgPtr;

            FlightData_AddTraffic(TrafficAppData.fdata,msg->index,msg->latitude,msg->longitude,msg->altiude,msg->vx,msg->vy,msg->vz);
            break;
        }

        case ICAROUS_POSITION_MID:{
            position_t* msg;
            msg = (position_t*) TrafficAppData.Traffic_MsgPtr;

            if (msg->aircraft_id != CFE_PSP_GetSpacecraftId()) {
                FlightData_AddTraffic(TrafficAppData.fdata, msg->aircraft_id, msg->latitude, msg->longitude,
                                      msg->altitude_rel, msg->vx, msg->vy, msg->vz);
                OS_printf("received traffic from aircraft_id %s", msg->aircraft_id);
            }

            break;
        }

        case SERVICE_TRAFFIC_MID: {
            service_t* msg;
            msg = (service_t*) TrafficAppData.Traffic_MsgPtr;
            TrafficServiceHandler(msg);
            break;
        }
    }
    return;
}