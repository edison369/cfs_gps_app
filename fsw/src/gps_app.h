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
 * @file
 *
 * Main header file for the GPS application
 */

#ifndef GPS_APP_H
#define GPS_APP_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "gps_app_perfids.h"
#include "gps_app_msgids.h"
#include "gps_app_msg.h"

/***********************************************************************/

/*
** Include and constants for I2C
*/
#include "gen-uC.h"

static const char bus_path[] = "/dev/i2c-2";
static const char genuC_path[] = "/dev/i2c-2.genuC-0";

/***********************************************************************/
#define GPS_APP_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    /*
    ** Housekeeping telemetry packet...
    */
    GPS_APP_HkTlm_t HkTlm;
    GPS_APP_OutData_t OutData;

    /*
    ** GPS Data...
    */
    float latitude;
    float longitude;
    float altitude;
    uint8 satellites;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;
} GPS_APP_Data_t;

typedef union
{
float number;
uint8_t bytes[4];
} floatu_t;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (GPS_APP_Main), these
**       functions are not called from any other source module.
*/
void  GPS_APP_Main(void);
int32 GPS_APP_Init(void);
void  GPS_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void  GPS_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr);
int32 GPS_APP_ReadSensor(const CFE_MSG_CommandHeader_t *Msg);
int32 GPS_APP_ReportRFTelemetry(const CFE_MSG_CommandHeader_t *Msg);
int32 GPS_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg);
int32 GPS_APP_ResetCounters(const GPS_APP_ResetCountersCmd_t *Msg);
int32 GPS_APP_Noop(const GPS_APP_NoopCmd_t *Msg);

bool GPS_APP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

#endif /* GPS_APP_H */
