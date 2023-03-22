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
 * Define GPS App Message IDs
 *
 * \note The GPS App assumes default configuration which uses V1 of message id implementation
 */

#ifndef GPS_APP_MSGIDS_H
#define GPS_APP_MSGIDS_H

/* V1 Command Message IDs must be 0x18xx */
#define GPS_APP_CMD_MID     0x18C0
#define GPS_APP_SEND_HK_MID 0x18C1
#define GPS_APP_SEND_RF_MID 0x18C2
#define GPS_APP_READ_MID 	 0x18C3
/* V1 Telemetry Message IDs must be 0x08xx */
#define GPS_APP_HK_TLM_MID 0x08C1
#define GPS_APP_RF_DATA_MID 0x08C2

#endif /* GPS_APP_MSGIDS_H */
