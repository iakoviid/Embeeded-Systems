# Embeeded-Systems
Implementation of a network system and report of test runs
on the Raspberrypi Zero W

# Contents
Report.pdf Report and analysis of the implementations

periodicMessaging.c  Periodic generation of messages

randomtime.c Random generation of messages 1sec<time<2sec

#Compiling

gcc periodicMessaging.c -pthread -o periodic 


#Executing

./periodic [Totaltime] [period] [devicelist]
./periodic 100 0.1 8943

