
/**
 * @file      Service.cpp
 * @Author    Tristan Beau ( tristan.beau@univ-paris-diderot.fr )
 * @date      November, 2017
 * @RevDate   February, 2019
 * @brief     Class implementation for Green House services
 * 
 * Detailed description
 * 
 * Nota : here we define the maximum managed sensors.
 * 
 */

#include "Service.h"

// --- init of members -------------------------------------------
void (*Service::softReset)(void)        = 0;  // function pointer to NULL address
Service* Service::sm_instance           = 0;  // instance init on demand

const int Service::sm_maxStringLength   = 63; 

// --- Creator ------------------------------------------------------------
Service::Service(int wd_pin) {
  // Comm' init
  delay(500);
  Serial.begin(9600); // init serial comm'  
  delay(100);
  Wire.begin();
  delay(500);
  Serial.println("");
  Serial.println("GreenHouse Service Manager started. Waiting for commands now.");
  m_inputString = "";
  m_stringComplete = false;
  m_lastInputString = "RESET"; // used for LCD info
  m_lastStatusString = "ACK INIT OK";
  Serial.println(m_lastStatusString);
  delay(5);
  
  //LCD init 
  m_LCD.CleanAll(WHITE);    //Erase all
  delay(250);            //Delay for 1s.
  //logo USPC
  //LCD.WorkingModeConf(ON, ON, WM_BitmapMode); // for an image
  //LCD.DrawScreenAreaAt(&bmlogo_uspc_big, 0 , 0);
  //delay(2000);
  //Setting text mode for LCD, add a title.
  m_LCD.WorkingModeConf(OFF, ON, WM_CharMode);  // Logo OFF, ScreenLight ON
  m_LCD.CharGotoXY(0, 0);      //Set the start coordinate.
  m_LCD.FontModeConf(Font_6x12, FM_MNL_AAA, WHITE_BAC);
  //m_LCD.print("  ==> Serre USPC <==");m_LCD.print(HFILL_LINE);
  m_LCD.print(" \\(^-^)/  Serre USPC" HFILL_LINE); // see e.g. http://1lineart.kulaone.com/#/
  m_LCD.FontModeConf(Font_6x8, FM_MNL_AAA, BLACK_BAC); // manual newline

  // About WD
  wdt_enable(WDTO_4S);
  m_pinWdLed = wd_pin;
  pinMode(m_pinWdLed,OUTPUT);
  m_WdLedValue = LOW;
  digitalWrite(m_pinWdLed,m_WdLedValue);

  // About sensors
  m_sensorCnt = 0;
  m_sensorArray = new Sensor* [sm_maxSensorCnt];
  m_actuatorCnt = 0;
  m_actuatorArray = new Actuator* [sm_maxActuatorCnt];
}

// --- getInstance() -------------------------------------------------------
Service* Service::getInstance(int wd_pin) {
  if ( ! sm_instance ) {
    sm_instance = new Service(wd_pin); 
  }
  return sm_instance;
}

// --- doLoop() ------------------------------------------------------------
void Service::doLoop() {
  static int i=0;
  int k,x;
  if (m_stringComplete) {
    analyzeCommand();
    m_inputString = "";
    m_stringComplete = false;
  }

  // sensor value update
  if ( !i ) {
    m_LCD.CharGotoXY(0,13);      //Set the start coordinate.
    // print last command and status
    m_LCD.print("@");m_LCD.print(m_lastInputString);m_LCD.print(char(29));m_LCD.print(m_lastStatusString);m_LCD.print(HFILL_LINE);
    for (k=0,x=0;k<m_sensorCnt;k++) {
      float v = getSensor(k)->getLastValue();
      if ( getSensor(k)->getDisplay() == true ) {
          m_LCD.print(getSensor(k)->getID());m_LCD.print(" : " HFILL_LINE);
          m_LCD.CharGotoXY(60,(21+8*x++)%64); // align value display
          m_LCD.print(String(v,getSensor(k)->getPrecision()));m_LCD.print(HFILL_LINE);
      }
    }
    for (k=x;k<5;k++) m_LCD.print(HFILL_LINE); // to blank screen after last displayed sensor info
  }
  if ( i++ > sm_loopsBtwDisplayUpdates ) i=0; // roughly 5 seconds if delay(200) in wd stuff just below
  
  //wd stuff
  delay(sm_delayDeepLoop);
  m_WdLedValue = ! m_WdLedValue;
  digitalWrite(m_pinWdLed,m_WdLedValue);
  wdt_reset();
}

// --- doSerialEvent() : read serial input if any ---------------------------
void Service::doSerialEvent() {
  char inChar;
  while (Serial.available()) {
    // get the new byte:
    inChar = (char)Serial.read();
    // add it to the inputString:
    m_inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      m_stringComplete = true;
      m_inputString.toUpperCase();
    }
  }
  delay(10);
}

// --- Service::printAll ----------------------------------------------------
void Service::printAll() {
  int i,x;
  String s="\n  Connected devices ... \n";
  s += " * Found " ; s+= m_sensorCnt ; s+= " sensors :\n";
  for (i=0;i<m_sensorCnt;i++) {
    s += "Sensor ";
    s += i ;
    s += " (" ;
    s += getSensor(i)->getID();
    s += ") : " ;
    s += getSensor(i)->getLastValue();
    s += "\n";
  }
  s += " * Found " ; s+= m_actuatorCnt; s+= " actuators :\n";
  for (i=0;i<m_actuatorCnt;i++) {
    s += "Actuator " ;
    s += i ;
    s += " (";
    s += getActuator(i)->getID() ;
    s += ") \n";
  }
  s += " * Displayed sensors on LCD screen :\n";
  for (i=0,x=0;i<m_sensorCnt;i++) 
   if (getSensor(i)->getDisplay() ) {
    s += "Display ";
    s += x++ ;
    s += " (" ;
    s += getSensor(i)->getID();
    s += ") : precision " ;
    s += getSensor(i)->getPrecision();
    s += "\n";
  }
  Serial.println(s);
  m_lastStatusString = "ACK ALL";
}

