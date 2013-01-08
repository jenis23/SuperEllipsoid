Name: Jenis Modi

Email Address: jenis.modi@wsu.edu


Project Authors:

Jenis Modi

Dr. Wayne Cochran

The project creates Super Ellipsoid as defined in Project document. It contains 3 "C" files, 1 vertex shader file and 1 fragment shader file. I have kept M and N value as 40 and 24 respectively. Have also kept values m and n as 2 and 2. Please use textureEllipsoid to get final texture output.


1. wireFrame.c : Generate only wireFrame in rotation mode

2. shadedEllipsoid.c: Generate shaded object in rotation mode

3. textureEllipsoid.c: Generates Toroidal spiral with shade and light in rotation mode


Additional feature: Incrementing/decrementing bulge factors "m" and/or "n" value. Please use right click to increase/decrease "m" and increase/decrease "n" value. You can quit the project using either ESC or Menu option. You can also use Left/Right Arrow key to increase/decrease "m" value and Up/Down key to increase/decrease "n" value.


The project is coded in C Language. I have a make file to execute my code. You can just use “make” command to compile the code and then ./wireFrame OR ./shadedEllipsoid OR ./textureEllipsoid to run the code. The code is compiled with gnu99 option in the makefile. If this is not supported in user's machine, please change it to c99.

