# MarieeBinaire

Pour utiliser le driver lancer l'application linux MB_test (ou la version stdc MB).

Pour compiler une application avec le driver inclure l'entête mbdriver.h dans votre fichier principal et compiler mbdriver.cpp, arduino-serial.cpp avec votre éxécutable.

L'utilisation de la classe est simple :
//Création
MBDriver MBD;
//Connection aux 2 modules série
MBD.opendevices("/dev/ttyACM0","/dev/ttyACM1");
//Lecture/Écriture
MBD.maestroGetPosition(MOTORCOIN);
MBD.maestroSetTarget(MOTORCOIN, 100);
MBD.readArduino();
MBD.writeArduino();

En quittant le destructeur se charge de couper les connection séries correctement.

Bonne utilisation!
David St-Onge.
