#include <EEPROM.h>
#include <SD.h>

// #define MOCK
#define MOTOR_PWM_PIN 6
#define LDR_PIN A5
#define DRONE_PULSE_PIN 2
#define LDR_THRESHOLD 850

#define FILE_SUFIX "_LOG.TXT"

#define VERSION "VERSION 0.1"

enum sm{
  IDLE = 0,
  CHECK_PULSE = 1,
  START_MOTOR = 2,
  CHECK_LDR = 3,
  STOP_MOTOR = 4,
  WRITE_SD = 5
};

int state = CHECK_PULSE;
String log_file = String("");
bool drop_flag = false;
unsigned int drone_pulse_counter = 0;
unsigned int capsules_droped = 0;

unsigned long debounce_finish = 0;
unsigned long debounce_interval = 50;


void init_sdcard(){
  if (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    while (1);
  }
  Serial.println("SD initialization done.");

  //Get the name of the next file
  File root = SD.open("/");
  while (true) {
    File entry =  root.openNextFile();
    if (! entry) {
     // no more files
     break;
    }

    String sufix = String(FILE_SUFIX);
    String entry_name = String(entry.name());
    if (entry_name.endsWith(sufix)){
      //found last log file
      entry_name.replace(FILE_SUFIX,"");
      int prefix = entry_name.toInt();
      prefix++;
      log_file = String(prefix)+String(FILE_SUFIX);
    }
    entry.close();
  }
  root.close();

  if(log_file.equals("")){
    log_file = String("0") + String(FILE_SUFIX);
  }
  Serial.print("New log file: ");
  Serial.println(log_file);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.print("Iniciando dispenser ");
  Serial.println(VERSION);
  pinMode(MOTOR_PWM_PIN,OUTPUT);
  pinMode(LDR_PIN,INPUT);
  pinMode(DRONE_PULSE_PIN,INPUT);
  init_sdcard();
  attachInterrupt(digitalPinToInterrupt(DRONE_PULSE_PIN), drone_pulse_callback, RISING);

}

void drone_pulse_callback(){
  drone_pulse_counter++;
  drop_flag = true;
}

void turn_on_motor(){
    digitalWrite(MOTOR_PWM_PIN,HIGH);
    Serial.println("Turn motor on");
}

void turn_off_motor(){
    digitalWrite(MOTOR_PWM_PIN,LOW);
    Serial.println("Turn motor off");
}

void log_to_sd(){
  char log_filename[50];
  log_file.toCharArray(log_filename, log_file.length()+1);
  File file = SD.open(log_filename, FILE_WRITE);
  if (file){
    Serial.println("File opened OK.");
    char msg[50];
    snprintf(msg, 50, "%d,%d", drone_pulse_counter, capsules_droped);
    file.println(msg);
    Serial.println(msg);
    file.close();
  }else{
    Serial.print("Failed to open FILE! :");
    Serial.println(log_filename);
  }
}

bool check_ldr(){
  #ifdef MOCK
    return true;
  #else
    // Serial.println(analogRead(LDR_PIN));
    // Serial.print("Analog read: ");
    return analogRead(LDR_PIN) < LDR_THRESHOLD;
  #endif
}

bool debouncing(){
  return millis() < debounce_finish;
}

void start_debounce(){
  debounce_finish = debounce_interval + millis();
}

void loop()
{
  switch(state){
    case IDLE:
      break;
    case CHECK_PULSE:
      //Serial.print(".");
      if (drop_flag && !debouncing()){
        turn_on_motor();
        drop_flag = false;
        start_debounce();
        state = CHECK_LDR;
        //Serial.println();
      }
      break;
    case CHECK_LDR:
      //Serial.println("|");
      if (check_ldr()){
        capsules_droped++;
        turn_off_motor();
        state = WRITE_SD;
        //Serial.println();
      }
      break;
    case WRITE_SD:
      //Serial.println("WRITE_SD");
      log_to_sd();
      state = CHECK_PULSE;
      break;
  }
  //delay(100);
}
