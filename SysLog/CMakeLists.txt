cmake_minimum_required(VERSION 3.2)
project (log)

find_package(CURL REQUIRED)
# Find the json-c library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

add_executable(log log.c)

target_include_directories(log PRIVATE /usr/local/include/influxdb)
target_link_directories(log PRIVATE /usr/local/lib)
target_link_directories(log PRIVATE /home/pi/paho.mqtt.c/build/output/lib  ../mylib)
target_include_directories(log PRIVATE ${CURL_INCLUDE_DIRS} ${JSONC_INCLUDE_DIRS}) # Add the include directories for json-c
target_link_libraries(log libpaho-mqtt3c.so libpaho-mqtt3a.so libpaho-mqtt3as.so libpaho-mqtt3cs.so libinfluxdb.so libmylib.a ${CURL_LIBRARIES} ${JSONC_LIBRARIES})
install(TARGETS log DESTINATION /home/pi/MilanoWaterProject/bin)