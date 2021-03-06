
////////////////////////////////////////////////////////////////////////////////////
// Logiciel de contrôle du mécanisme pour l'installation "Mariée Binaire"
// Programmé par David St-Onge, 14-05-2015
// - Communication USB avec le board Maestro de Pololu pour le contrôle de 5 servos
// - Communication USB avec l'arduino UNO pour le contrôle des shocks sur la langue
// - Communication USB avec l'arduino pour le contrôle de l'électro-larynx
// NOTE: Le mode de communication série du board Mestro (pour les servos) doit être mis à "USB Dual Port" avec l'interface utilisateur (MaestroControlCenter).
// TODO:
//	- enregistrer et lire les configurations de mécanisme pour les phonèmes (cvs ou xml)
//	- contrôle d'un stepper (via arduino) pour la machoire
/////////////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <sstream>
#include <iostream>
#include <fstream>

/*#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>*/

#include "mbdriver.h"

static struct termios old, newt;

// Initialize new terminal i/o settings
void initTermios(int echo)
{
  tcgetattr(0, &old); // grab old terminal i/o settings
  newt = old; // make new settings same as old settings
  newt.c_lflag &= ~ICANON; // disable buffered i/o
  newt.c_lflag &= echo ? ECHO : ~ECHO; // set echo mode
  tcsetattr(0, TCSANOW, &newt); // use these new terminal i/o settings now
}

// Restore old terminal i/o settings
void resetTermios(void)
{
  tcsetattr(0, TCSANOW, &old);
}

// Fonctions pour la lecture de caractères entrés par l'utilisateur
char getch_(int echo)
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

char getch(void)
{
  return getch_(0);
}

void print_caps_lock_state(int state){
    printf("Caps Lock state: %s (%d)\n",state & K_CAPSLOCK == K_CAPSLOCK ? "on" : "off", state);
}


int main(int argc, char **argv)
{
  //LIBXML_TEST_VERSION
  MBDriver MBD;
  if(!DEBUG){
      int rc = MBD.opendevices("/dev/ttyACM0","/dev/ttyACM1"); //Device Arduino puis Maestro (première des 2)
      if(rc!=0){
          printf("Failed to start devices\n");
          return -1;
      }
  }
  else
      std::cout << "Not opening devices in DEBUG mode." << std::endl;


  printf("Les flèches pour bouger les lèvres (s pour passer de haut à bas), 'a'/'z' pour les comissures des lèvres, 'p' pour shocker la langue, 'e' pour le larynx, 'm' pour le menton et 'q' pour quitter. \n");
  char val=' ', zone=' ', *buf;
  int pwr=0, p=0, state=0;
  while(1){
    val=getch();
    switch(val){
        //case '\033': //Flèches.
        //    getch();val=getch();
        //    switch(val){
            case 'u':
                //printf("LUP-FW!! ");
                if(state)
                    MBD.servoIncr(10,MOTORHG);
                else
                    MBD.servoIncr(10,MOTORBG);
                break;
            case 'n':
                //printf("LUP-RW!! ");
                if(state)
                    MBD.servoIncr(-10,MOTORHG);
                else
                    MBD.servoIncr(-10,MOTORBG);
                break;
            case 'h':
                //printf("LDOWN-FW!! ");
                if(state)
                    MBD.servoIncr(10,MOTORHD);
                else
                    MBD.servoIncr(10,MOTORBD);
                break;
            case 'j':
                //printf("LDOWN-RW!! ");
                if(state)
                    MBD.servoIncr(-10,MOTORHD);
                else
                    MBD.servoIncr(-10,MOTORBD);
                break;
        //    }
        //    break;
        case 's':
            if(state)
                state=0;
            else
                state=1;
            break;
        case 'q':
            //xmlCleanupParser();
            return 0;
        case 'a':
            MBD.servoIncr(10,MOTORCOIN);
            break;
        case 'z':
            MBD.servoIncr(-10,MOTORCOIN);
            break;

        case 'p':
            printf("SHOCK, entrer la zone (a,b,c) et la puissance (0-100) :");
            if (scanf("%c%d", &zone, &pwr) == 2)
                    printf ("%c à %d\% \n", zone, pwr);
            else
                printf("Mauvaise entrée");
            if(!DEBUG){
                MBD.setLangue(zone,pwr);
                MBD.shock(20);
            }
            break;

        case 'e':
            printf("ELECTROLARYNX, entrer la puissance (0-100) :");
            if (scanf("%d", &pwr) == 1)
                    printf ("%d\% \n", pwr);
            else
                printf("Mauvaise entrée");
            MBD.setLarynx(pwr);
            break;

        case 'm':
            printf("MENTON, entrer la position (-100->100) :");
            if (scanf("%d", &pwr) == 1)
                    printf ("%d\% \n", pwr);
            else
                printf("Mauvaise entrée");
            MBD.chinGoTo(pwr);
            break;
        default:  //renvoi la commande non interprétée
            printf("%c ",val);
    }
  }
  return 0;
}
