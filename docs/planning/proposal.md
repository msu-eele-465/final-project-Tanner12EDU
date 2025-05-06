# Final project proposal

- [x ] I have reviewed the project guidelines.
- [x ] I will be working alone on this project.
- [x ] No significant portion of this project will be (or has been) used in other course work.

## Embedded System Description

The MSP2355 will serve as the master for the project and the slave because my MSP2310’s are shoddily soldered. The master will send data to two servos. 
The master will read the data from the UV sensor and display this data on a 7-segment display. The master will also read data from the RTC and two temperature sensors. 
This data will be interpreted, then sent to the MSP2355 slave. The slave will be used to drive the LCD. It will display the current date and time on the LCD, updating
every minute along with two ambient temperatures. It will also display the amount of times the servos have run per hour. The UV sensor is an analog input. 
It reads the UV index of the nearby light level. The UV index is scaled from 0-10 so displaying it on a 7-segment display will work well. The analog input will have to 
be converted to a usable value before it can be displayed on the 7-segment display. The RTC I will use is the DS3231. It has several modes of functionality. 
It requires the proper day, month, year, and time be initialized upon start up. Data will then be read from these registers after being properly initialized. 
I will use a 7-segment display to output the UV index. We have not worked with 7-segment displays this semester and it will be necessary to read the datasheet to 
get proper functionality. The LCD will be the same LCD that has been used in previous projects. It will need to have a new display structure. The two temperature sensors 
are to be used to monitor ambient room temperature and the soil of a potted plant. When these two temperatures are equivalent/or close to; the system will activate servos 
to tip more water into the plant. Every time the servos activate the master will send that data to be displayed on the LCD. This project could be used as an automated plant 
watering system with temperature and UV monitoring abilities along with a watering counter.

## Hardware Setup

For this project one MSP microcontroller is required. An MSP2355 with one I2C peripheral as a slave and another as master will be used. Additionally, three data 
acquisition units will be required for this project. A DS3231 precision RTC, UV index sensor and two temperature sensors. There will be three different outputs as well. 
A 7-segment display, two servos, and a LCD screen.

## Software overview

The MSP2355 will constantly be reading data from both the RTC, temperature sensors and UV index sensor. Whenever the 2355 detects a change in the data 
(it will store previous readings) it will send the new data to both the 7-segment display and the MSP2355 slave peripheral. The slave will then read this 
data and update the LCD accordingly.

## Testing Procedure

I can test the LCD and RTC pretty easily, its a straight up display. To test the temperature and servo control I can bring my water bottle full of ice.
I was hoping I can just demonstrate the servos working without actually needing them to tip water into a pot. The UV sensor only needs light 
and I can put my hand over it or some other thing to dim the light.


## Prescaler

Desired Prescaler level: 

- [ ] 100%
- [ ] 95% 
- [ ] 90% 
- [ ] 85% 
- [x ] 80% 
- [ ] 75% 

### Prescalar requirements 

**Outline how you meet the requirements for your desired prescalar level**

**The inputs to the system will be:**
1.  RTC 
2.  UV Sensor
3.  2 LM19 temperature sensors

**The outputs of the system will be:**
1.   2 Servos to tip water holder
2. 	 7 segment display to display UV level
3.	 LCD display of relevant data

**The project objective is**

This would be a "theoretical" house plant monitoring system. Using two temperature sensors to monitor soil temperature and room temperature along with 
the amount of light the plant is getting. When the two temperatures become close within a certain range, the servos will activate to hypothetically tip water
into the soil. The LCD display will show a clock and amount of watering along with both temperatures and the 7 segment display will show light level.

**The new hardware or software modules are:**
1. UV sensor to monitor the amount of light the house plant is receiving.
2. 


The Master will be responsible for:

The master will use the ADC to collect temperature averages from two different temperature sensors. Will control the 7 segment disply according to the UV sensor.
It will also control the servos and communicate with the RTC. Then it will send all necessary data to the slave.

The Slave(s) will be responsible for:

Receiving necessary data from the master and displaying two different temperatures, RTC, and number of servo activcations per day(or hour).



### Argument for Desired Prescaler

Although we have used a RTC during class before, we have not had to set accurate date or time values before. This will require understanding of the datasheet. 
Additionally, the UV sensor is a completely new component that has not been interfaced with for the class. This will also require familiarity with the data sheet.
 7-segment displays have not been introduced in this class but were in 371. The LCD will be used again but it will be displaying new data. 
 The servos are an addition to this class but have been used in the 371 course. The temperature sensors are to be used again but don’t add anything new.
