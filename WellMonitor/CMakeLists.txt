cmake_minimum_required(VERSION 3.2)
project (wellmonitor)

# Find the json-c library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

add_executable(wellmonitor wellmonitor.c)
target_include_directories(wellmonitor PRIVATE ${JSONC_INCLUDE_DIRS}) # Add the include directories for json-c
target_link_directories(wellmonitor PRIVATE /home/pi/paho.mqtt.c/build/output/lib ../mylib)
target_link_libraries(wellmonitor libpaho-mqtt3c.so libpaho-mqtt3a.so libpaho-mqtt3as.so libpaho-mqtt3cs.so libmylib.a ${JSONC_LIBRARIES})
install(TARGETS wellmonitor DESTINATION /home/pi/MilanoWaterProject/bin)