cmake_minimum_required(VERSION 3.2)
project (monitor)
add_executable(monitor monitor.c)

# Find the json-c library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

target_link_directories(monitor PRIVATE /home/pi/paho.mqtt.c/build/output/lib  ../mylib)
target_include_directories(monitor PRIVATE ${JSONC_INCLUDE_DIRS}) # Add the include directories for json-c
target_link_libraries(monitor libpaho-mqtt3c.so libpaho-mqtt3a.so libpaho-mqtt3as.so libpaho-mqtt3cs.so libmylib.a ${JSONC_LIBRARIES})

install(TARGETS monitor DESTINATION /home/pi/MilanoWaterProject/bin)