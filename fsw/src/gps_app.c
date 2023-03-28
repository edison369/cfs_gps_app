/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file
 *   This file contains the source code for the GPS App.
 */

/*
** Include Files:
*/
#include "gps_app_events.h"
#include "gps_app_version.h"
#include "gps_app.h"

/*
** global data
*/
GPS_APP_Data_t GPS_APP_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void GPS_APP_Main(void)
{
    int32            status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(GPS_APP_PERF_ID);

    /*
    ** Perform application specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = GPS_APP_Init();
    if (status != CFE_SUCCESS){
        GPS_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** GPS Runloop
    */
    while (CFE_ES_RunLoop(&GPS_APP_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(GPS_APP_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, GPS_APP_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(GPS_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            GPS_APP_ProcessCommandPacket(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(GPS_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "GPS APP: SB Pipe Read Error, App Will Exit");

            GPS_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(GPS_APP_PERF_ID);

    CFE_ES_ExitApp(GPS_APP_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 GPS_APP_Init(void)
{
    int32 status;

    GPS_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize app command execution counters
    */
    GPS_APP_Data.CmdCounter = 0;
    GPS_APP_Data.ErrCounter = 0;

    GPS_APP_Data.latitude = 0;
    GPS_APP_Data.longitude = 0;
    GPS_APP_Data.altitude = 0;
    GPS_APP_Data.satellites = 0;

    /*
    ** Initialize app configuration data
    */
    GPS_APP_Data.PipeDepth = GPS_APP_PIPE_DEPTH;

    strncpy(GPS_APP_Data.PipeName, "GPS_APP_CMD_PIPE", sizeof(GPS_APP_Data.PipeName));
    GPS_APP_Data.PipeName[sizeof(GPS_APP_Data.PipeName) - 1] = 0;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("GPS App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Initialize housekeeping packet (clear user data area).
    */
    CFE_MSG_Init(CFE_MSG_PTR(GPS_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(GPS_APP_HK_TLM_MID),
                 sizeof(GPS_APP_Data.HkTlm));

   /*
   ** Initialize output RF packet.
   */
   CFE_MSG_Init(CFE_MSG_PTR(GPS_APP_Data.OutData.TelemetryHeader), CFE_SB_ValueToMsgId(GPS_APP_RF_DATA_MID),
                sizeof(GPS_APP_Data.OutData));
   GPS_APP_Data.OutData.App_Pckg_Counter = 0;

    /*
    ** Create Software Bus message pipe.
    */
    status = CFE_SB_CreatePipe(&GPS_APP_Data.CommandPipe, GPS_APP_Data.PipeDepth, GPS_APP_Data.PipeName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("GPS App: Error creating pipe, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Subscribe to Housekeeping request commands
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(GPS_APP_SEND_HK_MID), GPS_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("GPS App: Error Subscribing to HK request, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Subscribe to RF command packets
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(GPS_APP_SEND_RF_MID), GPS_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("GPS App: Error Subscribing to RF request, RC = 0x%08lX\n", (unsigned long)status);
        return status;
    }

    /*
    ** Subscribe to Read command packets
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(GPS_APP_READ_MID), GPS_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("GPS App: Error Subscribing to Command, RC = 0x%08lX\n", (unsigned long)status);

        return status;
    }

    /*
    ** Subscribe to ground command packets
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(GPS_APP_CMD_MID), GPS_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("GPS App: Error Subscribing to Command, RC = 0x%08lX\n", (unsigned long)status);

        return status;
    }

    CFE_EVS_SendEvent(GPS_APP_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "GPS App Initialized.%s",
                      GPS_APP_VERSION_STRING);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the GPS    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void GPS_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case GPS_APP_CMD_MID:
            GPS_APP_ProcessGroundCommand(SBBufPtr);
            break;

        case GPS_APP_SEND_HK_MID:
            GPS_APP_ReportHousekeeping((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        case GPS_APP_SEND_RF_MID:
            GPS_APP_ReportRFTelemetry((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        case GPS_APP_READ_MID:
            GPS_APP_ReadSensor((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(GPS_APP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "GPS: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* GPS ground commands                                                     */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void GPS_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process "known" GPS app ground commands
    */
    switch (CommandCode)
    {
        case GPS_APP_NOOP_CC:
            if (GPS_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(GPS_APP_NoopCmd_t)))
            {
                GPS_APP_Noop((GPS_APP_NoopCmd_t *)SBBufPtr);
            }

            break;

        case GPS_APP_RESET_COUNTERS_CC:
            if (GPS_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(GPS_APP_ResetCountersCmd_t)))
            {
                GPS_APP_ResetCounters((GPS_APP_ResetCountersCmd_t *)SBBufPtr);
            }

            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(GPS_APP_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid ground command code: CC = %d", CommandCode);
            break;
    }
}

int32 GPS_APP_ReadSensor(const CFE_MSG_CommandHeader_t *Msg){
  uint8_t *tmp;
  tmp = NULL;

  uC_read_bytes(14, &tmp);

  floatu_t lat_u;
  floatu_t long_u;
  floatu_t alt_u;

  for (int i = 0; i < 12; i++) {
    if(i<4){
      lat_u.bytes[i] = tmp[i];
    }else if(i<8){
      long_u.bytes[i-4] = tmp[i];
    }else{
      alt_u.bytes[i-8] = tmp[i];
    }
  }

  GPS_APP_Data.latitude = lat_u.number;
  GPS_APP_Data.longitude = long_u.number;
  GPS_APP_Data.altitude = alt_u.number;
  GPS_APP_Data.satellites = tmp[12];

  free(tmp);


  return CFE_SUCCESS;
}


int32 GPS_APP_ReportRFTelemetry(const CFE_MSG_CommandHeader_t *Msg){

    /*
    ** Get command execution counters...
    */
    GPS_APP_Data.OutData.CommandErrorCounter = GPS_APP_Data.ErrCounter;
    GPS_APP_Data.OutData.CommandCounter      = GPS_APP_Data.CmdCounter;

    /* Get the app ID */
    GPS_APP_Data.OutData.AppID_H = (uint8_t) ((GPS_APP_HK_TLM_MID >> 8) & 0xff);
    GPS_APP_Data.OutData.AppID_L = (uint8_t) (GPS_APP_HK_TLM_MID & 0xff);

    ++GPS_APP_Data.OutData.App_Pckg_Counter;

    /* Copy the GPS data */
    uint8_t *aux_array1;
    aux_array1 = NULL;
    aux_array1 = malloc(4 * sizeof(uint8_t));
    aux_array1 = (uint8_t*)(&GPS_APP_Data.latitude);

    uint8_t *aux_array2;
    aux_array2 = NULL;
    aux_array2 = malloc(4 * sizeof(uint8_t));
    aux_array2 = (uint8_t*)(&GPS_APP_Data.longitude);

    uint8_t *aux_array3;
    aux_array3 = NULL;
    aux_array3 = malloc(4 * sizeof(uint8_t));
    aux_array3 = (uint8_t*)(&GPS_APP_Data.altitude);

    uint8_t aux_byte = (uint8_t) GPS_APP_Data.satellites;
    uint8_t aux_array4[] = {aux_byte,0,0,0};

    for(int i=0;i<4;i++){
      GPS_APP_Data.OutData.byte_group_1[i] = aux_array1[i];
      GPS_APP_Data.OutData.byte_group_2[i] = aux_array2[i];
      GPS_APP_Data.OutData.byte_group_3[i] = aux_array3[i];
      GPS_APP_Data.OutData.byte_group_4[i] = aux_array4[i];
      GPS_APP_Data.OutData.byte_group_5[i] = 0;
      GPS_APP_Data.OutData.byte_group_6[i] = 0;
    }

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(GPS_APP_Data.OutData.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(GPS_APP_Data.OutData.TelemetryHeader), true);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 GPS_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg)
{
    /*
    ** Get command execution counters...
    */
    GPS_APP_Data.HkTlm.Payload.CommandErrorCounter = GPS_APP_Data.ErrCounter;
    GPS_APP_Data.HkTlm.Payload.CommandCounter      = GPS_APP_Data.CmdCounter;

    GPS_APP_Data.HkTlm.Payload.latitude = GPS_APP_Data.latitude;
    GPS_APP_Data.HkTlm.Payload.longitude = GPS_APP_Data.longitude;
    GPS_APP_Data.HkTlm.Payload.altitude = GPS_APP_Data.altitude;
    GPS_APP_Data.HkTlm.Payload.satellites = GPS_APP_Data.satellites;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(GPS_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(GPS_APP_Data.HkTlm.TelemetryHeader), true);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* GPS NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 GPS_APP_Noop(const GPS_APP_NoopCmd_t *Msg)
{
    GPS_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(GPS_APP_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION, "GPS: NOOP command %s",
                      GPS_APP_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 GPS_APP_ResetCounters(const GPS_APP_ResetCountersCmd_t *Msg)
{
    GPS_APP_Data.CmdCounter = 0;
    GPS_APP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(GPS_APP_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION, "GPS: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool GPS_APP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(GPS_APP_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        GPS_APP_Data.ErrCounter++;
    }

    return result;
}
