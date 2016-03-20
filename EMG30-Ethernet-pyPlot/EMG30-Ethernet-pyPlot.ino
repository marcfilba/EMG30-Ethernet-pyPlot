#define MOTOR_ENABLE   5
#define MOTOR_LEFT     6
#define MOTOR_RIGHT    7

#define MOTOR_ENC_A    3
#define MOTOR_ENC_B    2

#define MOTOR_MAX_RPM  220

//#define DEBUG
#define ETHERNET

String inString = "";

int realRPM, desiredRPM, correctedRPM = 0;
int error, integralError = 0;
int Kp, Ki = 1;

bool runMotor = false;

#ifdef ETHERNET
  #include <SPI.h>
  #include <Ethernet.h>
  #include <EthernetUdp.h>
  
  byte myMAC [] = { 0x90, 0xA2, 0xDA, 0x0F, 0x48, 0xE5 };
  IPAddress myIP (192, 168, 0, 47);
  
  unsigned int localPort = 8889; // local port to listen on
  char packetBuffer [UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
  EthernetUDP Udp; // EthernetUDP instance to let us receive packets over UDP

  void print (String s){
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write (String(s + "\n").c_str());
    Udp.endPacket();
  }
#else
  void print (String s){
    Serial.println (s);
  }
#endif
void showHelp (){
  print ("P4: Motor Control throw Ethernet");
  print ("1 - Show help");
  print ("2 - Start motor");
  print ("3 - Stop motor");
  print ("4 - Increment RPM in 10");
  print ("5 - Decrement RPM in 10");
}

void setRPM (int rpm){
  desiredRPM = max( min (rpm, MOTOR_MAX_RPM), 0);
  print ("Desired RPM setted to " + String (desiredRPM));
}

void startMotor (){
  runMotor = true;
  setRPM (desiredRPM);
  digitalWrite (MOTOR_LEFT,   HIGH);
  digitalWrite (MOTOR_RIGHT,  LOW);
}

void stopMotor (){
  runMotor = false;
  analogWrite (MOTOR_ENABLE, 0);
}

int getOption (){
  int option = -1;
  #ifdef ETHERNET
    if (Udp.parsePacket()){
      Udp.read (packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      option = String(packetBuffer).toInt();
    }
  #else
    while (Serial.available() > 0){
      int inChar = Serial.read();
      if (isDigit(inChar))  inString += (char)inChar;
      
      if (inChar == '\n') {
        option = inString.toInt();
        inString = "";
      }
    }
  #endif
  return option;
}

void handleOption (){
  switch (getOption ()){
    case -1: break;
    case 1:{                                  showHelp ();              break; }
    case 2:{   print ("Starting motor");      startMotor();             break; }
    case 3:{   print ("Stop motor");          stopMotor();              break; }
    case 4:{   print ("Increment RPM in 10"); setRPM (desiredRPM + 10); break; }
    case 5:{   print ("Decrement RPM in 10"); setRPM (desiredRPM - 10); break; }
    default:{  print ("Option not permitted");                          break; }
  }
}

void printData (){
  #ifdef DEBUG
    print ("desired RPM: " + String (desiredRPM));
    print ("desired RPM: " + String (desiredRPM));
    print ("real RPM: " + String (realRPM));
    print ("corrected RPM: " + String (correctedRPM));
    print ("error: " + String (error));
    print ("integralError: " + String (integralError) + "\n");
  #else
    print (String (desiredRPM) + "," + String (realRPM) + "," + String (correctedRPM) + "," + String (error) + "," + String (integralError));
  #endif
}

void setup() {
  Serial.begin (115200);
  
  pinMode(MOTOR_ENABLE, OUTPUT);
  pinMode(MOTOR_LEFT,   OUTPUT);
  pinMode(MOTOR_RIGHT,  OUTPUT);
  
  pinMode (MOTOR_ENC_A, INPUT);
  pinMode (MOTOR_ENC_B, INPUT);

  digitalWrite (MOTOR_ENC_B, HIGH);

  #ifdef ETHERNET
    Ethernet.begin (myMAC, myIP);
    Udp.begin (localPort);
    
    Serial.begin (115200);
  #endif

  setRPM (120);
  
  showHelp ();
}


/*
 *  360 polsos = 1 revolució 
 *  pulseBlength = microsegons que dura un pols 
 *  1/((pulseBength*180/1000000)/60) 
 */


void loop() {
  handleOption ();

  if (runMotor){    
    // velocity=readEencoder();
    realRPM = 1 / ((pulseIn (MOTOR_ENC_B, HIGH) * 3 / 1000000.0 ));

    // error=velocitySetpoint-velocity;
    error = desiredRPM - realRPM;

    // integralError=integralError+error;
    integralError += (desiredRPM - realRPM);    

    // controlSignal=Kp*error+Ki*IntegralError;
    correctedRPM = desiredRPM + ((Kp * error) + (Ki * integralError));

    // applyInActuators(controlSignal);
    analogWrite (MOTOR_ENABLE, max (min (correctedRPM, 255), 0));

    // en cas d'overflow, corregim els càlculs fets
    if (correctedRPM > 255 or correctedRPM < 0) integralError -= (desiredRPM - realRPM);

    printData ();
  }
  delay (150);
}
