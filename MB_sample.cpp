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
        int pos = 2; // numéro de la position également distribuée entre min et max selon le nbr maximum de position possible configuré dans config.txt
        int section = 1; // section des lèvres: 0-haut, 1-bas, 2-coins
        MBD.mvtx(pos,section);

        // Pour lancer l'électrolarynx
        int pwrL = 100;
        MBD.setLarynx(pwrL);

        // Pour bouger le menton (négatif déroule)
        int posC = 220;
        int speed = 50;
        MBD.chinGoTo(posC,speed);

        // Pour lancer un shock
        char zone = 'a'; int pwr = 100;
        MBD.setLangue(zone,pwr); //a-pointe, b-centre, c-fond
        int t = 20; // temps de shock en millis
        MBD.shock(20);

        // Pour tout faire d'une commande
        int posarray[5]={4000,4000,5000,3000,7000}; //positions de chaque servo des lèvres
        int pwrLangue[3]={100,0,100};
        MBD.all(posarray,pwrL,posC,pwrLangue);
    }
}
