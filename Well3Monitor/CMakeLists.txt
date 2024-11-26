cmake_minimum_required(VERSION 3.2)
project (well3monitor)


# Find the json-c library
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)


add_executable(well3monitor well3monitor.c)
target_link_directories(well3monitor PRIVATE /home/pi/paho.mqtt.c/build/output/lib ../mylib)
target_include_directories(well3monitor PRIVATE ${JSONC_INCLUDE_DIRS}) # Add the include directories for json-c
target_link_libraries(well3monitor libpaho-mqtt3c.so libpaho-mqtt3a.so libpaho-mqtt3as.so libpaho-mqtt3cs.so libmylib.a ${JSONC_LIBRARIES})
install(TARGETS well3monitor DESTINATION /home/pi/MilanoWaterProject/bin)