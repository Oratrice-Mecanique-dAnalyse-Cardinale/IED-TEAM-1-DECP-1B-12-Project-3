// Title 


// Header files for Libraries 🗿
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h> 

/*
      Defining sensor pins' locations and setup
                    |
                    |
                    V
*/

// Movement 🚗 (Motor driver module + IR sensors)
#define IR1Pin A0 // A0 input pin for left IR sensor signal
#define IR2Pin A1 // A1 input pin for right IR sensor signal
#define motor_board_input_pin_IN2 6  
#define motor_board_input_pin_IN1 9  
#define motor_board_input_pin_IN4 3 
#define motor_board_input_pin_IN3 5  
#define IR_THRESHOLD 500 

// Ultrasonic sensor pins 🛸
#define TrigPin1 11 // U/S1 Trig -> pin 11
#define EchoPin1 10 // U/S1 Echo -> pin 10
#define TrigPin2 8  // U/S2 Trig -> pin 8
#define EchoPin2 7  // U/S2 Echo -> pin 7


//  𝓟𝓘𝓡 sensor pins
#define PIR_SENSOR_OUTPUT_PIN A2
#define buzzer A3
#define LED_PIN 2 // Red LED -> D2


// LCD 🖥️
LiquidCrystal_I2C lcd(0x27, 16, 2); 

//Target board
#define Hit_aPin 4
int counterA = 0; // Global var for no. of hits detected from the target sensor

// Wifi module ESP8266 
SoftwareSerial espSerial(12, 13);   //Pin 12 and 13 act as RX and TX of ESP8266      
#define DEBUG true
String mySSID = "WifiEddie";       // WiFi SSID from Andriod Hospot
String myPWD = "12345678"; // WiFi Password
String myAPI = "1UI3B82IJA7UQP8S";   // API Key from Thingspeak server
String myHOST = "api.thingspeak.com";
String myPORT = "80";
String myFIELD = "field2"; 
int sendVal; // a global var

// Declaration of FUNCTIONS

// Functions for movement
void move_forward(void);
void turn_left(void);
void turn_right(void);
void brake(void);
int movement_state;
int times_braked = 0;

// Functions from Targetboard 
void detect_light(void);
void slow_down(void);

// Functions for sensors

// Ultrasonic
int getDistance(int EchoPin, int TrigPin);
bool objectDetected = false;
bool motionDetected = false;
unsigned long lastMotionTime = 0;
unsigned long lastObjectTime = 0;


// wifi module
void send_data_to_thingspeak(int pineapple_pizza);
String espData(String command, const int timeout, boolean debug);
/*

MAIN FUNCTIONS setup() and loop() start here (●'◡'●)
                        |
                        |
                        V
*/

