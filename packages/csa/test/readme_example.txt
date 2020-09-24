Contamination Source Identification example
==================================================


This example uses the CSA algorithm to identify the contamination source candidates. The example network is the Epanet Net3 network. Description of the example code is in section 3.1 of the CSA user's manual (Ver. 1).
The included files are:

net_true.inp   --  the EPANET network input file.
sensoridNet3   --  the text file containing the ID for all the sensor locations.
sm.txt   --  all the measurements from all sensors for the entire simulation duration.
candidates.txt   --  the CSA candidates output file: it contains the number of node-time pairs identified as unsafe and safe candidates at each analysis time.
Smatrix.txt -- the Contamination Source Matrix output file: it contains all the Contamination Source matrices S computed during all the simulation duration;
			each matrix corresponds to a particular analysis time that is printed before each matrix. The size of the matrix increases until the number of columns equals the histrorical analysis time frame 
csatest.cpp  --  example code

To create your project be sure to include in the same folder the following files:
csa.h, CSA.dll, CSA.lib, epanetbtx.h, EPANETBTX.dll, EPANETBTX.lib, epanet2.h. epanet2.dll, and epanet2.lib


Note:
Please note that the example code 'csatest.cpp' use only 3 sensor locations (the first 3 sensors in the list of the file 'sensoridNet3').
The file 'sm.txt' contains a matrix of 240x3 (3 sensors and 240 sampling times, from sampling time 1h to time 240h ).
The output file 'candidates.txt' lists for each analysis time (first column, from 0 to 240) the number of unsafe candidates (second column) and the number of safe candidates (third column).




These instructions are for the Microsoft Visual C/C++
2005.


1. Create a new project in Visual Studio 2005 as Win32 Consol Application.

2. Choose the option 'empty project'.

3. in the directory of your project copy these files:
csa.h, CSA.dll, CSA.lib, epanetbtx.h, EPANETBTX.dll, EPANETBTX.lib, epanet2.h. epanet2.dll, epanet2.lib, csatest.cpp,net_true.inp, sensoridNet3, and sm.txt.


4. add 'csatest.cpp' as 'existing item' to the Source file directory of your project.


5.open the Configuration Properties> C/C++>General and include additional directories (as the one where you kept your files).
 
6. open the Configuration Properties> Linker
- add the path to the additional libraries directory 
- add the additional dependencies: epanet2.lib epanetbtx.lib CSA.lib
7. From the Build menu, you can proceed  to build and to link 
