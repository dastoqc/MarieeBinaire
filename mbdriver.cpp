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
#include <math.h>

using namespace std;

MBDriver::MBDriver()
{
    cout << "MBDriver: " << __DATE__ << " " __TIME__ << "\n";

    fdMaestro = -1;
    fdArduino = -1;
    Dmaestro = "/dev/ttyACM1";  // Maestro Pololu Controller
    Darduino = "/dev/ttyACM0";  // Arduino
    quiet=0;
    eolchar = '\n';
    timeout = 5000;

    defaultconfig();
    readconfig();
}

int MBDriver::defaultconfig()
{
    for(int i=0;i<5;i++) {
        servo[i].pos_min=4000;
        servo[i].pos_max=8000;
        servo[i].start=6000;
        servo[i].nb_pos=3;
    }
    for(int i=0;i<3;i++) {
        elangue[i].max=120;
        elangue[i].nb_pos=3;
    }
    elarynx.max=255;
    elarynx.nb_pos=3;
    chinStepper.max=300;
    chinStepper.nb_pos=3;
    chinStepper.current=0;
}

int MBDriver::readconfig()
{
    ifstream file("config.txt");
    const std::string whitespace(" \t\n\r");
    if (file)
    {/*
        //Get the size of the file
        file.seekg(0,ios::end);
        streampos          length = file.tellg();
        file.seekg(0,ios::beg);
        // Use a vector as the buffer.
        // It is exception safe and will be tidied up correctly.
        // This constructor creates a buffer of the correct length.
        // Then read the whole file into the buffer.
        vector<char>       buffer(length);
        file.read(&buffer[0],length);
        // Create your string stream.
        // Get the stringbuffer from the stream and set the vector as it source.
        stringstream       localStream;
        localStream.rdbuf()->pubsetbuf(&buffer[0],length);
        // Note the buffer is NOT copied, if it goes out of scope
        // the stream will be reading from released memory.
        */
        string line;
        while( getline(file, line) )
        {
          size_t first_nonws = line.find_first_not_of( whitespace );
          // skip empty lines
          if( first_nonws == string::npos ) {
            continue;
          }
            // skip c++ comments
          if( line.find("//") == first_nonws ) {
              if(DEBUG) cout << "coments!!" << endl;
            continue;
          }
          istringstream is_line(line);
          string nums;
          if( getline(is_line, nums, '.') )
          {
            int num = atoi(nums.c_str());
            string key;
            if( getline(is_line, key, '=') )
            {
                string values;
                if( getline(is_line, values) ){
                    int value = atoi(values.c_str());
                    if(DEBUG) cout << num << " " << key << " " << value << endl;
                    if(num==5)
                        if(key=="maxpos")
                            chinStepper.max = value;
                        if(key=="nbpos")
                            chinStepper.nb_pos = value;
                    else if(num==6){
                        if(key=="max")
                            elarynx.max = value;
                        if(key=="nbpos")
                            elarynx.nb_pos = value;
                    }else if(num>6){
                        if(key=="max")
                            elangue[num].max = value;
                        if(key=="nbpos")
                            elangue[num].nb_pos = value;
                    }else{
                        if(key=="start")
                          servo[num].start = value;
                        if(key=="min")
                            servo[num].pos_min = value;
                        if(key=="max")
                            servo[num].pos_max = value;
                        if(key=="nbpos")
                            servo[num].nb_pos = value;
                    }
                  }
            }
          }
        }
    }

    for(int i=0;i<5;i++)
        cout << "Moteur " << i << " " << "Start:" << servo[i].start << ", Max:" << servo[i].pos_max << ", Min:" << servo[i].pos_min << ", Nbre positions:" << servo[i].nb_pos << endl;
    cout << "ÉlectroLarynx " << "Max:" << elarynx.max << endl;
    cout << "Stepper Menton " << "Max:" << chinStepper.max << ", Nbre positions:" << chinStepper.nb_pos << endl;
    for(int i=0;i<3;i++)
        cout << "ÉlectroLangue " << i << " Max:" << elangue[i].max << endl;

    return 0;
}

