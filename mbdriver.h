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
    int start;
    int nb_pos;
};

struct EDEV {
    int max;
    int nb_pos;
};

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

class MBDriver
{
public:
    MBDriver();
    ~MBDriver();
    int opendevices(char* arduino, char* maestro);
    int readconfig();
    int saveconfig();
    void mvtx(int rec_pos, int section);
    void servoIncr(int num, int pas);
    void servoGoTo(int num, int pos);
    void chinGoTo(int rec_pos, int speed = 50);
    void setLarynx(int pwr);
    void setLangue(char zone, int pwr);
    void shock(int t);
    void all(int posarray[5],int pwrL,int pwrC,int pwrLangue[3]);

private:
    SERVO servo[5];
    EDEV elarynx;
    EDEV elangue[3];
    EDEV chinStepper;
    int fdMaestro;
    int fdArduino;
    const char * Dmaestro;  // Maestro Pololu Controller
    const char * Darduino;  // Arduino
    const int buf_max = 256;
    char quiet, eolchar;
    int timeout;
    int rc,n;

    int defaultconfig();
    int readArduino();
    int writeArduino(char* buf);
    int maestroGetPosition(unsigned char channel);
    int maestroSetTarget(unsigned char channel, unsigned short target);
};

#endif // MBDRIVER_H
