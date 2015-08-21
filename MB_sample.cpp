#include "mbdriver.h"

#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    MBDriver MBD;
    if(!DEBUG){
        int rc = MBD.opendevices("/dev/ttyACM0","/dev/ttyACM1"); //Device Arduino puis Maestro (première des 2)
        if(rc!=0){
            cout << "Failed to start devices" << endl;
            return -1;
        }
        // Pour bouguer les lèvres
        int pos = 4500;
        MBD.servoGoTo(MOTORBD,pos); //MOTORBD,MOTORBG,MOTORHD,MOTORHG,MOTORCOIN

        // Pour lancer l'électrolarynx
        int pwrL = 100;
        MBD.setLarynx(pwrL);

        // Pour bouger le menton (négatif déroule) .. plus de tests à faire pour les limites
        int pwrC = 50;
        MBD.chinGoTo(pwrC);

        // Pour lancer un shock
        char zone = 'a'; int pwr = 100;
        MBD.setLangue(zone,pwr); //a-pointe, b-centre, c-fond
        MBD.shock(0);

        // Pour tout faire d'une commande
        int posarray[5]={4000,4000,5000,3000,7000};
        int pwrLangue[3]={100,0,100};
        MBD.all(posarray,pwrL,pwrC,pwrLangue);
    }
}
