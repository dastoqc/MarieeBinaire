#include "mbdriver.h"

#include "arduino-serial/arduino-serial-lib.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <termios.h>

using namespace std;

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
}

int MBDriver::readconfig()
{
    ifstream file("config.txt");
    if (file)
    {
        /*
         * Get the size of the file
         */
        file.seekg(0,ios::end);
        streampos          length = file.tellg();
        file.seekg(0,ios::beg);

        /*
         * Use a vector as the buffer.
         * It is exception safe and will be tidied up correctly.
         * This constructor creates a buffer of the correct length.
         *
         * Then read the whole file into the buffer.
         */
        vector<char>       buffer(length);
        file.read(&buffer[0],length);

        /*
         * Create your string stream.
         * Get the stringbuffer from the stream and set the vector as it source.
         */
        stringstream       localStream;
        localStream.rdbuf()->pubsetbuf(&buffer[0],length);

        /*
         * Note the buffer is NOT copied, if it goes out of scope
         * the stream will be reading from released memory.
         */
        string line;
        while( getline(localStream, line) )
        {
          istringstream is_line(line);
          string nums;
          if( getline(is_line, nums, '.') )
          {
            int num = atoi(nums.c_str());
            string key;
            if( getline(is_line, key, '=') )
            {
                string value;
                if( getline(is_line, value) ){
                    //cout << num << " " << key << " " << value << endl;
                    if(num==5)
                        chinStepper.max=atoi(value.c_str());
                    else if(num==6){
                        if(key=="min")
                          if ( ! (istringstream(value) >> elarynx.min) ) elarynx.min = 0;
                        if(key=="max")
                          if ( ! (istringstream(value) >> elarynx.max) ) elarynx.max = 800;
                    }else if(num>6){
                        if(key=="min")
                          if ( ! (istringstream(value) >> elangue[num-7].min) ) elangue[num-7].min = 0;
                        if(key=="max")
                          if ( ! (istringstream(value) >> elangue[num-7].max) ) elangue[num-7].max = 800;
                    }else{
                        if(key=="off")
                          if ( ! (istringstream(value) >> servo[num].offset) ) servo[num].offset = 0;
                        if(key=="min")
                          if ( ! (istringstream(value) >> servo[num].pos_min) ) servo[num].pos_min = 4000;
                        if(key=="max")
                          if ( ! (istringstream(value) >> servo[num].pos_max) ) servo[num].pos_max = 8000;
                    }
                  }
            }
          }
        }
    }

    for(int i=0;i<5;i++)
        cout << "Moteur " << i << " " << "Offset:" << servo[i].offset << ", Max:" << servo[i].pos_max << ", Min:" << servo[i].pos_min << endl;
    cout << "ÉlectroLarynx " << "Max:" << elarynx.max << ", Min:" << elarynx.min << endl;
    for(int i=0;i<3;i++)
        cout << "ÉlectroLangue " << i << " Max:" << elangue[i].max << ", Min:" << elangue[i].min << endl;

    return 0;
}

int MBDriver::saveconfig()
{
    return 0;
}

int MBDriver::opendevices(char* arduino, char* maestro)
{
    int p[6];

    Dmaestro = maestro;  // Maestro Pololu Controller
    Darduino = arduino;  // Arduino

    fdMaestro = open(Dmaestro, O_RDWR | O_NOCTTY);
    if (fdMaestro <= 0)
    {
      perror(Dmaestro);
      return -1;
    }

    cout << "Configuration de communication Maestro...";
    struct termios options;
    tcgetattr(fdMaestro, &options);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~(ONLCR | OCRNL);
    tcsetattr(fdMaestro, TCSANOW, &options);
    cout << " complétée." << endl;

    cout << "Configuration de la communication Arduino...";
    fdArduino = serialport_init(Darduino, 115200);
    if(fdArduino <=0){
          perror(Darduino);
          return -1;
    }
    /* Wait for the Arduino to reset */
    // 	serialport_flush(fdArduino);
    usleep(2000*1000);
    readArduino();
    cout << " complétée." << endl;

    p[0] = maestroGetPosition(0);p[1] = maestroGetPosition(1);p[2] = maestroGetPosition(2);p[3] = maestroGetPosition(3);p[4] = maestroGetPosition(4);p[5] = maestroGetPosition(5);
    cout << "Les positions des servos sont actuellement : " << p[0] << ", " << p[1] << ", " << p[2] << ", " << p[3] << ", " << p[4] << ", " << p[5] << ", " << endl;


    int target = 3970+(8000-3970)/2;

    cout << "Tous les servos sont remis à " <<  target << " (" << target/4 << " us)." << endl;
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
  cout << "Message from Arduino: ";//%s\n", response);
  fwrite(response, n, 1, stdout);

  return 0;

}

