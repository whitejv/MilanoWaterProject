# Project Focus: MilanoWaterProject

## This project is an IOT project used to monitor my home water system. It uses sensors to collect raw data for pressure, temperature, flow and amperage. The sensors are implemented using the arduino ecosystem and an ESP 8266. The senors collect data and sent it to the server via MQTT. The server is a Raspberry Pi running LINUX. There are several apps that recieve the data and format it into more common units like gallons, farenhiet, etc. These apps start at bootup on the server. There is an interface to send data to influxdb for logging and graphing purposes.

### Sensors - Utilizing a ESP8266 board with Arduino Uno compatability and using the arduino ecosystem, the sensors collect data for each pump which includes well pumps and irrigation pumps.

### Sever - A LINUX based server implemented on raspberry pi computer is host to several apps that process sensor data and feed it to Blynk I/F and Telegraf via MQTT.

**Current Goal:** Project directory structure and information
**Upcoming Additions: **
**Key Components:**
├─ 📄 CMakeCache.txt
├─ 📄 CMakeLists.txt
├─ 📄 Focus.md
├─ 📄 MWP.pptx
├─ 📄 Makefile
├─ 📄 README.md
├─ 📄 WaterDiag.xlsm
├─ 📄 WaterDiag.xlsx
├─ 📄 WemosD1-Pinout.rtf
├─ 📄 Xsetup.sh
├─ 📄 cmake_install.cmake
├─ 📄 cronchk.sh
├─ 📄 crontab.file
├─ 📄 install_manifest.txt
├─ 📄 output.log
├─ 📄 setup.sh
├─ 📄 whenneededh2o.sh
├─ 📄 ~$MWP.pptx
├─ 📄 ~$WaterDiag.xlsm
├─ 📄 ~$WaterDiag.xlsx
├─ 📁 AlertMonitor
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 monitor
│  └─ 📄 monitor.c
├─ 📁 AsynchVer
│  ├─ 📄 main.cpp
│  └─ 📄 msgarrvdAsync.c
├─ 📁 Blynk
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 blynk
│  ├─ 📄 blynkWater
│  ├─ 📄 blynkWater.c
│  ├─ 📄 cmake_install.cmake
│  ├─ 📁 CMakeFiles
│  │  ├─ 📄 CMakeDirectoryInformation.cmake
│  │  ├─ 📄 progress.marks
│  │  └─ 📁 blynkWater.dir
│  │     ├─ 📄 C.includecache
│  │     ├─ 📄 DependInfo.cmake
│  │     ├─ 📄 blynkWater.c.o
│  │     ├─ 📄 build.make
│  │     ├─ 📄 cmake_clean.cmake
│  │     ├─ 📄 depend.internal
│  │     ├─ 📄 depend.make
│  │     ├─ 📄 flags.make
│  │     ├─ 📄 link.txt
│  │     └─ 📄 progress.make
│  └─ 📁 utility
│     ├─ 📄 BlynkDateTime.h
│     ├─ 📄 BlynkDebug.cpp
│     ├─ 📄 BlynkFifo.h
│     ├─ 📄 BlynkHandlers.cpp
│     ├─ 📄 BlynkTimer.cpp
│     ├─ 📄 BlynkUtility.h
│     └─ 📄 utility.cpp
├─ 📁 BlynkAlert
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 blynkAlert
│  ├─ 📄 blynkAlert.c
│  └─ 📁 utility
│     ├─ 📄 BlynkDateTime.h
│     ├─ 📄 BlynkDebug.cpp
│     ├─ 📄 BlynkFifo.h
│     ├─ 📄 BlynkHandlers.cpp
│     ├─ 📄 BlynkTimer.cpp
│     ├─ 📄 BlynkUtility.h
│     └─ 📄 utility.cpp
├─ 📁 CMakeFiles
│  ├─ 📄 CMakeDirectoryInformation.cmake
│  ├─ 📄 CMakeOutput.log
│  ├─ 📄 CMakeRuleHashes.txt
│  ├─ 📄 Makefile.cmake
│  ├─ 📄 Makefile2
│  ├─ 📄 TargetDirectories.txt
│  ├─ 📄 cmake.check_cache
│  ├─ 📄 progress.marks
│  ├─ 📁 3.18.4
│  │  ├─ 📄 CMakeCCompiler.cmake
│  │  ├─ 📄 CMakeCXXCompiler.cmake
│  │  ├─ 📄 CMakeDetermineCompilerABI_C.bin
│  │  ├─ 📄 CMakeDetermineCompilerABI_CXX.bin
│  │  ├─ 📄 CMakeSystem.cmake
│  │  ├─ 📁 CompilerIdC
│  │  │  ├─ 📄 CMakeCCompilerId.c
│  │  │  └─ 📄 a.out
│  │  └─ 📁 CompilerIdCXX
│  │     ├─ 📄 CMakeCXXCompilerId.cpp
│  │     └─ 📄 a.out
│  └─ 📁 clean-all.dir
│     ├─ 📄 DependInfo.cmake
│     ├─ 📄 build.make
│     ├─ 📄 cmake_clean.cmake
│     ├─ 📄 depend.make
│     └─ 📄 progress.make
├─ 📁 ESPClients
│  ├─ 📁 ESP32ClientWellX
│  │  ├─ 📄 ESP32ClientWell.ino.bin
│  │  ├─ 📄 ESP32ClientWell.ino.elf
│  │  ├─ 📄 ESP32ClientWell.ino.map
│  │  └─ 📄 ESP32ClientWellX.ino
│  ├─ 📁 GenericFlowX
│  │  └─ 📄 GenericFlowX.ino
│  ├─ 📁 GenericSensor
│  │  └─ 📄 GenericSensor.ino
│  ├─ 📁 GenericWellX
│  │  └─ 📄 GenericWellX.ino
│  ├─ 📁 NanoClientWell
│  │  └─ 📄 NanoClientWell.ino
│  └─ 📁 archive
│     ├─ 📄 ESPClient.ino
│     ├─ 📁 ESPClientTankGallons
│     │  └─ 📄 ESPClientTankGallons.ino
│     ├─ 📁 GenericFlowXX
│     │  └─ 📄 GenericFlowXX.ino
│     ├─ 📁 GenericFlowXYZ
│     │  └─ 📄 GenericFlowXYZ.ino
│     └─ 📁 GenericSensorTest
│        └─ 📄 GenericSensorTest.ino
├─ 📁 HouseMonitor
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 cmake_install.cmake
│  ├─ 📄 housemonitor
│  ├─ 📄 housemonitor.c
│  └─ 📁 CMakeFiles
│     ├─ 📄 CMakeDirectoryInformation.cmake
│     ├─ 📄 progress.marks
│     └─ 📁 housemonitor.dir
│        ├─ 📄 C.includecache
│        ├─ 📄 DependInfo.cmake
│        ├─ 📄 build.make
│        ├─ 📄 cmake_clean.cmake
│        ├─ 📄 depend.internal
│        ├─ 📄 depend.make
│        ├─ 📄 flags.make
│        ├─ 📄 housemonitor.c.o
│        ├─ 📄 link.txt
│        └─ 📄 progress.make
├─ 📁 IrrigationMonitor
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 cmake_install.cmake
│  ├─ 📄 irrigationmonitor
│  ├─ 📄 irrigationmonitor.c
│  ├─ 📄 irrigationmonitor.orig
│  └─ 📁 CMakeFiles
│     ├─ 📄 CMakeDirectoryInformation.cmake
│     ├─ 📄 progress.marks
│     └─ 📁 irrigationmonitor.dir
│        ├─ 📄 C.includecache
│        ├─ 📄 DependInfo.cmake
│        ├─ 📄 build.make
│        ├─ 📄 cmake_clean.cmake
│        ├─ 📄 depend.internal
│        ├─ 📄 depend.make
│        ├─ 📄 flags.make
│        ├─ 📄 irrigationmonitor.c.o
│        ├─ 📄 link.txt
│        └─ 📄 progress.make
├─ 📁 MilanoWaterProject
│  ├─ 📄 README.md
│  ├─ 📄 cronchk.sh
│  ├─ 📄 crontab.file
│  ├─ 📁 Blynk
│  │  ├─ 📄 Makefile
│  │  ├─ 📄 main.cpp
│  │  └─ 📁 utility
│  │     ├─ 📄 BlynkDateTime.h
│  │     ├─ 📄 BlynkDebug.cpp
│  │     ├─ 📄 BlynkFifo.h
│  │     ├─ 📄 BlynkHandlers.cpp
│  │     ├─ 📄 BlynkTimer.cpp
│  │     ├─ 📄 BlynkUtility.h
│  │     └─ 📄 utility.cpp
│  ├─ 📁 TankESPClient
│  │  └─ 📁 ESPClient
│  │     └─ 📄 ESPClient.ino
│  ├─ 📁 TankMonitor
│  │  ├─ 📄 Makefile
│  │  └─ 📄 monitor.c
│  ├─ 📁 TankSubscriber
│  │  ├─ 📄 Makefile
│  │  └─ 📄 subscriber.c
│  └─ 📁 include
│     └─ 📄 water.h
├─ 📁 MilanoWaterProject.xcodeproj
│  ├─ 📄 project.pbxproj
│  ├─ 📁 project.xcworkspace
│  │  ├─ 📄 contents.xcworkspacedata
│  │  └─ 📁 xcshareddata
│  │     ├─ 📄 IDEWorkspaceChecks.plist
│  │     └─ 📄 WorkspaceSettings.xcsettings
│  └─ 📁 xcshareddata
│     └─ 📁 xcschemes
│        ├─ 📄 FlowMonitor.xcscheme
│        ├─ 📄 MilanoWaterProject.xcscheme
│        ├─ 📄 TankAlert.xcscheme
│        ├─ 📄 TankMonitor.xcscheme
│        └─ 📄 TankSubscriber.xcscheme
├─ 📁 Monitor
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 cmake_install.cmake
│  ├─ 📄 monitor
│  ├─ 📄 monitor.c
│  └─ 📁 CMakeFiles
│     ├─ 📄 CMakeDirectoryInformation.cmake
│     ├─ 📄 progress.marks
│     └─ 📁 monitor.dir
│        ├─ 📄 C.includecache
│        ├─ 📄 DependInfo.cmake
│        ├─ 📄 build.make
│        ├─ 📄 cmake_clean.cmake
│        ├─ 📄 depend.internal
│        ├─ 📄 depend.make
│        ├─ 📄 flags.make
│        ├─ 📄 link.txt
│        ├─ 📄 monitor.c.o
│        └─ 📄 progress.make
├─ 📁 SensorTest
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 readme
│  ├─ 📄 sensortest
│  ├─ 📄 sensortest.c
│  ├─ 📄 test.txt
│  └─ 📁 testfiles
│     ├─ 📄 floats.txt
│     └─ 📄 tankgallons.txt
├─ 📁 SysAlert
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 alert
│  ├─ 📄 alert.c
│  ├─ 📄 alert.orig
│  ├─ 📄 cmake_install.cmake
│  └─ 📁 CMakeFiles
│     ├─ 📄 CMakeDirectoryInformation.cmake
│     ├─ 📄 progress.marks
│     └─ 📁 alert.dir
│        ├─ 📄 C.includecache
│        ├─ 📄 DependInfo.cmake
│        ├─ 📄 alert.c.o
│        ├─ 📄 build.make
│        ├─ 📄 cmake_clean.cmake
│        ├─ 📄 depend.internal
│        ├─ 📄 depend.make
│        ├─ 📄 flags.make
│        ├─ 📄 link.txt
│        └─ 📄 progress.make
├─ 📁 SysTest
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 cmake_install.cmake
│  ├─ 📄 readme
│  ├─ 📄 test
│  ├─ 📄 test.c
│  ├─ 📄 test.txt
│  ├─ 📄 well_test.txt
│  ├─ 📁 CMakeFiles
│  │  ├─ 📄 CMakeDirectoryInformation.cmake
│  │  ├─ 📄 progress.marks
│  │  └─ 📁 test.dir
│  │     ├─ 📄 C.includecache
│  │     ├─ 📄 DependInfo.cmake
│  │     ├─ 📄 build.make
│  │     ├─ 📄 cmake_clean.cmake
│  │     ├─ 📄 depend.internal
│  │     ├─ 📄 depend.make
│  │     ├─ 📄 flags.make
│  │     ├─ 📄 link.txt
│  │     ├─ 📄 progress.make
│  │     └─ 📄 test.c.o
│  └─ 📁 tests
│     ├─ 📄 floats.txt
│     └─ 📄 tankgallons.txt
├─ 📁 TankMonitor
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 Newtankmonitor.c
│  ├─ 📄 cmake_install.cmake
│  ├─ 📄 tankmonitor
│  ├─ 📄 tankmonitor.c
│  ├─ 📄 tankmonitor.old
│  └─ 📁 CMakeFiles
│     ├─ 📄 CMakeDirectoryInformation.cmake
│     ├─ 📄 progress.marks
│     └─ 📁 tankmonitor.dir
│        ├─ 📄 C.includecache
│        ├─ 📄 DependInfo.cmake
│        ├─ 📄 build.make
│        ├─ 📄 cmake_clean.cmake
│        ├─ 📄 depend.internal
│        ├─ 📄 depend.make
│        ├─ 📄 flags.make
│        ├─ 📄 link.txt
│        ├─ 📄 progress.make
│        └─ 📄 tankmonitor.c.o
├─ 📁 WellMonitor
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 Makefile.bak
│  ├─ 📄 cmake_install.cmake
│  ├─ 📄 wellmonitor
│  ├─ 📄 wellmonitor.c
│  └─ 📁 CMakeFiles
│     ├─ 📄 CMakeDirectoryInformation.cmake
│     ├─ 📄 progress.marks
│     └─ 📁 wellmonitor.dir
│        ├─ 📄 C.includecache
│        ├─ 📄 DependInfo.cmake
│        ├─ 📄 build.make
│        ├─ 📄 cmake_clean.cmake
│        ├─ 📄 depend.internal
│        ├─ 📄 depend.make
│        ├─ 📄 flags.make
│        ├─ 📄 link.txt
│        ├─ 📄 progress.make
│        └─ 📄 wellmonitor.c.o
├─ 📁 bin
│  ├─ 📄 CMakeCache.txt
│  ├─ 📄 alert
│  ├─ 📄 blynkAlert
│  ├─ 📄 blynkWater
│  ├─ 📄 housemonitor
│  ├─ 📄 irrigationmonitor
│  ├─ 📄 monitor
│  ├─ 📄 sensortest
│  ├─ 📄 tankmonitor
│  ├─ 📄 test
│  └─ 📄 wellmonitor
├─ 📁 include
│  ├─ 📄 alert.h
│  └─ 📄 water.h
├─ 📁 misc
│  └─ 📄 vsftpd.conf
├─ 📁 mylib
│  ├─ 📄 CMakeLists.txt
│  ├─ 📄 Makefile
│  ├─ 📄 cmake_install.cmake
│  ├─ 📄 flowmon.c
│  ├─ 📄 flowmon.orig
│  ├─ 📄 libmylib.a
│  ├─ 📄 logMsg.c
│  ├─ 📄 logTest.c
│  ├─ 📄 msgarrvd.c
│  └─ 📁 CMakeFiles
│     ├─ 📄 CMakeDirectoryInformation.cmake
│     ├─ 📄 progress.marks
│     └─ 📁 mylib.dir
│        ├─ 📄 C.includecache
│        ├─ 📄 DependInfo.cmake
│        ├─ 📄 build.make
│        ├─ 📄 cmake_clean.cmake
│        ├─ 📄 cmake_clean_target.cmake
│        ├─ 📄 depend.internal
│        ├─ 📄 depend.make
│        ├─ 📄 flags.make
│        ├─ 📄 flowmon.c.o
│        ├─ 📄 link.txt
│        ├─ 📄 logMsg.c.o
│        ├─ 📄 logTest.c.o
│        └─ 📄 progress.make
└─ 📁 pyrainbird
   └─ 📄 RainbirdSync.py

