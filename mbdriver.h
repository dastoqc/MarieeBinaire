#ifndef MBDRIVER_H
#define MBDRIVER_H

#define MOTORHD 0
#define MOTORHG 1
#define MOTORBD 2
#define MOTORBG 3
#define MOTORCOIN 4

// Pour tester sans les board branchés en SUB mettre cette valeur à 1.
#define DEBUG 0

struct SERVO {
    int pos_max;
    int pos_min;
    int offset;
};

class MBDriver
{
public:
    MBDriver();
    ~MBDriver();
    int opendevices(char* arduino, char* maestro);
    int readconfig();
    int saveconfig();
    int maestroGetPosition(unsigned char channel);
    int maestroSetTarget(unsigned char channel, unsigned short target);
    int readArduino();
    int writeArduino(char* buf);
    void mvtx(int pas, int haut);

private:
    SERVO servo[5];
    int fdMaestro;
    int fdArduino;
    const char * Dmaestro;  // Maestro Pololu Controller
    const char * Darduino;  // Arduino
    const int buf_max = 256;
    char quiet, eolchar;
    int timeout;
    int rc,n;
};

#endif // MBDRIVER_H