void setup() {

  Serial.begin(9600); // sets the baud rate 😭

  //Ultrasonic sensor
  pinMode(TrigPin1, OUTPUT);
  pinMode(EchoPin1, INPUT);
  pinMode(TrigPin2, OUTPUT);
  pinMode(EchoPin2, INPUT);

  //Motor
  pinMode(motor_board_input_pin_IN2, OUTPUT);
  pinMode(motor_board_input_pin_IN1, OUTPUT);
  pinMode(motor_board_input_pin_IN4, OUTPUT);
  pinMode(motor_board_input_pin_IN3, OUTPUT);

  // PIR
  pinMode(PIR_SENSOR_OUTPUT_PIN, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(LED_PIN, OUTPUT); // Set LED pin as output

  //LCD
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System ready");
  Serial.println("System ready");

  //target
  pinMode(Hit_aPin, INPUT_PULLUP);  // Use Hit_aPin as digital INPUT
  PCICR |= B00000100;      // Enable interrupts on PD port (D0 to D7)
  PCMSK2 |= B00000100; // Trigger interrupt on pin D4 (Target Sensor) 

  //wifi
  espSerial.begin(9600);
  espData("AT+RST", 1000, DEBUG);                      //Reset the ESP8266 module
  espData("AT+CWMODE=1", 1000, DEBUG);                 //Set the ESP mode as station mode
  espData("AT+CWJAP=\""+ mySSID +"\",\""+ myPWD +"\"", 1000, DEBUG);

  delay(2000); // Allow sensors to stabilize at startup
}

void loop() {


//   CODE FOR MOVEMENT in general


  int distance1 = getDistance(EchoPin1, TrigPin1);
  int distance2 = getDistance(EchoPin2, TrigPin2);

  int IR1Value = analogRead(IR1Pin);
  int IR2Value = analogRead(IR2Pin);

  // Debugging output to Serial Monitor
  Serial.print("IR1: "); Serial.print(IR1Value);
  Serial.print(" | IR2: "); Serial.println(IR2Value);
  if (IR1Value > IR_THRESHOLD && IR2Value > IR_THRESHOLD){
    move_forward();
    delay(500);
  }
  if (IR1Value <= IR_THRESHOLD && IR2Value > IR_THRESHOLD){
    turn_left(); // The sensor turns left when it detects white on the right and black on the left
  }else if (IR2Value <= IR_THRESHOLD && IR1Value > IR_THRESHOLD){
    turn_right(); // The same as above but the opposite direction
  }else if(IR1Value < IR_THRESHOLD && IR2Value < IR_THRESHOLD){
    turn_left(); // Emergency path-finding when it goes off track (spinny boi)
  }


/*

    CODE FOR SENSORS
            |
            |
            V

*/

  // Ignore invalid or unstable readings for PIR
  
  if (distance1 < 2 || distance1 > 400) distance1 = 400;
  if (distance2 < 2 || distance2 > 400) distance2 = 400;

  bool newObjectDetected = (distance1 <= 10 || distance2 <= 10);
  bool newMotionDetected = detectMotion();

  // Debugging sensor readings PIR
/*
  Serial.print("Distance1: ");
  Serial.print(distance1);
  Serial.print(" cm, Distance2: ");
  Serial.print(distance2);
  Serial.print(" cm, PIR: ");
  Serial.println(digitalRead(PIR_SENSOR_OUTPUT_PIN));
  Serial.print("IR1: "); Serial.print(IR1Value);
  Serial.print(" | IR2: "); Serial.println(IR2Value);
*/
  // Object detection
  if (newObjectDetected && !objectDetected) {
    objectDetected = true;
    motionDetected = false; // Reset motion state
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("STOP");
    lcd.setCursor(0, 1);
    lcd.print("Object detected");
    Serial.println("STOP - Object detected");
    alertSystem();
    lastObjectTime = millis();

  } 
  // Motion detection
  /*
  else if (newMotionDetected && !motionDetected && (millis() - lastObjectTime > 2000)) {
    motionDetected = true;
    objectDetected = false; // Reset object state
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("STOP");
    lcd.setCursor(0, 1);
    lcd.print("Motion detected");
    Serial.println("STOP - Motion detected");
    alertSystem();
    brake();
    lastMotionTime = millis();
  }
*/
  // Idle state
  else if (!newObjectDetected && !newMotionDetected) {
    if (objectDetected || motionDetected) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("System ready");
      Serial.println("System ready");
      objectDetected = false;
      motionDetected = false;
      digitalWrite(LED_PIN, LOW); // Turn OFF LED when idle
    }
  }
  detect_THE_light();
  send_data_to_thingspeak(times_braked);
  delay(200);
}


/*

-------> FUNCTIONS STARTS HERE <-------------

*/
/*
void move_forward() {
  Serial.println("MOVEMENT - Moving forward");
  digitalWrite(motor_board_input_pin_IN2, LOW);
  analogWrite(motor_board_input_pin_IN1, 120);
  digitalWrite(motor_board_input_pin_IN3, LOW);
  analogWrite(motor_board_input_pin_IN4, 120);
}*/

void move_forward(){
  Serial.println("Forward");
  movement_state = 0;
  digitalWrite(motor_board_input_pin_IN2, LOW);
  analogWrite(motor_board_input_pin_IN1, 79); //motor A higher forward speed
  digitalWrite(motor_board_input_pin_IN3, LOW);
  analogWrite(motor_board_input_pin_IN4, 79);// motor B lower reverse speed
}
void turn_right()
{
  Serial.println("Turing right");
  movement_state = 0;
  digitalWrite(motor_board_input_pin_IN2, LOW);
  analogWrite(motor_board_input_pin_IN1, 120); //motor A higher forward speed
  digitalWrite(motor_board_input_pin_IN3, LOW);
  analogWrite(motor_board_input_pin_IN4, 60);// motor B lower reverse speed
}
void turn_left()
{
  Serial.println("Turing left");
  movement_state = 0;
  digitalWrite(motor_board_input_pin_IN2, LOW);
  analogWrite(motor_board_input_pin_IN1, 60); //motor A higher forward speed
  digitalWrite(motor_board_input_pin_IN3, LOW);
  analogWrite(motor_board_input_pin_IN4, 120);// motor B lower reverse speed
}


void slow_down() {
  Serial.println("Headlights detected! Slowing down...");
  analogWrite(motor_board_input_pin_IN1, 0);
  analogWrite(motor_board_input_pin_IN4, 0);
}

void brake(){
  Serial.println("MOVEMENT - Brake");
  times_braked++;
  movement_state = 0;
  analogWrite(motor_board_input_pin_IN2, 0);
  analogWrite(motor_board_input_pin_IN1, 0); 
  analogWrite(motor_board_input_pin_IN3, 0);
  analogWrite(motor_board_input_pin_IN4, 0);
}
/*
              ^
              |
              |

      ༼ つ ◕_◕ ༽つ
                 \ ________________________________
                  |Above functions are for movement|
*/

int getDistance(int EchoPin, int TrigPin) {//Ultrasonic distance detection
  long duration;
  int distance;

  digitalWrite(TrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW);

  duration = pulseIn(EchoPin, HIGH, 30000); // Timeout
  if (duration == 0) return 400; // If no echo is detected, return max range

  distance = duration / 58;

  return distance;
}

