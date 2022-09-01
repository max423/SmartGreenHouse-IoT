# SmartGreenHouse-IoT
Smart Greenhouse monitoring and management IOT system.

INTRO

The Smart Greenhouse is prototype implementation of a remotely controlled and monitored greenhouse. Using an IoT - based communication and control technology, the smart greenhouse enables remote control and monitoring of the environment.The goal of this project is to design and implement an IoT solution for autonomous managing a Greenhouse. 




COMMANDS

Database:
1) create DB : CREATE DATABASE SmartGreenHouse;
2) select DB :  USE smartgreenhouse;
3) import dump : SOURCE Dump_SmartGreenHouse.sql;

Mosquitto:
1) check status: systemctl status mosquitto 

Collector:
in the collector 
1) mvn clean install
2) mvn package
3) java -jar target/collector-1.0-SNAPSHOT.jar

Simulation:
1) contikier 
2) cd tools/cooja
3) ant run 

Border router:
1) tool -> serial socket server
2) cd border router folder
3) make TARGET=cooja connect-router-cooja

Grafana:
1) check status : sudo systemctl status grafana-server
2) dashboard : localhost:3000

Flash:
1) cd cartella sensore
2) make distclean
3) make TARGET=cc26x0-cc13x0 BOARD=/launchpad/cc2650 PORT=/dev/ttyACM0 NODEID=0x0001 coap-sensor.upload
3) make TARGET=cc26x0-cc13x0 BOARD=/launchpad/cc2650 PORT=/dev/ttyACM0 NODEID=0x0001 mqtt-sensor.upload
4) rpl] make TARGET=nrf52840 BOARD=dongle connect-router PORT=/dev/ttyACM0 