int MBDriver::saveconfig()
{
    // TODO
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


    //int target = 3970+(8000-3970)/2;

    cout << "Tous les servos sont remis à leur position de démarrage." << endl;
    for(int i=0;i<5;i++)
        maestroSetTarget(i, servo[i].start);

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
// Selon le nbr de position permises dans la config file. !!1 = min!!
void MBDriver::mvtx(float rec_pos, int sec)
{
  rec_pos--;
  if(sec==1){
    int range = servo[MOTORHD].pos_max - servo[MOTORHD].pos_min;
    if(rec_pos>=servo[MOTORHD].nb_pos)
        rec_pos=servo[MOTORHD].nb_pos-1;
    int position = floor((float)rec_pos/(float)(servo[MOTORHD].nb_pos-1)*(float)range)+servo[MOTORHD].pos_min;
    servoGoTo(MOTORHD, position);
    range = servo[MOTORHG].pos_max - servo[MOTORHG].pos_min;
    if(rec_pos>=servo[MOTORHG].nb_pos)
        rec_pos=servo[MOTORHG].nb_pos-1;
    position = floor(((float)servo[MOTORHG].nb_pos-(float)rec_pos-1)/(float)(servo[MOTORHG].nb_pos-1)*(float)range)+servo[MOTORHG].pos_min;
    servoGoTo(MOTORHG, position);
  }else if(sec==0){
      int range = servo[MOTORBD].pos_max - servo[MOTORBD].pos_min;
      if(rec_pos>=servo[MOTORBD].nb_pos)
          rec_pos=servo[MOTORBD].nb_pos-1;
      int position = floor((float)rec_pos/(float)(servo[MOTORBD].nb_pos-1)*(float)range)+servo[MOTORBD].pos_min;
     servoGoTo(MOTORBD, position);
      range = servo[MOTORBG].pos_max - servo[MOTORBG].pos_min;
      if(rec_pos>=servo[MOTORBG].nb_pos)
          rec_pos=servo[MOTORBG].nb_pos-1;
      position = floor(((float)servo[MOTORBG].nb_pos-(float)rec_pos-1)/(float)(servo[MOTORBG].nb_pos-1)*(float)range)+servo[MOTORBG].pos_min;
      servoGoTo(MOTORBG, position);
  }else if(sec==2){
      int range = servo[MOTORCOIN].pos_max - servo[MOTORCOIN].pos_min;
      if(rec_pos>=servo[MOTORCOIN].nb_pos)
          rec_pos=servo[MOTORCOIN].nb_pos-1;
      int position = floor((float)rec_pos/(float)(servo[MOTORCOIN].nb_pos-1)*(float)range)+servo[MOTORCOIN].pos_min;
      servoGoTo(MOTORCOIN, position);
    }
}

void MBDriver::servoIncr(int pas, int num)
{
    int pos = maestroGetPosition(num)+pas;
    if(pos>servo[num].pos_max)
        pos=servo[num].pos_max;
    else if (pos<servo[num].pos_min)
            pos=servo[num].pos_min;
    maestroSetTarget(num, pos);
    cout << "Position S" << num << ": " << pos << endl;
}

void MBDriver::servoGoTo(int num, int pos)
{
    if(pos>servo[num].pos_max)
        pos=servo[num].pos_max;
    else if (pos<servo[num].pos_min)
            pos=servo[num].pos_min;
    maestroSetTarget(num, pos);
}

void MBDriver::chinGoTo(float rec_pos, int speed)
{
    char* buf;
    rec_pos--;
    int pos = floor((float)rec_pos*(float)chinStepper.max/(float)(chinStepper.nb_pos-1));
    if(abs(pos)>chinStepper.max)
        pos=sgn(pos)*chinStepper.max;

    sprintf(buf, "v%03d", speed);
    int rc = writeArduino(buf);
    usleep(100*1000);
    readArduino();

    pos=pos-chinStepper.current;
    if(pos>0)
        sprintf(buf, "m%03d",pos);
    else
        sprintf(buf, "n%03d",-pos);

    //serialport_flush(fdArduino);
    rc = writeArduino(buf);
    usleep(100*1000);
    readArduino();
    chinStepper.current+=pos;
}

void MBDriver::setLarynx(int pwr)
{
    char* buf;
    if(pwr>elarynx.max)
        pwr=elarynx.max;

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
    char* buf;
    sprintf(buf, "%g%03d",t);
    int rc = writeArduino(buf);
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
