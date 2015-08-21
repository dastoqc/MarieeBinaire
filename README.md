# MarieeBinaire

Pour utiliser le driver lancer l'application linux MB_test (ou la version stdc MB).

Pour compiler une application avec le driver inclure l'entête mbdriver.h dans votre fichier principal et compiler mbdriver.cpp, arduino-serial.cpp avec votre éxécutable. Il est aussi possible de compiler le driver en librairie statique (voir la Makefile) et l'inclure à votre projet avec -lmbdriver .

L'utilisation de la classe est simple :

--Création

MBDriver MBD;

--Connection aux 2 modules série

MBD.opendevices("/dev/ttyACM0","/dev/ttyACM1");

Les commandes pour lancer les moteurs/shocks sont décrites dans le fichier exemple MB_sample.cpp .

En quittant le destructeur se charge de couper les connection séries correctement.

Bonne utilisation!
David St-Onge.
