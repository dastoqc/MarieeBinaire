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

#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "arduino-serial/arduino-serial-lib.h"

#define MOTORHD 0
#define MOTORHG 1
#define MOTORBD 2
#define MOTORBG 3
#define MOTORCOIN 4

// Pour tester sans les board branchés en SUB mettre cette valeur à 1.
#define DEBUG 1

int fdMaestro = -1;
int fdArduino = -1;

static struct termios old, new;

// XML parsing
static void print_element_names(xmlDoc *doc, xmlNode * a_node, int checkid, int onid)
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

    /*parse the file and get the DOM */
    if ((doc = xmlReadFile(filename, NULL, 0)) == NULL){
       printf("error: could not parse file %s\n", filename);
       exit(-1);
       }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);
    print_element_names(doc,root_element,checkid,0);
    xmlFreeDoc(doc);       // free document
    xmlCleanupParser();    // Free globals
    return 0;
}

// Initialize new terminal i/o settings
void initTermios(int echo) 
{
  tcgetattr(0, &old); // grab old terminal i/o settings
  new = old; // make new settings same as old settings
  new.c_lflag &= ~ICANON; // disable buffered i/o
  new.c_lflag &= echo ? ECHO : ~ECHO; // set echo mode 
  tcsetattr(0, TCSANOW, &new); // use these new terminal i/o settings now
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

// Renvoi la position du servos 'channel' sur le port SUB 'fd'
int maestroGetPosition(int fd, unsigned char channel)
{
  unsigned char command[] = {0x90, channel};
  fd_set set;
  struct timeval timeout;

  if(write(fd, command, sizeof(command)) == -1)
  {
    perror("error writing");
    return -1;
  }

  unsigned char response[2];
  if(read(fd,response,2) != 2)
  {
    perror("error reading");
    return -1;
  }
  return response[0] + 256*response[1];
}

// Set la position du servo 'channel' à 'target' en quarter-microseconds.
int maestroSetTarget(int fd, unsigned char channel, unsigned short target)
{
  unsigned char command[] = {0x84, channel, target & 0x7F, target >> 7 & 0x7F};
  if (write(fd, command, sizeof(command)) == -1)
  {
    perror("error writing");
    return -1;
  }
  return 0;
}

int readArduino(int fd)
{
  unsigned char response[48];
  int n = read(fd,response,48);
  if(n < 0)
  {
    perror("error reading");
    return -1;
  }
//  response[n]='\0';
  printf("Message from Arduino: ");//%s\n", response);
  fwrite(response, n, 1, stdout);

  return 0;

}

// Movement de chaque paire de servo en simultané pour une translation des lèvres perpendiculaire au visage
// Max 8000, min 3968
void mvtx(int pas, int haut)
{
  int position = 0;
  if(haut){
    position = maestroGetPosition(fdMaestro, MOTORHD);
	printf("Position %i\n",position);
    maestroSetTarget(fdMaestro, MOTORHD, position+pas);
    position = maestroGetPosition(fdMaestro, MOTORHG);
    maestroSetTarget(fdMaestro, MOTORHG, position-pas);
  }else{
    position = maestroGetPosition(fdMaestro, MOTORBD);
    maestroSetTarget(fdMaestro, MOTORBD, position+pas);
    position = maestroGetPosition(fdMaestro, MOTORBG);
    maestroSetTarget(fdMaestro, MOTORBG, position-pas);
  }
}

int main(int argc, char **argv)
{
  const char * Dmaestro = "/dev/ttyACM1";  // Maestro Pololu Controller
  const char * Darduino = "/dev/ttyACM0";  // Arduino
  const int buf_max = 256;

  char serialport[buf_max];
  int baudrate = 115200;  // default
  char quiet=0;
  char eolchar = '\n';
  int timeout = 5000;
  char buf[buf_max];
  int rc,n, p0=0, p1=0, p2=0, p3=0, p4=0, p5=0;

  LIBXML_TEST_VERSION

  if(!DEBUG){
	fdMaestro = open(Dmaestro, O_RDWR | O_NOCTTY);
  	if (fdMaestro <= 0)
  	{
    	perror(Dmaestro);
    	return 1;
  	}

  	printf("Configuring Maestro serial com...");
  	struct termios options;
  	tcgetattr(fdMaestro, &options);
  	options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  	options.c_oflag &= ~(ONLCR | OCRNL);
  	tcsetattr(fdMaestro, TCSANOW, &options);
  	printf(" done.\n");

  	printf("Configuring Arduino serial com...");
  	fdArduino = serialport_init(Darduino, baudrate);
  	if(fdArduino <=0){
      		perror(Darduino);
      		return 1;
  	}
	/* Wait for the Arduino to reset */
// 	serialport_flush(fdArduino);
	usleep(2000*1000);
	readArduino(fdArduino);
  	printf(" done.\n");

  	p0 = maestroGetPosition(fdMaestro, 0);p1 = maestroGetPosition(fdMaestro, 1);p2 = maestroGetPosition(fdMaestro, 2);p3 = maestroGetPosition(fdMaestro, 3);p4 = maestroGetPosition(fdMaestro, 4);p5 = maestroGetPosition(fdMaestro, 5);
  	printf("Les positions des servos sont actuellement : %d,%d,%d,%d,%d,%d.\n", p0,p1,p2,p3,p4,p5); 
  }

  int target = 3970+(8000-3970)/2;
  if(argc>1)
	target=atoi(argv[1]);

  printf("Tous les servos sont remis à %d (%d us).\n", target, target/4);
  if(!DEBUG) maestroSetTarget(fdMaestro, MOTORHD, target);
  if(!DEBUG) maestroSetTarget(fdMaestro, MOTORHG, target);
  if(!DEBUG) maestroSetTarget(fdMaestro, MOTORBD, target);
  if(!DEBUG) maestroSetTarget(fdMaestro, MOTORBG, target);
  if(!DEBUG) maestroSetTarget(fdMaestro, MOTORCOIN, target);
  //maestroSetTarget(fdMaestro, 5, target);  //channel libre.

  printf("Les flèches pour bouger les lèvres, 'a'/'z' pour les comissures des lèvres, 'p' pour shocker la langue et 'q' pour quitter. \n");
  char val=' ', zone=' ';
  int pwr=0;
  while(1){
	val=getch();
	switch(val){
		case '\033': //Flèches.
			getch();val=getch();
			switch(val){
			case 'A':
				printf("LUP-FW!! ");
				if(!DEBUG) mvtx(10,1);
				break;
			case 'B':
				printf("LUP-RW!! ");
				if(!DEBUG) mvtx(-10,1);
				break;
			case 'C':
				printf("LDOWN-FW!! ");
				if(!DEBUG) mvtx(10,0);
				break;
			case 'D':
				printf("LDOWN-RW!! ");
				if(!DEBUG) mvtx(-10,0);
				break;
			}
			break;
		case 'q':
            		if(fdMaestro>0)
                		close(fdMaestro);
            		if(fdArduino>0)
                		serialport_close(fdArduino);
			xmlCleanupParser();
			return 0;
		case 'a':
			printf("LCOIN-FW!! ");
            		if(!DEBUG) p4 = maestroGetPosition(fdMaestro, MOTORCOIN);
			printf("Position %i\n",p4);
            		if(!DEBUG) maestroSetTarget(fdMaestro, MOTORCOIN, p4+10);
			break;
		case 'z':
			printf("LCOIN-RW!! ");
            		if(!DEBUG) p4 = maestroGetPosition(fdMaestro, MOTORCOIN);
			printf("Position %i\n",p4);
			if(p4-10<4790)
				p4=p4+10;
            		if(!DEBUG) maestroSetTarget(fdMaestro, MOTORCOIN, p4-10);
			break;
        	case 'p':
			printf("SHOCK, entrer la zone (a,b,c) et la puissance (0-100) :");
			if (scanf("%c%d", &zone, &pwr) == 2)
    				printf ("%c à %d\% \n", zone, pwr);
  			else
				printf("Mauvaise entrée");
            		sprintf(buf, "%c%03d",zone,pwr);
            		if(!DEBUG){
				rc = serialport_write(fdArduino, buf);
				usleep(20*1000);
				readArduino(fdArduino);
				rc = serialport_write(fdArduino, "g000");
				usleep(500*1000);
				readArduino(fdArduino);
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
				rc = serialport_write(fdArduino, buf);
				usleep(20*1000);
				readArduino(fdArduino);
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
				rc = serialport_write(fdArduino, buf);
				usleep(100*1000);
				readArduino(fdArduino);
			}
            		break;
		case 'f':
			printf("Phonème enregistrés (test.xml) :");
			if (scanf("%d", &zone) == 1){
    				//printf ("%d\% \n", pwr);
				int val[7];getnames("test.xml",zone,val);
	            		if(!DEBUG) rc = setphoneme(zone);
  			} else
				printf("Mauvaise entrée");
            		break;
		default:  //renvoi la commande non interprétée
			printf("%c ",val);
	}
  }
  return 0;
}


setphoneme(int id)
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
}
