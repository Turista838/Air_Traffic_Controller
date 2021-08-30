# Air_Traffic_Controller

### Project developed for 'Operative Systems II', a second year subject @ISEC

#### Guide:

This project consists in several programs that, as a whole, simulate a management system for airports, passengers and planes.<br/>
Communication between the Controlador (Controller) and the Avião (Airplane) is made through shared memory, with certain information following the circular buffer communication paradigm, and others using direct access, always using synchronization mechanisms that are best adapted to each type of information. Regarding the communication between the Controlador and the Passageiro (Passenger) it involves named pipes in both directions.<br/><br/>
You need to lauch the arbitro.c first