int MBDriver::writeArduino(char* buf)
{
    return serialport_write(fdArduino, buf);
}

// Movement de chaque paire de servo en simultané pour une translation des lèvres perpendiculaire au visage
// Max 8000, min 3968
void MBDriver::mvtx(int pas, int sec)
{
  int position = 0;
  if(sec==1){
    position = maestroGetPosition(MOTORHD);
    //cout << "Position S" << (int)MOTORHD << ": " << position << endl;
    servoGoTo(MOTORHD, position+pas);
    position = maestroGetPosition(MOTORHG);
    //cout << "Position S" << (int)MOTORHG << ": " << position << endl;
    servoGoTo(MOTORHG, position-pas);
  }else if(sec==0){
    position = maestroGetPosition(MOTORBD);
    //cout << "Position S" << (int)MOTORBD << ": " << position << endl;
    servoGoTo(MOTORBD, position+pas);
    position = maestroGetPosition(MOTORBG);
    //cout << "Position S" << (int)MOTORBG << ": " << position << endl;
    servoGoTo(MOTORBG, position-pas);
  }else if(sec==2){
      position = maestroGetPosition(MOTORCOIN);
      //cout << "Position S" << (int)MOTORCOIN << ": " << position << endl;
      servoGoTo(MOTORCOIN, position+pas);
    }
}

void MBDriver::servoGoTo(int num, int pos)
{
    if(pos>servo[num].pos_max)
        pos=servo[num].pos_max;
    else if (pos<servo[num].pos_min)
            pos=servo[num].pos_min;
    maestroSetTarget(num, pos);
}

void MBDriver::chinGoTo(int pwr)
{
    char* buf;
    if(abs(pwr)>chinStepper.max)
        pwr=sgn(pwr)*chinStepper.max;

    if(pwr>0)
        sprintf(buf, "m%03d",pwr);
    else
        sprintf(buf, "n%03d",-pwr);

    //serialport_flush(fdArduino);
    int rc = writeArduino(buf);
    usleep(100*1000);
    readArduino();
}

void MBDriver::setLarynx(int pwr)
{
    char* buf;
    if(pwr>elarynx.max)
        pwr=elarynx.max;
    else if (pwr<elarynx.min)
        pwr=elarynx.min;

    //serialport_flush(fdArduino);
    sprintf(buf, "l%03d",pwr);
    int rc = writeArduino(buf);
    usleep(20*1000);
    readArduino();
}

void MBDriver::setLangue(char zone, int pwr)
{
    char* buf;
    sprintf(buf, "%c%03d",zone,pwr);
    int rc = writeArduino(buf);
    usleep(20*1000);
    readArduino();

}

void MBDriver::shock(int t)
{
    int rc = writeArduino("g000");
    usleep((500+t)*1000);
    readArduino();
}

void MBDriver::all(int posarray[5],int pwrL,int pwrC,int pwrLangue[3])
{
    for(int i=0;i<5;i++)
        servoGoTo(i, posarray[i]);
    chinGoTo(pwrC);
    setLarynx(pwrL);
    setLangue('a', pwrLangue[0]);
    setLangue('b', pwrLangue[1]);
    setLangue('c', pwrLangue[2]);
    shock(0);
}

MBDriver::~MBDriver()
{
    if(fdMaestro>0)
        close(fdMaestro);
    if(fdArduino>0)
        serialport_close(fdArduino);
}
