#include "mbdriver.h"

#include "arduino-serial/arduino-serial-lib.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/select.h>
#include <termios.h>

MBDriver::MBDriver()
{
    fdMaestro = -1;
    fdArduino = -1;
    Dmaestro = "/dev/ttyACM1";  // Maestro Pololu Controller
    Darduino = "/dev/ttyACM0";  // Arduino
    quiet=0;
    eolchar = '\n';
    timeout = 5000;

    readconfig();
    if(!DEBUG) opendevices();
}

MBDriver::MBDriver(char* arduino, char* maestro)
{
    fdMaestro = -1;
    fdArduino = -1;
    Dmaestro = maestro;  // Maestro Pololu Controller
    Darduino = arduino;  // Arduino
    quiet=0;
    eolchar = '\n';
    timeout = 5000;

    readconfig();
    if(!DEBUG) opendevices();
}

int MBDriver::readconfig()
{
    servo[MOTORHD].offset=0;
    servo[MOTORHD].pos_max=8000;
    servo[MOTORHD].pos_min=4000;

    servo[MOTORHG].offset=0;
    servo[MOTORHG].pos_max=8000;
    servo[MOTORHG].pos_min=4000;

    servo[MOTORBD].offset=0;
    servo[MOTORBG].pos_max=8000;
    servo[MOTORBG].pos_min=4000;

    servo[MOTORCOIN].offset=0;
    servo[MOTORCOIN].pos_max=8000;
    servo[MOTORCOIN].pos_min=4090;

    return 0;
}

int MBDriver::saveconfig()
{
    return 0;
}

int MBDriver::opendevices()
{
    int p[6];
    fdMaestro = open(Dmaestro, O_RDWR | O_NOCTTY);
    if (fdMaestro <= 0)
    {
      perror(Dmaestro);
      return -1;
    }

    printf("Configuring Maestro serial com...");
    struct termios options;
    tcgetattr(fdMaestro, &options);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~(ONLCR | OCRNL);
    tcsetattr(fdMaestro, TCSANOW, &options);
    printf(" done.\n");

    printf("Configuring Arduino serial com...");
    fdArduino = serialport_init(Darduino, 115200);
    if(fdArduino <=0){
          perror(Darduino);
          return -1;
    }
    /* Wait for the Arduino to reset */
    // 	serialport_flush(fdArduino);
    usleep(2000*1000);
    readArduino();
    printf(" done.\n");

    p[0] = maestroGetPosition(0);p[1] = maestroGetPosition(1);p[2] = maestroGetPosition(2);p[3] = maestroGetPosition(3);p[4] = maestroGetPosition(4);p[5] = maestroGetPosition(5);
    printf("Les positions des servos sont actuellement : %d,%d,%d,%d,%d,%d.\n", p[0],p[1],p[2],p[3],p[4],p[5]);


    int target = 3970+(8000-3970)/2;

    printf("Tous les servos sont remis à %d (%d us).\n", target, target/4);
    maestroSetTarget(MOTORHD, target);
    maestroSetTarget(MOTORHG, target);
    maestroSetTarget(MOTORBD, target);
    maestroSetTarget(MOTORBG, target);
    maestroSetTarget(MOTORCOIN, target);
    //maestroSetTarget(fdMaestro, 5, target);  //channel libre.

    return 0;
}

// Renvoi la position du servos 'channel' sur le port SUB 'fd'
int MBDriver::maestroGetPosition(unsigned char channel)
{
  unsigned char command[] = {0x90, channel};
  fd_set set;
  struct timeval timeout;

  if(write(fdMaestro, command, sizeof(command)) == -1)
  {
    perror("error writing");
    return -1;
  }

  unsigned char response[2];
  if(read(fdMaestro,response,2) != 2)
  {
    perror("error reading");
    return -1;
  }
  return response[0] + 256*response[1];
}

// Set la position du servo 'channel' à 'target' en quarter-microseconds.
int MBDriver::maestroSetTarget(unsigned char channel, unsigned short target)
{
  unsigned char command[] = {0x84, channel, target & 0x7F, target >> 7 & 0x7F};
  if (write(fdMaestro, command, sizeof(command)) == -1)
  {
    perror("error writing");
    return -1;
  }
  return 0;
}

int MBDriver::readArduino()
{
  unsigned char response[48];
  int n = read(fdArduino,response,48);
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

int MBDriver::writeArduino(char* buf)
{
    return serialport_write(fdArduino, buf);
}

// Movement de chaque paire de servo en simultané pour une translation des lèvres perpendiculaire au visage
// Max 8000, min 3968
void MBDriver::mvtx(int pas, int haut)
{
  int position = 0;
  if(haut){
    position = maestroGetPosition(MOTORHD);
    printf("Position %i\n",position);
    maestroSetTarget(MOTORHD, position+pas);
    position = maestroGetPosition(MOTORHG);
    maestroSetTarget(MOTORHG, position-pas);
  }else{
    position = maestroGetPosition(MOTORBD);
    maestroSetTarget(MOTORBD, position+pas);
    position = maestroGetPosition(MOTORBG);
    maestroSetTarget(MOTORBG, position-pas);
  }
}

MBDriver::~MBDriver()
{
    if(fdMaestro>0)
        close(fdMaestro);
    if(fdArduino>0)
        serialport_close(fdArduino);
}
