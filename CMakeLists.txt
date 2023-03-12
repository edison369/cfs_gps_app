project(CFE_GPS_APP C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(${gps_app_MISSION_DIR}/fsw/platform_inc)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(gps_app ${APP_SRC_FILES})
