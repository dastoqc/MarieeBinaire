
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

/*#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>*/
#include "arduino-serial/arduino-serial-lib.h"

#include "mbdriver.h"

static struct termios old, newt;

// XML parsing
/*static void print_element_names(xmlDoc *doc, xmlNode * a_node, int checkid, int onid)
{
   xmlNode *cur_node = NULL;

   for (cur_node = a_node; cur_node; cur_node =
      cur_node->next) {
      if (cur_node->type == XML_ELEMENT_NODE) {
    if(!strcmp(cur_node->name, "id")){
        if(atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1))==checkid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
            onid=1;
        }
    }
    if(!strcmp(cur_node->name, "posCoin") && onid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
    }
    if(!strcmp(cur_node->name, "posUP") && onid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
    }
    if(!strcmp(cur_node->name, "posDOWN") && onid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
    }
    if(!strcmp(cur_node->name, "pwrEL") && onid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
    }
    if(!strcmp(cur_node->name, "pwrZ1") && onid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
    }
    if(!strcmp(cur_node->name, "pwrZ2") && onid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
    }
    if(!strcmp(cur_node->name, "pwrZ3") && onid){
                printf("%s %i\n",cur_node->name, atoi(xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1)));
    }
      }
      print_element_names(doc,cur_node->children, checkid, onid);
   }
}

int getnames(const char* filename, int checkid, int* val)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    LIBXML_TEST_VERSION    // Macro to check API for match with
                           // the DLL we are using

    //parse the file and get the DOM
    if ((doc = xmlReadFile(filename, NULL, 0)) == NULL){
       printf("error: could not parse file %s\n", filename);
       exit(-1);
       }

    //Get the root element node
    root_element = xmlDocGetRootElement(doc);
    print_element_names(doc,root_element,checkid,0);
    xmlFreeDoc(doc);       // free document
    xmlCleanupParser();    // Free globals
    return 0;
}*/

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

int main(int argc, char **argv)
{
  //LIBXML_TEST_VERSION
  MBDriver MBD("/dev/ttyACM0","/dev/ttyACM1"); //Device Arduino puis Maestro (première des 2)

  printf("Les flèches pour bouger les lèvres, 'a'/'z' pour les comissures des lèvres, 'p' pour shocker la langue et 'q' pour quitter. \n");
  char val=' ', zone=' ', *buf;
  int pwr=0, p=0;
  while(1){
    val=getch();
    switch(val){
        case '\033': //Flèches.
            getch();val=getch();
            switch(val){
            case 'A':
                printf("LUP-FW!! ");
                if(!DEBUG) MBD.mvtx(10,1);
                break;
            case 'B':
                printf("LUP-RW!! ");
                if(!DEBUG) MBD.mvtx(-10,1);
                break;
            case 'C':
                printf("LDOWN-FW!! ");
                if(!DEBUG) MBD.mvtx(10,0);
                break;
            case 'D':
                printf("LDOWN-RW!! ");
                if(!DEBUG) MBD.mvtx(-10,0);
                break;
            }
            break;
        case 'q':
            //xmlCleanupParser();
            return 0;
        case 'a':
            printf("LCOIN-FW!! ");
                    if(!DEBUG) int p = MBD.maestroGetPosition(MOTORCOIN);
            printf("Position %i\n",p);
                    if(!DEBUG) MBD.maestroSetTarget(MOTORCOIN, p+10);
            break;
        case 'z':
            printf("LCOIN-RW!! ");
                    if(!DEBUG) int p = MBD.maestroGetPosition(MOTORCOIN);
            printf("Position %i\n",p);
            if(p-10<4790)
                p=p+10;
                    if(!DEBUG) MBD.maestroSetTarget(MOTORCOIN, p-10);
            break;
            case 'p':
            printf("SHOCK, entrer la zone (a,b,c) et la puissance (0-100) :");
            if (scanf("%c%d", &zone, &pwr) == 2)
                    printf ("%c à %d\% \n", zone, pwr);
            else
                printf("Mauvaise entrée");
                    sprintf(buf, "%c%03d",zone,pwr);
                    if(!DEBUG){
                int rc = MBD.writeArduino(buf);
                usleep(20*1000);
                MBD.readArduino();
                rc = MBD.writeArduino("g000");
                usleep(500*1000);
                MBD.readArduino();
            }
                    break;
        case 'e':
            printf("ELECTROLARYNX, entrer la puissance (0-100) :");
            if (scanf("%d", &pwr) == 1)
                    printf ("%d\% \n", pwr);
            else
                printf("Mauvaise entrée");
                    sprintf(buf, "l%03d",pwr);
                    if(!DEBUG){
//			  	serialport_flush(fdArduino);
                int rc = MBD.writeArduino(buf);
                usleep(20*1000);
                MBD.readArduino();
            }
                    break;
        case 'm':
            printf("MENTON, entrer la vitesse (0-100) :");
            if (scanf("%d", &pwr) == 1)
                    printf ("%d\% \n", pwr);
            else
                printf("Mauvaise entrée");
                    if(pwr>0)
                sprintf(buf, "m%03d",pwr);
            else
                sprintf(buf, "n%03d",-pwr);
                    if(!DEBUG){
//			  	serialport_flush(fdArduino);
                int rc = MBD.writeArduino(buf);
                usleep(100*1000);
                MBD.readArduino();
            }
                    break;
/*        case 'f':
            printf("Phonème enregistrés (test.xml) :");
            if (scanf("%d", &zone) == 1){
                    //printf ("%d\% \n", pwr);
                int val[7];getnames("test.xml",zone,val);
                if(!DEBUG) rc = setphoneme(zone);
            } else
                printf("Mauvaise entrée");
                    break;*/
        default:  //renvoi la commande non interprétée
            printf("%c ",val);
    }
  }
  return 0;
}


/*setphoneme(int id)
{
  int val[7];
  getnames("test.xml",id,val);
  char buf[256];
  int posCoin=0, posUP=0, posDOWN=0, pwrEL=0, pwrZ1=0, pwrZ2=0, pwrZ3=0;
  maestroSetTarget(fdMaestro, MOTORCOIN, posCoin);
  mvtx(posUP, 1);
  mvtx(posDOWN, 0);
  sprintf(buf, "0 %i %i %i %i\n", pwrEL, pwrZ1, pwrZ2, pwrZ3);
  serialport_write(fdArduino, buf);
}*/