void Service::printHelp() {
  String s="\n Arduino command help ... \n\n";
  s+= " HELP : this help.\n";
  s+= " ALL : reply with all sensor list and values, and all actuator list.\n";
  s+= " GET <sensor name> : get value of the sensor.\n";
  s+= " DISPLAY <sensor name> <0|1> : display (1) or not (0) value on the LCD screen.\n";
  s+= " SET <actuator name> <value> : set value for the specified actuator.\n";
  s+= " RESET : send a software reset.\n\n";
  s+= "If a command is correctly understood, reply with a line begining with ACK.\n";
  s+= "The line may contain the ERR string if an error occurs.\n\n";
  Serial.println(s);
  m_lastStatusString = "ACK HELP";
}

// --- Service::addSensor ---------------------------------------------------
void Service::addSensor(Sensor *s) {
  if (m_sensorCnt < sm_maxSensorCnt) 
    m_sensorArray[m_sensorCnt++] = s;
   else 
    abort();
}

// --- Service::getSensorCnt ------------------------------------------------
int Service::getSensorCnt() const {
  return m_sensorCnt;
}

// --- Service::getSensor(int) ----------------------------------------------
Sensor* Service::getSensor(int i) const {
  if ( i >= getSensorCnt() ) return NULL;
  return m_sensorArray[i];
}

// --- Service::getSensor(String) -------------------------------------------
Sensor* Service::getSensor(String s) const {
  int i=0;
  while (i<getSensorCnt()){
    if ( s == m_sensorArray[i]->getID() ) break;
    i++;
  }
  return getSensor(i);
}


// --- Service::addActuator ---------------------------------------------------
void Service::addActuator(Actuator *s) {
  if (m_actuatorCnt < sm_maxActuatorCnt) 
    m_actuatorArray[m_actuatorCnt++] = s;
   else 
    abort();
}

// --- Service::getSensorCnt ------------------------------------------------
int Service::getActuatorCnt() const {
  return m_actuatorCnt;
}

// --- Service::getSensor(int) ----------------------------------------------
Actuator* Service::getActuator(int i) const {
  if ( i >= getActuatorCnt() ) return NULL;
  return m_actuatorArray[i];
}

// --- Service::getSensor(String) -------------------------------------------
Actuator* Service::getActuator(String s) const {
  int i=0;
  while (i<getActuatorCnt()){
    if ( s == m_actuatorArray[i]->getID() ) break;
    i++;
  }
  return getActuator(i);
}
// --- Service::analyzeCommand() --------------------------------------------
void Service::analyzeCommand() {
  char *word;
  int i=0;
  boolean bContinue=true, bGet=false, bSet=false, bDisp=false;
  char *s;
  char s0[sm_maxStringLength];
  Actuator *lA = NULL;
  Sensor *lS = NULL;

  s = (char*)s0;
  snprintf(s,sm_maxStringLength,"%s",m_inputString.c_str());
  s[strlen(s)-1]='\0'; //suppress the ending \n
  m_lastInputString=s;

  m_lastStatusString = "ACK BAD CMD";
  while ( (word = strtok_r(s," ",&s)) != NULL ) { // split command line in words
    switch(i++) {
      case 0: // first word analysis
        if      ( ! strncmp(word,"SET",3) ) bSet=true;
        else if ( ! strncmp(word,"DISPLAY",3) ) bDisp=true;
        else if ( ! strncmp(word,"GET",3) ) bGet=true;
	      else if ( ! strncmp(word,"ALL",3) ) { printAll(); bContinue=false; }
        else if ( ! strncmp(word,"HELP",3) ) { printHelp(); bContinue=false; }
        else if ( ! strncmp(word,"RESET",3) ) softReset();
        else {
          m_lastStatusString = "ACK UNK CMD";
          bContinue=false;
        }
        break;
      case 1: // second word analysis        
        if (bGet) {
          lS = getSensor(word);
          if ( lS == NULL ) { // that sensor does not exist
            m_lastStatusString = "ACK ERR SENSOR";
            bContinue = false;
            break;
          }
          m_lastStatusString = "ACK " + String(lS->getLastValue(), 3) ; 
          bContinue = false;
        }
        else if (bSet) {
          lA = getActuator(word);
          if ( lA == NULL ) { // that actuator does not exist
            m_lastStatusString = "ACK ERR ACTUATOR";
            bContinue = false;
            break;
          }
          // and then needs 3rd arg
        }
        else if (bDisp) {
          lS = getSensor(word);
          if ( lS == NULL ) { // that sensor does not exist
            m_lastStatusString = "ACK ERR SENSOR";
            bContinue = false;
            break;
          }
          // and then needs 3rd arg
        }
        break;
      case 2: //third word analysis
        if (bSet) {
          lA->setValue( (float) atof(word) );
          m_lastStatusString = "ACK SET OK" ;
        }
        else if (bDisp) {
          lS->setDisplay( (int)atoi(word)?true:false );
          m_lastStatusString = "ACK DISP OK";
        }
        else {
          m_lastStatusString = "ACK LOST";
        }
        break;
      default:
        m_lastStatusString = "ACK LOST";
        break;
    }
    if ( !bContinue ) break;
  }
  Serial.println(m_lastStatusString.c_str());
  Serial.println(" ");
  Serial.flush();
}
