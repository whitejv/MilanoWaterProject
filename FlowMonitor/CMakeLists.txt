cmake_minimum_required(VERSION 3.2)
project (flowmonitor)


# Find the json-c library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)


add_executable(flowmonitor flowmonitor.c)
target_link_directories(flowmonitor PRIVATE /home/pi/paho.mqtt.c/build/output/lib ../mylib)
target_include_directories(flowmonitor PRIVATE ${JSONC_INCLUDE_DIRS}) # Add the include directories for json-c
target_link_libraries(flowmonitor libpaho-mqtt3c.so libpaho-mqtt3a.so libpaho-mqtt3as.so libpaho-mqtt3cs.so libmylib.a ${JSONC_LIBRARIES})
install(TARGETS flowmonitor DESTINATION /home/pi/MilanoWaterProject/bin)