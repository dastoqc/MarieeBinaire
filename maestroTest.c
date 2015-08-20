// Uses POSIX functions to send and receive data from a Maestro.
// NOTE: The Maestro's serial mode must be set to "USB Dual Port".
// NOTE: You must change the 'const char * device' line below.
// Modified to control the 5 motors of MB Petkova installation - DS2015
// TODO: save mouth position in cvs|xml and read.
// TODO: arduino control of tongue and stepper.

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <termios.h>


#define MOTORHD 0
#define MOTORHG 1
#define MOTORBD 2
#define MOTORBG 3
#define MOTORCOIN 4

int fd = NULL;

static struct termios old, new;

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

// Read 1 character - echo defines echo mode
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

// Read 1 character without echo
char getch(void) 
{
  return getch_(0);
}

// Read 1 character with echo
char getche(void) 
{
  return getch_(1);
}

// Gets the position of a Maestro channel.
// See the "Serial Servo Commands" section of the user's guide.
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

// Sets the target of a Maestro channel.
// See the "Serial Servo Commands" section of the user's guide.
// The units of 'target' are quarter-microseconds.
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

// Move both motor of the 5-bars mecanism in mirror - only X translation
// Max 8000, min 3968
void mvtx(int pas, int haut)
{
  int position = 0;
  if(haut){
	position = maestroGetPosition(fd, MOTORHD);
	printf("Position %i\n",position);
	maestroSetTarget(fd, MOTORHD, position+pas);
	position = maestroGetPosition(fd, MOTORHG);
	maestroSetTarget(fd, MOTORHG, position-pas);
  }else{
	position = maestroGetPosition(fd, MOTORBD);
	maestroSetTarget(fd, MOTORBD, position+pas);
	position = maestroGetPosition(fd, MOTORBG);
	maestroSetTarget(fd, MOTORBG, position-pas);
  }
}

int main(int argc, char **argv)
{
  const char * device = "/dev/ttyACM0";  // Linux
  fd = open(device, O_RDWR | O_NOCTTY);
  if (fd <= 0)
  {
    perror(device);
    return 1;
  }

 printf("Configuring serial com...");
  struct termios options;
  tcgetattr(fd, &options);
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  options.c_oflag &= ~(ONLCR | OCRNL);
  tcsetattr(fd, TCSANOW, &options);
  printf("done.\n");

  int p0 = maestroGetPosition(fd, 0);int p1 = maestroGetPosition(fd, 1);int p2 = maestroGetPosition(fd, 2);int p3 = maestroGetPosition(fd, 3);int p4 = maestroGetPosition(fd, 4);int p5 = maestroGetPosition(fd, 5);
  printf("Current position is %d,%d,%d,%d,%d,%d.\n", p0,p1,p2,p3,p4,p5); 

  int target = 3970+(8000-3970)/2;
  if(argc>1)
	target=atoi(argv[1]);

  printf("Setting target to %d (%d us).\n", target, target/4);
  maestroSetTarget(fd, MOTORHD, target);
  maestroSetTarget(fd, MOTORHG, target);
  maestroSetTarget(fd, MOTORBD, target);
  maestroSetTarget(fd, MOTORBG, target);
  maestroSetTarget(fd, MOTORCOIN, target);
  maestroSetTarget(fd, 5, target);

  printf("Arrows to move, 'q' to quit\n");
  char val=' ';
  while(1){
	val=getch();
	switch(val){
		case '\033':
			getch();val=getch();
			switch(val){
			case 'A':
				printf("LUP-FW!! ");
				mvtx(10,1);
				break;
			case 'B':
				printf("LUP-RW!! ");
				mvtx(-10,1);
				break;
			case 'C':
				printf("LDOWN-FW!! ");
				mvtx(10,0);
				break;
			case 'D':
				printf("LDOWN-RW!! ");
				mvtx(-10,0);
				break;
			}
			break;
		case 'q':
			close(fd);
			return 0;
		case 'a':
			printf("LCOIN-FW!! ");
			p4 = maestroGetPosition(fd, MOTORCOIN);
			printf("Position %i\n",p4);
			maestroSetTarget(fd, MOTORCOIN, p4+10);
			break;
		case 'z':
			printf("LCOIN-RW!! ");
			p4 = maestroGetPosition(fd, MOTORCOIN);
			printf("Position %i\n",p4);
			maestroSetTarget(fd, MOTORCOIN, p4-10);
			break;
		default:
			printf("%c ",val);
	}
  }
  return 0;
}
