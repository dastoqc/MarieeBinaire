/*
 * Controlleur HW Mariee Binaire.
 * -----------------
 * Contrôle : 
 * - la puissance durée des électroshock de langue* -> 'a/b/c'XXX (de 0 à 255)
 * - la durée de l'électroshock -> gXXX (0 à 999 ms)
 * - la fréquence des électrochocks -> fXXX (001, 008, 032, 064, 128 ou 256)
 * - la vitesse du stepper pour le menton* -> vXXX (0 à 100)
 * - la rotation du stepper -> 'm/n'XXX (0 à 999 *10steps)
 * - la puissance de l'électrolarynx -> lXXX (0 à 255)
 *
 * PAR DAVID ST-ONGE. 24/08/2015
 * 
 * Inspiration pour le code :
 * http://spacetinkerer.blogspot.ca/2011/02/arduino-control-over-serial-port.html
 * http://playground.arduino.cc/Code/PwmFrequency
 * http://bildr.org/2011/06/easydriver/
 */
 
// Pins du Arduino utilisées
#define TONGUE1 3
#define TONGUE2 9
#define TONGUE3 10
#define LARYNX 6
#define CHINDIR 4
#define CHINSTEP 11
char commandLetter;  // the delineator / command chooser
char numStr[4];      // the number characters and null
int speeda = 0 , speedb = 0, speedc = 0, speedm = 50, valueRead = 0;          // the number stored as a long integer

void setup() {              
  // initialize the pins as outputs.
  pinMode(TONGUE1, OUTPUT);
  analogWrite(TONGUE1, 0);
  pinMode(TONGUE2, OUTPUT);
  analogWrite(TONGUE2, 0);
  pinMode(TONGUE3, OUTPUT);
  analogWrite(TONGUE3, 0);
  pinMode(LARYNX, OUTPUT);
  analogWrite(LARYNX, 0);
  pinMode(CHINSTEP, OUTPUT);
  analogWrite(CHINSTEP, 0);
  pinMode(CHINDIR, OUTPUT);
  // modified the default PWM frequencies
  setPwmFrequency(TONGUE1, 8);
  setPwmFrequency(TONGUE2, 8);
  setPwmFrequency(TONGUE3, 8);
  setPwmFrequency(LARYNX, 1024);
  
  Serial.begin(115200);
  Serial.println("Ready...");
}

void serialEvent() {
  // Parse the string input once enough characters are available
  if(Serial.available() >= 4) {
    commandLetter = Serial.read();
    
    //dump the buffer if the first character was not a letter
    if (!isalpha(commandLetter)) {
      // dump until a letter is found or nothing remains
      while( Serial.available() ) {
        Serial.read(); // throw out the letter
      }
      return;
    }
    
    // read the characters from the buffer into a character array
    int i;
    for( i = 0; i < 3; ++i ) {
      numStr[i] = Serial.read();
    }
    
    //terminate the string with a null prevents atol reading too far
    numStr[i] = '\0';
    
    //read character array until non-number, convert to long int
    valueRead = atoi(numStr);
    //Serial.println(speed);
  }
}

void rotate(int steps, float speed){ 
  //rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
  //speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
  int dir = (steps > 0)? HIGH:LOW;
  steps = abs(steps);

  digitalWrite(CHINDIR,dir); 

  float usDelay = (1/speed) * 7000;

  for(int i=0; i < steps; i++){ 
    digitalWrite(CHINSTEP, HIGH); 
    delayMicroseconds(usDelay); 

    digitalWrite(CHINSTEP, LOW); 
    delayMicroseconds(usDelay); 
  } 
}

void loop() {
    serialEvent();
    if (commandLetter == 'a'){ //pwr to tongue 1
      Serial.print("Speed 1 is ");
      Serial.println(valueRead);
      commandLetter=' ';
      speeda=valueRead;
      valueRead=0;
    }
    else if (commandLetter == 'b'){ //pwr to tongue 2
      Serial.print("Speed 2 is ");
      Serial.println(valueRead);
      commandLetter=' ';
      speedb=valueRead;
      valueRead=0;
    }
    else if (commandLetter == 'c'){ //pwr to tongue 3
      Serial.print("Speed 3 is ");
      Serial.println(valueRead);
      commandLetter=' ';
      speedc=valueRead;
      valueRead=0;
    }
    else if (commandLetter == 'l'){ //pwr to larynx
      analogWrite(LARYNX, valueRead);
      Serial.print("Larynx force is ");
      Serial.println(valueRead);
      commandLetter=' ';
      valueRead=0;
    }
    else if (commandLetter == 'g'){ //shock langue !!
      Serial.print("SHOCK!! ... ");
      analogWrite(TONGUE1, speeda);
      analogWrite(TONGUE2, speedb);
      analogWrite(TONGUE3, speedc);
      delay(valueRead);  //pas exactement des ms!!!
      analogWrite(TONGUE1, 0);
      analogWrite(TONGUE2, 0);
      analogWrite(TONGUE3, 0);
      Serial.println("DONE");
      commandLetter=' ';
      valueRead=0;
    }
    else if (commandLetter == 'f'){ //set freq. des shocks
      setPwmFrequency(TONGUE1, valueRead);
      setPwmFrequency(TONGUE2, valueRead);
      setPwmFrequency(TONGUE3, valueRead);
      Serial.print("Frequency divided by ");
      Serial.println(valueRead);
      commandLetter=' ';
      valueRead=0;
    }
    else if (commandLetter == 'm'){ //position to chin stepper
      rotate(valueRead*10, speedm);
      Serial.print("Chin clockwise goto ");
      Serial.println(valueRead);
      commandLetter=' ';
      valueRead=0;
    }
    else if (commandLetter == 'n'){ //position to chin stepper
      rotate(-valueRead*10, speedm);
      Serial.print("Chin counter-clockwise goto ");
      Serial.println(valueRead);
      commandLetter=' ';
      valueRead=0;
    }
    else if (commandLetter == 'v'){ //speed to chin stepper
      speedm = valueRead;
      Serial.print("Chin speed is ");
      Serial.println(valueRead);
      commandLetter=' ';
      valueRead=0;
    }
}