bool detectMotion() { // PIR MOTION detection
  if (digitalRead(PIR_SENSOR_OUTPUT_PIN) == HIGH) {
    if (millis() - lastMotionTime > 200) { // Debounce PIR sensor
      lastMotionTime = millis();
      return true;
    }
  }
  return false;
}

void alertSystem() {
  digitalWrite(LED_PIN, HIGH); // Turn the LED ON
  brake();
  soundBuzzer(2,2.5); // how many seconds to play audio + pause and how many loops
  digitalWrite(LED_PIN, LOW); // Turns OFF LED
  Serial.println("Release");
}

void soundBuzzer(int loop_count, float loop_duration) {
  int Fs4 = 740;  // Note F#4 
  int G4 = 784;   // Note G4  
  int noteDuration = (loop_duration-0.5)*1000 / 4; // calculation individual note length
  for (int i = 0; i < loop_count; i++){
    tone(buzzer, Fs4, noteDuration);
    delay(noteDuration);  // Wait for the note to finish

    // Play F#
    tone(buzzer, Fs4, noteDuration);
    delay(noteDuration);

    // Play G
    tone(buzzer, G4, noteDuration);
    delay(noteDuration);

    // Play F#
    tone(buzzer, Fs4, noteDuration);
    delay(noteDuration);

    delay(500);
  }

}


// Functions for sending data
void send_data_to_thingspeak(int pineapple_pizza){ // "pineapple_pizza" is a parameter for what data to send to the server
    sendVal = pineapple_pizza; // Send a random number between 1 and 1000
    String sendData = "GET /update?api_key="+ myAPI +"&"+ myFIELD +"="+String(sendVal);
    espData("AT+CIPMUX=1", 1000, DEBUG);       //Allows multiple connections
    espData("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG);
    espData("AT+CIPSEND=0," +String(sendData.length()+4),1000,DEBUG);  
    espSerial.find(">"); 
    espSerial.println(sendData);
    Serial.print("Value to be sent: ");
    Serial.println(sendVal);
     
    espData("AT+CIPCLOSE=0",1000,DEBUG);

}

String espData(String command, const int timeout, boolean debug){
  Serial.print("AT Command ==> ");
  Serial.print(command);
  Serial.println("     ");
  
  String response = "";
  espSerial.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (espSerial.available())
    {
      char c = espSerial.read();
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }
  return response;
}


/* 
target board functions beneath
            |
            |
            V
*/  
void detect_THE_light(void){
  Serial.print("Light detected count: ");
  Serial.println(counterA);
  Serial.println("Monitoring for headlights...");

  if (!digitalRead(Hit_aPin)) {  // If bright light (headlights) detected
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Light detected");
    brake();
    lcd.setCursor(0, 1);
    lcd.print("1");
    tone(buzzer,2000,100);
    delay(1000); 
    lcd.setCursor(0, 1);
    lcd.print("2");
    tone(buzzer,2000,100);
    delay(1000);
    lcd.setCursor(0,1);
    lcd.print("3");
    tone(buzzer,2000,100);
    delay(1000);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("System ready");
    tone(buzzer,400,200);
    delay(210);
    tone(buzzer,1180,300);

    }               // Slow down the robot
 // ISR_interrupt();
}

/*
void ISR_interrupt (void) { // Interrupt function to detect when the target sensor detects light
  if (!digitalRead(Hit_aPin)) {  // If pin D4 = 0 (light detected)
    counterA++; // Increase counter by 1
  }
}
*/

/*

    A masterpiece crafted by: 


███████ ██████  ██████  ██ ███████         
██      ██   ██ ██   ██ ██ ██              
█████   ██   ██ ██   ██ ██ █████           
██      ██   ██ ██   ██ ██ ██              
███████ ██████  ██████  ██ ███████            (Aung Kaung Khant P2435893)

                                           
███    ██ ███████  ██████                  
████   ██ ██      ██    ██                 
██ ██  ██ █████   ██    ██                 
██  ██ ██ ██      ██    ██                 
██   ████ ███████  ██████                     (Neo June Wai P2401182)     
                                           
                                           
███████  █████  ██████   █████  ██   ██    
██      ██   ██ ██   ██ ██   ██ ██   ██    
███████ ███████ ██████  ███████ ███████    
     ██ ██   ██ ██   ██ ██   ██ ██   ██    
███████ ██   ██ ██   ██ ██   ██ ██   ██       (Sarah Lim Si Qin P2421892)
                                           

     ██  █████  ██    ██ ███████ ███    ██ 
     ██ ██   ██ ██    ██ ██      ████   ██ 
     ██ ███████ ██    ██ █████   ██ ██  ██ 
██   ██ ██   ██  ██  ██  ██      ██  ██ ██ 
 █████  ██   ██   ████   ███████ ██   ████    (Javen Tang P2422439)
                                           
                                          
*/