# Air_Traffic_Controller

### Project developed for 'Operative Systems II', a second year subject @ISEC

#### Guide:

This project consists in several programs that, as a whole, simulate a management system for airports, passengers and planes.<br/>
Communication between the Controller (control.c) and the Airplane (aviao.c) is made through shared memory, with certain information following the circular buffer communication paradigm, and others using direct access, always using synchronization mechanisms that are best adapted to each type of information. Regarding the communication between the Controller and the Passenger (passag.c) it involves named pipes in both directions.<br/>
Controller is built with a GUI (Win32), whereas the Airplane and Passenger are console only.
<br/>
#### control.c
You need to launch the control.c first, otherwise the other programs will terminate automatically. When launched, it will ask you for the user to input the maximum number of airports that can be created and the maximum number of airplanes to be accepted by the Controller. After that you can navigate in the menu bar to add a new airport, view all planes connected, view all passengers connected, view all airports created, and suspend temporarily the acceptance of new incoming planes (new instances of aviao.c). Only one instance of control.c can be active. You can only create airports between 0 and 1000 coordinates (x,y).
<br/>
#### aviao.c
Before launching aviao.c, you need to set the following arguments:
1. Airplane capacity
2. Velocity (Pixels per second)
3. Name of the starting airport (It must be already created in control.c)<br/><br/>
Example: aviao.c 10 4 Coimbra. Otherwise it will not launch.<br/><br/>
* Commands:<br/>
```exit``` -> exits the system (all clients are notified)<br/>
```players``` -> lists all players (clients) connected
<br/>
#### passag.c
As for passag.c, you need to set the following arguments:
1. Name of the departure airport
2. Name of the arrival airport
3. Name of the Passenger
4. How many time this instance will wait for a plane to be available, in seconds. (Optional)<br/><br/>
Example: passag.c Porto Lisboa Gonçalo 540. Otherwise it will not launch. Passenger program does not support commands.<br/><br/>