**Project Context:**
Type: File and directory tracking
Target Users: Users of MilanoWaterProject
Main Functionality: Project directory structure and information
Key Requirements:
- Type: Generic Project
- File and directory tracking
- Automatic updates

**Development Guidelines:**
- Keep code modular and reusable
- Follow best practices for the project type
- Maintain clean separation of concerns

# File Analysis
`mylib/flowmon.c` (92 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<resetFlowData>: This function helps with the program's functionality
<updateFlowRate>: Updates flow rate | Takes float flow rate g p m
<calculateAverageFlowRate>: Calculates average flow rate | Ensures count > 0 | Returns (count > 0) ? sum / count : 0
<flowmon>: Takes int new data flag and int milliseconds and int pulse count and float *p avgflow rate g p m and float *pinterval flow and float calibration factor | Ensures milliseconds < 5000

`mylib/logTest.c` (82 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<log_test>: Takes int verbose and int log level and int msg level and const char* format and ...

`mylib/logMsg.c` (32 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<log_message>: Takes const char *format and ...

`mylib/msgarrvd.c` (147 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<msgarrvd>: Takes void *context and char *topic name and int topic len and  m q t t client message *message
<parse_message>: Takes char* message and  controller* controller

`include/water.h` (1344 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<prototypes>: Takes const char *format and ...
**🚨 Critical-Length Alert: File is more than 200% of recommended length (1344 lines vs. recommended 300)**

`include/alert.h` (54 lines)
**Main Responsibilities:** Project file

`BlynkAlert/utility/BlynkUtility.h` (87 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkMin>: Takes const  t& a and const  t& b | Returns (b < a) ? b : a
<BlynkMax>: Takes const  t& a and const  t& b | Returns (b < a) ? a : b
<BlynkMathMap>: Takes  t x and  t2 in min and  t2 in max and  t2 out min and  t2 out max
<BlynkMathClamp>: Takes  t val and  t2 low and  t2 high | Returns (val < low) ? low : ((val > high) ? high : val)
<BlynkMathClampMap>: Takes  t x and  t2 in min and  t2 in max and  t2 out min and  t2 out max
<BlynkAverageSample>: Takes  t& avg and const  t& input | Ensures add > 0
<BlynkCRC32>: Takes const void* data and size t length and uint32 t previous crc32 | Ensures j < 8 | Returns ~crc
<c>: Takes counter

`BlynkAlert/utility/BlynkFifo.h` (159 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkFifo>: This function helps with the program's functionality
<clear>: This function helps with the program's functionality
<writeable>: Takes void | Returns free() > 0
<free>: Takes void | Ensures s <= 0 | Returns s - 1
<put>: Takes const  t& c | Returns c
<put>: This function validates not blocking and c < f | Takes const  t* p and int n and bool blocking | Returns n - c
<readable>: Takes void | Returns (_r != _w)
<size>: Takes void | Ensures s < 0 | Returns s
<get>: Retrieves  data | Takes void | Returns t
<peek>: Takes void | Returns _b[r]
<get>: Retrieves  data | This function validates f and not blocking | Takes  t* p and int n and bool blocking | Returns n - c
<_inc>: Takes int i and int n | Returns (i + n) % N

`BlynkAlert/utility/BlynkDebug.cpp` (330 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: This function helps with the program's functionality
<BlynkFreeRam>: Returns ESP.getFreeHeap()
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: Returns &stack_dummy - sbrk(0)
<BlynkReset>: This function helps with the program's functionality
<BlynkMillis>: Returns t
<BlynkReset>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<blynk_wake>: This function helps with the program's functionality
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: Returns blynk_millis_timer.read_ms()
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: Returns Clock_getTicks()
<BlynkDelay>: Takes millis time t ms | Returns delay(ms)
<BlynkMillis>: Returns millis()
<BlynkFreeRam>: Returns 0
<BlynkReset>: This function helps with the program's functionality
<BlynkFatal>: This function helps with the program's functionality
**📄 Length Alert: File exceeds recommended length (330 lines vs. recommended 300)**

`BlynkAlert/utility/BlynkHandlers.cpp` (407 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkNoOpCbk>: This function helps with the program's functionality
<BlynkWidgetRead>: Takes  blynk req  b l y n k  u n u s e d &request
<BlynkWidgetWrite>: Takes  blynk req  b l y n k  u n u s e d &request and const  blynk param  b l y n k  u n u s e d &param
<GetReadHandler>: Retrieves read handler data | Takes uint8 t pin | Returns NULL
<GetWriteHandler>: Retrieves write handler data | Takes uint8 t pin | Returns NULL
**📄 Length Alert: File exceeds recommended length (407 lines vs. recommended 300)**

`BlynkAlert/utility/BlynkDateTime.h` (178 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<invalid>: Returns BlynkTime()
<mTime>: Takes -1 **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<mTime>: Takes t.m time **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<mTime>: Takes seconds %  m a x  t i m e **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<BlynkTime>: Takes int hour and int minute and int second
<adjustSeconds>: This function validates isValid( | Takes int sec **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 65)**
<mTime>: Takes 0 **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<BlynkDateTime>: Takes const  blynk date time& t
<BlynkDateTime>: Takes blynk time t t
<BlynkDateTime>: Takes int hour and int minute and int second and int day and int month and int year
<adjustSeconds>: This function validates isValid( | Takes int sec **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 65)**
<getTm>: Retrieves tm data | Returns mTm

`BlynkAlert/utility/utility.cpp` (214 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<dtostrf_internal>: Takes double number and signed char  b l y n k  u n u s e d width and unsigned char prec and char *s | Ensures number > 4294967040 and number < 0 and prec > 0 | Returns s
<atoll_internal>: Takes const char *instr | Returns retval
<blynk_gmtime_r>: Takes const blynk time t* t and struct blynk tm *tm | Returns tm
<blynk_mk_gmtime>: Takes struct blynk tm *tm | Ensures tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0

`BlynkAlert/utility/BlynkTimer.cpp` (302 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<parameters>: Returns micros()
<elapsed>: Returns micros() **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 34)**
<elapsed>: Returns BlynkMillis() **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 34)**
<numTimers>: Takes -1
<contributed>: Takes unsigned num timer
**📄 Length Alert: File exceeds recommended length (302 lines vs. recommended 300)**

`MilanoWaterProject/include/water.h` (130 lines)
**Main Responsibilities:** Project file

`MilanoWaterProject/Blynk/utility/BlynkUtility.h` (87 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkMin>: Takes const  t& a and const  t& b | Returns (b < a) ? b : a
<BlynkMax>: Takes const  t& a and const  t& b | Returns (b < a) ? a : b
<BlynkMathMap>: Takes  t x and  t2 in min and  t2 in max and  t2 out min and  t2 out max
<BlynkMathClamp>: Takes  t val and  t2 low and  t2 high | Returns (val < low) ? low : ((val > high) ? high : val)
<BlynkMathClampMap>: Takes  t x and  t2 in min and  t2 in max and  t2 out min and  t2 out max
<BlynkAverageSample>: Takes  t& avg and const  t& input | Ensures add > 0
<BlynkCRC32>: Takes const void* data and size t length and uint32 t previous crc32 | Ensures j < 8 | Returns ~crc
<c>: Takes counter

`MilanoWaterProject/Blynk/utility/BlynkFifo.h` (159 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkFifo>: This function helps with the program's functionality
<clear>: This function helps with the program's functionality
<writeable>: Takes void | Returns free() > 0
<free>: Takes void | Ensures s <= 0 | Returns s - 1
<put>: Takes const  t& c | Returns c
<put>: This function validates not blocking and c < f | Takes const  t* p and int n and bool blocking | Returns n - c
<readable>: Takes void | Returns (_r != _w)
<size>: Takes void | Ensures s < 0 | Returns s
<get>: Retrieves  data | Takes void | Returns t
<peek>: Takes void | Returns _b[r]
<get>: Retrieves  data | This function validates f and not blocking | Takes  t* p and int n and bool blocking | Returns n - c
<_inc>: Takes int i and int n | Returns (i + n) % N

`MilanoWaterProject/Blynk/utility/BlynkDebug.cpp` (330 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: This function helps with the program's functionality
<BlynkFreeRam>: Returns ESP.getFreeHeap()
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: Returns &stack_dummy - sbrk(0)
<BlynkReset>: This function helps with the program's functionality
<BlynkMillis>: Returns t
<BlynkReset>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<blynk_wake>: This function helps with the program's functionality
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: Returns blynk_millis_timer.read_ms()
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: Returns Clock_getTicks()
<BlynkDelay>: Takes millis time t ms | Returns delay(ms)
<BlynkMillis>: Returns millis()
<BlynkFreeRam>: Returns 0
<BlynkReset>: This function helps with the program's functionality
<BlynkFatal>: This function helps with the program's functionality
**📄 Length Alert: File exceeds recommended length (330 lines vs. recommended 300)**

`MilanoWaterProject/Blynk/utility/BlynkHandlers.cpp` (407 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkNoOpCbk>: This function helps with the program's functionality
<BlynkWidgetRead>: Takes  blynk req  b l y n k  u n u s e d &request
<BlynkWidgetWrite>: Takes  blynk req  b l y n k  u n u s e d &request and const  blynk param  b l y n k  u n u s e d &param
<GetReadHandler>: Retrieves read handler data | Takes uint8 t pin | Returns NULL
<GetWriteHandler>: Retrieves write handler data | Takes uint8 t pin | Returns NULL
**📄 Length Alert: File exceeds recommended length (407 lines vs. recommended 300)**

`MilanoWaterProject/Blynk/utility/BlynkDateTime.h` (178 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<invalid>: Returns BlynkTime()
<mTime>: Takes -1 **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<mTime>: Takes t.m time **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<mTime>: Takes seconds %  m a x  t i m e **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<BlynkTime>: Takes int hour and int minute and int second
<adjustSeconds>: This function validates isValid( | Takes int sec **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 65)**
<mTime>: Takes 0 **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<BlynkDateTime>: Takes const  blynk date time& t
<BlynkDateTime>: Takes blynk time t t
<BlynkDateTime>: Takes int hour and int minute and int second and int day and int month and int year
<adjustSeconds>: This function validates isValid( | Takes int sec **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 65)**
<getTm>: Retrieves tm data | Returns mTm

`MilanoWaterProject/Blynk/utility/utility.cpp` (214 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<dtostrf_internal>: Takes double number and signed char  b l y n k  u n u s e d width and unsigned char prec and char *s | Ensures number > 4294967040 and number < 0 and prec > 0 | Returns s
<atoll_internal>: Takes const char *instr | Returns retval
<blynk_gmtime_r>: Takes const blynk time t* t and struct blynk tm *tm | Returns tm
<blynk_mk_gmtime>: Takes struct blynk tm *tm | Ensures tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0

`MilanoWaterProject/Blynk/utility/BlynkTimer.cpp` (292 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<parameters>: Returns micros()
<elapsed>: Returns micros() **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 34)**
<elapsed>: Returns BlynkMillis() **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 34)**
<numTimers>: Takes -1
<contributed>: Takes unsigned num timer

`AsynchVer/msgarrvdAsync.c` (73 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<msgarrvdAsync>: Takes void *context and char *topic name and int topic len and  m q t t async message *message

`CMakeFiles/3.18.4/CompilerIdC/CMakeCCompilerId.c` (675 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<main>: This function helps with the program's functionality
**🚨 Critical-Length Alert: File is more than 200% of recommended length (675 lines vs. recommended 300)**

`Blynk/utility/BlynkUtility.h` (87 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkMin>: Takes const  t& a and const  t& b | Returns (b < a) ? b : a
<BlynkMax>: Takes const  t& a and const  t& b | Returns (b < a) ? a : b
<BlynkMathMap>: Takes  t x and  t2 in min and  t2 in max and  t2 out min and  t2 out max
<BlynkMathClamp>: Takes  t val and  t2 low and  t2 high | Returns (val < low) ? low : ((val > high) ? high : val)
<BlynkMathClampMap>: Takes  t x and  t2 in min and  t2 in max and  t2 out min and  t2 out max
<BlynkAverageSample>: Takes  t& avg and const  t& input | Ensures add > 0
<BlynkCRC32>: Takes const void* data and size t length and uint32 t previous crc32 | Ensures j < 8 | Returns ~crc
<c>: Takes counter

`Blynk/utility/BlynkFifo.h` (159 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkFifo>: This function helps with the program's functionality
<clear>: This function helps with the program's functionality
<writeable>: Takes void | Returns free() > 0
<free>: Takes void | Ensures s <= 0 | Returns s - 1
<put>: Takes const  t& c | Returns c
<put>: This function validates not blocking and c < f | Takes const  t* p and int n and bool blocking | Returns n - c
<readable>: Takes void | Returns (_r != _w)
<size>: Takes void | Ensures s < 0 | Returns s
<get>: Retrieves  data | Takes void | Returns t
<peek>: Takes void | Returns _b[r]
<get>: Retrieves  data | This function validates f and not blocking | Takes  t* p and int n and bool blocking | Returns n - c
<_inc>: Takes int i and int n | Returns (i + n) % N

`Blynk/utility/BlynkDebug.cpp` (330 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: This function helps with the program's functionality
<BlynkFreeRam>: Returns ESP.getFreeHeap()
<BlynkReset>: This function helps with the program's functionality
<BlynkFreeRam>: Returns &stack_dummy - sbrk(0)
<BlynkReset>: This function helps with the program's functionality
<BlynkMillis>: Returns t
<BlynkReset>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<blynk_wake>: This function helps with the program's functionality
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: Returns blynk_millis_timer.read_ms()
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkSystemInit>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: This function helps with the program's functionality
<BlynkReset>: This function helps with the program's functionality
<BlynkDelay>: Takes millis time t ms
<BlynkMillis>: Returns Clock_getTicks()
<BlynkDelay>: Takes millis time t ms | Returns delay(ms)
<BlynkMillis>: Returns millis()
<BlynkFreeRam>: Returns 0
<BlynkReset>: This function helps with the program's functionality
<BlynkFatal>: This function helps with the program's functionality
**📄 Length Alert: File exceeds recommended length (330 lines vs. recommended 300)**

`Blynk/utility/BlynkHandlers.cpp` (407 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<BlynkNoOpCbk>: This function helps with the program's functionality
<BlynkWidgetRead>: Takes  blynk req  b l y n k  u n u s e d &request
<BlynkWidgetWrite>: Takes  blynk req  b l y n k  u n u s e d &request and const  blynk param  b l y n k  u n u s e d &param
<GetReadHandler>: Retrieves read handler data | Takes uint8 t pin | Returns NULL
<GetWriteHandler>: Retrieves write handler data | Takes uint8 t pin | Returns NULL
**📄 Length Alert: File exceeds recommended length (407 lines vs. recommended 300)**

`Blynk/utility/BlynkDateTime.h` (178 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<invalid>: Returns BlynkTime()
<mTime>: Takes -1 **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<mTime>: Takes t.m time **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<mTime>: Takes seconds %  m a x  t i m e **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<BlynkTime>: Takes int hour and int minute and int second
<adjustSeconds>: This function validates isValid( | Takes int sec **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 65)**
<mTime>: Takes 0 **🔄 Duplicate Alert: Function appears 4 times (first occurrence: line 38)**
<BlynkDateTime>: Takes const  blynk date time& t
<BlynkDateTime>: Takes blynk time t t
<BlynkDateTime>: Takes int hour and int minute and int second and int day and int month and int year
<adjustSeconds>: This function validates isValid( | Takes int sec **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 65)**
<getTm>: Retrieves tm data | Returns mTm

`Blynk/utility/utility.cpp` (214 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<dtostrf_internal>: Takes double number and signed char  b l y n k  u n u s e d width and unsigned char prec and char *s | Ensures number > 4294967040 and number < 0 and prec > 0 | Returns s
<atoll_internal>: Takes const char *instr | Returns retval
<blynk_gmtime_r>: Takes const blynk time t* t and struct blynk tm *tm | Returns tm
<blynk_mk_gmtime>: Takes struct blynk tm *tm | Ensures tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0 and tm_sec < 0

`Blynk/utility/BlynkTimer.cpp` (302 lines)
**Main Responsibilities:** Project file
**Key Functions:**
<parameters>: Returns micros()
<elapsed>: Returns micros() **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 34)**
<elapsed>: Returns BlynkMillis() **🔄 Duplicate Alert: Function appears 2 times (first occurrence: line 34)**
<numTimers>: Takes -1
<contributed>: Takes unsigned num timer
**📄 Length Alert: File exceeds recommended length (302 lines vs. recommended 300)**

# Project Metrics Summary
Total Files: 273
Total Lines: 7,650

**Files by Type:**
- .c: 6 files (1,101 lines)
- .cpp: 12 files (3,749 lines)
- .h: 12 files (2,800 lines)

**Code Quality Alerts:**
- 🚨 Severe Length Issues: 2 files
- ⚠️ Critical Length Issues: 0 files
- 📄 Length Warnings: 8 files
- 🔄 Duplicate Functions: 24

Last updated: January 03, 2025 at 06:04 PM