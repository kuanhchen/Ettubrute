/*
  MechEng 706 Base Code

  This code provides basic movement and sensor reading for the MechEng 706 Mecanum Wheel Robot Project

  Hardware:
    Arduino Mega2560 https://www.arduino.cc/en/Guide/ArduinoMega2560
    MPU-9250 https://www.sparkfun.com/products/13762
    Ultrasonic Sensor - HC-SR04 https://www.sparkfun.com/products/13959
    Infrared Proximity Sensor - Sharp https://www.sparkfun.com/products/242
    Infrared Proximity Sensor Short Range - Sharp https://www.sparkfun.com/products/12728
    Servo - Generic (Sub-Micro Size) https://www.sparkfun.com/products/9065
    Vex Motor Controller 29 https://www.vexrobotics.com/276-2193.html
    Vex Motors https://www.vexrobotics.com/motors.html
    Turnigy nano-tech 2200mah 2S https://hobbyking.com/en_us/turnigy-nano-tech-2200mah-2s-25-50c-lipo-pack.html

  Date: 11/11/2016
  Author: Logan Stuart
  Modified: 15/02/2018
  Author: Logan Stuart
*/
#include <Servo.h>  //Need for Servo pulse output

//#define NO_READ_GYRO  //Uncomment of GYRO is not attached.
//#define NO_HC-SR04 //Uncomment of HC-SR04 ultrasonic ranging sensor is not attached.
//#define NO_BATTERY_V_OK //Uncomment of BATTERY_V_OK if you do not care about battery damage.

//State machine states
enum STATE {
  INITIALISING,
  RUNNING,
  STOPPED
};

//Refer to Shield Pinouts.jpg for pin locations

//Default motor control pins
const byte left_front = 46;
const byte left_rear = 47;
const byte right_rear = 50;
const byte right_front = 51;


//Default ultrasonic ranging sensor pins, these pins are defined my the Shield
const int TRIG_PIN = 48;
const int ECHO_PIN = 49;

// Anything over 400 cm (23200 us pulse) is "out of range". Hit:If you decrease to this the ranging sensor but the timeout is short, you may not need to read up to 4meters.
const unsigned int MAX_DIST = 23200;


// --------------------- OUR CODE -------------------------

//set up IR sensors, ultra sonic and gyro
//IR1 is Long range front left
//IR2 is Long range front right
//IR3 is Short range front 
//IR4 is Short range rear

float irSensor1 = A1;
float ir1ADC;
float ir1val;

float irSensor2 = A2;
float ir2ADC;
float ir2val;

float irSensor3 = A3;
float ir3ADC;
float ir3val;

float irSensor4 = A4;
float ir4ADC;
float ir4val;


//------------- Gyro variables----------------------------

int gyroSensor = A5;
float T = millis();
byte serialRead = 0;
int gyroSignalADC = 0;
float gyroSupplyVoltage = 5;

float gyroZeroVoltage = 0; // the value of voltage when gyro is zero
float gyroSensitivity = 0.007; // gyro sensitivity unit is (mv/degree/second) get from datasheet
float rotationThreshold = 1.5; // because of gyro drifting, defining rotation angular velocity less than this value will be ignored
float gyroRate = 0; // read out value of sensor in voltage
float currentAngle = 0; // current angle calculated by angular velocity integral on

//-------UltraSonic Vars---------

float mm = 0;


Servo left_font_motor;  // create servo object to control Vex Motor Controller 29
Servo left_rear_motor;  // create servo object to control Vex Motor Controller 29
Servo right_rear_motor;  // create servo object to control Vex Motor Controller 29
Servo right_font_motor;  // create servo object to control Vex Motor Controller 29
Servo turret_motor;


int speed_val = 0.0;
float speed_change;



//My globals

float tru[] = {0, 0}; //true X then Y distnace
float Y = 0; //from ultrasonic
float sensDist[] = {83.1, 103.64}; //distance between sensors X then Y


//Controller
float setP[] = {0, 150.0, 150.0}; //x1, 150mm, 150mm
float cur[] = {0, tru[0], Y}; //x2, true X, true Y

float kp[] = {10, 2, 1};//1 is largest gain - depedns on error size seen? set to 800m?? so 1*800 = 800 max value is 1000, need 200 for corretcing angle
float ki[] = {0, 0, 0};

float error[] = {0, 0, 0}; // 0 difference between x1 and x2, 1 differnce between true X and 150mm, 2 difference between true Y and 150mm
float ierror[] = {0, 0, 0};

int angle[] = {285, 195, 105}; 

int scenario = 1; //either alligning or 
int rotations = 0;
float cor_factor = 0; //correction factor in x direction

//end of my variables

//setup tom

//Setup gyro
  float sum = 0;
  //Serial.println("please keep the sensor still for calibration");
  //Serial.println("get the gyro zero voltage");



//end of setup and codeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee

//Serial Pointer
HardwareSerial *SerialCom;

int pos = 0;
void setup(void)
{
  turret_motor.attach(11);
  pinMode(LED_BUILTIN, OUTPUT);

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  // Setup the Serial port and pointer, the pointer allows switching the debug info through the USB port(Serial) or Bluetooth port(Serial1) with ease.
  SerialCom = &Serial;
  SerialCom->begin(115200);
  SerialCom->println("MECHENG706_Base_Code_25/01/2018");
  delay(1000);
  SerialCom->println("Setup....");

  delay(1000); //settling time but no really needed

}

void loop(void) //main loop
{
  static STATE machine_state = INITIALISING;
  //Finite-state machine Code
  switch (machine_state) {
    case INITIALISING:
      machine_state = initialising();
      break;
    case RUNNING: //Lipo Battery Volage OK
     // machine_state =  running();        //should work without?

      Serial.print("scenario: ");
      Serial.println(scenario);
      
      //Code boi

     //read_serial_command();
    
      measure();    
      error[0] = setP[0]-cur[0];
      error[1] = setP[1]-cur[1];
      error[2] = setP[2]-cur[2];
      
      //Serial.print("scenario: ");
      //Serial.println(scenario);
      
      switch (scenario) {
       case 1: //ALIGNING      
          alignLHS();      
          break;          
        case 2: //OPERATING      
          forward();
          break;
        case 3: //Rotating
          rotate();  
          break;
        case 4: //stop everything
          stop();
      }

      //end of my code
      
      break;
    case STOPPED: //Stop of Lipo Battery voltage is too low, to protect Battery
      machine_state =  stopped();
      break;
  };
}


STATE initialising() {
  //initialising
  SerialCom->println("INITIALISING....");
  delay(1000); //One second delay to see the serial string "INITIALISING...."
  SerialCom->println("Enabling Motors...");
  enable_motors();
  SerialCom->println("RUNNING STATE...");
  return RUNNING;
}

STATE running() {

  static unsigned long previous_millis;

  // read_serial_command();          //this is the line that allows wasd control
  
  fast_flash_double_LED_builtin();

  if (millis() - previous_millis > 500) {  //Arduino style 500ms timed execution statement
    previous_millis = millis();

    SerialCom->println("RUNNING---------");
    //speed_change_smooth();
    Analog_Range_A4();

#ifndef NO_READ_GYRO
    GYRO_reading();
#endif

#ifndef NO_HC-SR04
    HC_SR04_range();
#endif

#ifndef NO_BATTERY_V_OK
    if (!is_battery_voltage_OK()) return STOPPED;
#endif


    turret_motor.write(pos);

    if (pos == 0)
    {
      pos = 45;
    }
    else
    {
      pos = 0;
    }
  }



  return RUNNING;
}






//Stop of Lipo Battery voltage is too low, to protect Battery
STATE stopped() {
  static byte counter_lipo_voltage_ok;
  static unsigned long previous_millis;
  int Lipo_level_cal;
  disable_motors();
  slow_flash_LED_builtin();

  if (millis() - previous_millis > 500) { //print massage every 500ms
    previous_millis = millis();
    SerialCom->println("STOPPED---------");


#ifndef NO_BATTERY_V_OK
    //500ms timed if statement to check lipo and output speed settings
    if (is_battery_voltage_OK()) {
      SerialCom->print("Lipo OK waiting of voltage Counter 10 < ");
      SerialCom->println(counter_lipo_voltage_ok);
      counter_lipo_voltage_ok++;
      if (counter_lipo_voltage_ok > 10) { //Making sure lipo voltage is stable
        counter_lipo_voltage_ok = 0;
        enable_motors();
        SerialCom->println("Lipo OK returning to RUN STATE");
        return RUNNING;
      }
    } else
    {
      counter_lipo_voltage_ok = 0;
    }
#endif
  }
  return STOPPED;
}

void fast_flash_double_LED_builtin()
{
  static byte indexer = 0;
  static unsigned long fast_flash_millis;
  if (millis() > fast_flash_millis) {
    indexer++;
    if (indexer > 4) {
      fast_flash_millis = millis() + 700;
      digitalWrite(LED_BUILTIN, LOW);
      indexer = 0;
    } else {
      fast_flash_millis = millis() + 100;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }
}

void slow_flash_LED_builtin()
{
  static unsigned long slow_flash_millis;
  if (millis() - slow_flash_millis > 2000) {
    slow_flash_millis = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

void speed_change_smooth()
{
  speed_val += speed_change;
  if (speed_val > 1000)
    speed_val = 1000;
  speed_change = 0;
}

#ifndef NO_BATTERY_V_OK
boolean is_battery_voltage_OK()
{
  static byte Low_voltage_counter;
  static unsigned long previous_millis;

  int Lipo_level_cal;
  int raw_lipo;
  //the voltage of a LiPo cell depends on its chemistry and varies from about 3.5V (discharged) = 717(3.5V Min) https://oscarliang.com/lipo-battery-guide/
  //to about 4.20-4.25V (fully charged) = 860(4.2V Max)
  //Lipo Cell voltage should never go below 3V, So 3.5V is a safety factor.
  raw_lipo = analogRead(A0);
  Lipo_level_cal = (raw_lipo - 717);
  Lipo_level_cal = Lipo_level_cal * 100;
  Lipo_level_cal = Lipo_level_cal / 143;

  if (Lipo_level_cal > 0 && Lipo_level_cal < 160) {
    previous_millis = millis();
    SerialCom->print("Lipo level:");
    SerialCom->print(Lipo_level_cal);
    SerialCom->print("%");
    // SerialCom->print(" : Raw Lipo:");
    // SerialCom->println(raw_lipo);
    SerialCom->println("");
    Low_voltage_counter = 0;
    return true;
  } else {
    if (Lipo_level_cal < 0)
      SerialCom->println("Lipo is Disconnected or Power Switch is turned OFF!!!");
    else if (Lipo_level_cal > 160)
      SerialCom->println("!Lipo is Overchanged!!!");
    else {
      SerialCom->println("Lipo voltage too LOW, any lower and the lipo with be damaged");
      SerialCom->print("Please Re-charge Lipo:");
      SerialCom->print(Lipo_level_cal);
      SerialCom->println("%");
    }

    Low_voltage_counter++;
    if (Low_voltage_counter > 5)
      return false;
    else
      return true;
  }

}
#endif

#ifndef NO_HC-SR04
void HC_SR04_range()
{
  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float cm;
  float mm;

  // Hold the trigger pin high for at least 10 us
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait for pulse on echo pin
  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 0 ) {
    t2 = micros();
    pulse_width = t2 - t1;
    if ( pulse_width > (MAX_DIST + 1000)) {
      SerialCom->println("HC-SR04: NOT found");
      return;
    }
  }

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min

  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 1)
  {
    t2 = micros();
    pulse_width = t2 - t1;
    if ( pulse_width > (MAX_DIST + 1000) ) {
      SerialCom->println("HC-SR04: Out of range");
      return;
    }
  }

  t2 = micros();
  pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed
  //of sound in air at sea level (~340 m/s).
  cm = pulse_width / 58.0 + 10.65; //10.65 is distance from centre of bot to edge of sensor
  mm = cm * 10;

  // Print out results
  if ( pulse_width > MAX_DIST ) {
    SerialCom->println("HC-SR04: Out of range");
  } else {
    SerialCom->print("HC-SR04:");
    SerialCom->print(cm);
    SerialCom->println("cm");
  }
}
#endif

void Analog_Range_A4()
{
  SerialCom->print("Analog Range A4:");
  SerialCom->println(analogRead(A4));
}

#ifndef NO_READ_GYRO
void GYRO_reading()
{
  SerialCom->print("GYRO A3:");
  SerialCom->println(analogRead(A3));
}
#endif

//Serial command pasing
void read_serial_command()
{
  if (SerialCom->available()) {
    char val = SerialCom->read();
    SerialCom->print("Speed:");
    SerialCom->print(speed_val);
    SerialCom->print(" ms ");

    //Perform an action depending on the command
    switch (val) {
      case 'w'://Move Forward
      case 'W':
        forward ();
        SerialCom->println("Forward");
        break;
      case 's'://Move Backwards
      case 'S':
        reverse ();
        SerialCom->println("Backwards");
        break;
      case 'q'://Turn Left
      case 'Q':
        strafe_left();
        SerialCom->println("Strafe Left");
        break;
      case 'e'://Turn Right
      case 'E':
        strafe_right();
        SerialCom->println("Strafe Right");
        break;
      case 'a'://Turn Right
      case 'A':
        ccw();
        SerialCom->println("ccw");
        break;
      case 'd'://Turn Right
      case 'D':
        cw();
        SerialCom->println("cw");
        break;
      case '-'://Turn Right
      case '_':
        speed_change = -100;
        SerialCom->println("-100");
        break;
      case '=':
      case '+':
        speed_change = 100;
        SerialCom->println("+");
        break;
      default:
        stop();
        SerialCom->println("stop");
        break;
    }

  }

}

//----------------------Motor moments------------------------
//The Vex Motor Controller 29 use Servo Control signals to determine speed and direction, with 0 degrees meaning neutral https://en.wikipedia.org/wiki/Servo_control

void disable_motors()
{
  left_font_motor.detach();  // detach the servo on pin left_front to turn Vex Motor Controller 29 Off
  left_rear_motor.detach();  // detach the servo on pin left_rear to turn Vex Motor Controller 29 Off
  right_rear_motor.detach();  // detach the servo on pin right_rear to turn Vex Motor Controller 29 Off
  right_font_motor.detach();  // detach the servo on pin right_front to turn Vex Motor Controller 29 Off

  pinMode(left_front, INPUT);
  pinMode(left_rear, INPUT);
  pinMode(right_rear, INPUT);
  pinMode(right_front, INPUT);
}

void enable_motors()
{
  left_font_motor.attach(left_front);  // attaches the servo on pin left_front to turn Vex Motor Controller 29 On
  left_rear_motor.attach(left_rear);  // attaches the servo on pin left_rear to turn Vex Motor Controller 29 On
  right_rear_motor.attach(right_rear);  // attaches the servo on pin right_rear to turn Vex Motor Controller 29 On
  right_font_motor.attach(right_front);  // attaches the servo on pin right_front to turn Vex Motor Controller 29 On
}
void stop() //Stop
{
  left_font_motor.writeMicroseconds(1500);
  left_rear_motor.writeMicroseconds(1500);
  right_rear_motor.writeMicroseconds(1500);
  right_font_motor.writeMicroseconds(1500);
}

void forward()
{
  left_font_motor.writeMicroseconds(1500 + speed_val - cor_factor);
  left_rear_motor.writeMicroseconds(1500 + speed_val - cor_factor);
  right_rear_motor.writeMicroseconds(1500 - speed_val - cor_factor);
  right_font_motor.writeMicroseconds(1500 - speed_val - cor_factor);
}

void reverse ()
{
  left_font_motor.writeMicroseconds(1500 - speed_val);
  left_rear_motor.writeMicroseconds(1500 - speed_val);
  right_rear_motor.writeMicroseconds(1500 + speed_val);
  right_font_motor.writeMicroseconds(1500 + speed_val);
}

void ccw ()
{
  left_font_motor.writeMicroseconds(1500 - speed_val);
  left_rear_motor.writeMicroseconds(1500 - speed_val);
  right_rear_motor.writeMicroseconds(1500 - speed_val);
  right_font_motor.writeMicroseconds(1500 - speed_val);
}

void cw ()
{
  left_font_motor.writeMicroseconds(1500 + speed_val);
  left_rear_motor.writeMicroseconds(1500 + speed_val);
  right_rear_motor.writeMicroseconds(1500 + speed_val);
  right_font_motor.writeMicroseconds(1500 + speed_val);
}

void strafe_left ()
{
  left_font_motor.writeMicroseconds(1500 - speed_val);
  left_rear_motor.writeMicroseconds(1500 + speed_val);
  right_rear_motor.writeMicroseconds(1500 + speed_val);
  right_font_motor.writeMicroseconds(1500 - speed_val);
}

void strafe_right ()
{
  left_font_motor.writeMicroseconds(1500 + speed_val);
  left_rear_motor.writeMicroseconds(1500 - speed_val);
  right_rear_motor.writeMicroseconds(1500 - speed_val);
  right_font_motor.writeMicroseconds(1500 + speed_val);
}




//MY FUNCTIONS Boi

void alignLHS () {

  Serial.print("error");
  Serial.println(error[0]);
  Serial.println(error[1]);
  Serial.println(error[2]);
  

  if (abs(error[0]) < 6)  { //check if the robot is parallel with the wall
    if (abs(error[1]) < 6) { //check if the robot is 150mm away from it
      scenario = 2;
    }
    else {
      speed_val = Controller(1);
      strafe_right(); //strafe left lol    
    }
  }
  else {
    speed_val = Controller(0);
    Serial.print("speed ");
    Serial.println(speed_val);
    ccw(); //or cw, the controller will output a positive or negative
  }

  gyroSet();
  
}


void measure () {

  int i = 0;
  
  float sens[] = {ir3val, ir1val, ir4val, ir2val}; //x1,y1,x2,y2

  float theta[] = {};
  float avg[] = {};
  float dif[] = {error[0], ir1val-ir2val};



  ir1ADC = analogRead(irSensor1);
  Serial.print("IR Sensor 1 ADC: ");
  Serial.println(ir1ADC);
  ir1val = 0 - pow(ir1ADC,3) * 0.000004 + pow(ir1ADC,2) * 0.0056 - ir1ADC * 2.9377 + 708.67;
  Serial.print("IR Sensor 1 distance: ");
  Serial.println(ir1val);

  ir2ADC = analogRead(irSensor2 );
  Serial.print("IR Sensor 2 ADC: ");
  Serial.println(ir2ADC);
  ir2val = 0 - pow(ir2ADC,3) * 0.000005 + pow(ir2ADC,2) * 0.0072 - ir2ADC * 3.7209 + 831.08;
  Serial.print("IR Sensor 2 distance: ");
  Serial.println(ir2val);

  ir3ADC = analogRead(irSensor3);
  Serial.print("IR Sensor 3 ADC: ");
  Serial.println(ir3ADC);
  ir3val = 0 - pow(ir3ADC,3) * 0.000005 + pow(ir3ADC,2) * 0.0072 - ir3ADC * 3.7209 + 831.08;
  Serial.print("IR Sensor 3 distance: ");
  Serial.println(ir3val);

  ir4ADC = analogRead(irSensor4);
 Serial.print("IR Sensor 4 ADC: ");
  Serial.println(ir4ADC);
  ir4val = 0 - pow(ir4ADC,3) * 0.000004 + pow(ir4ADC,2) * 0.0054 - ir4ADC * 2.4371 + 466.8;
 Serial.print("IR Sensor 4 distance: ");
 Serial.println(ir4val);

  HC_SR04_range();
  readGyro();

  //Serial.print("please");
  //Serial.println(sens[0]);


  while (i < 2){
    avg[i] = (sens[i] + sens[i+1])/2;

     theta[i] = tan(sensDist[i]/dif[i]);

     tru[i] = avg[i] * cos(theta[i]);
     
     i++;   
  }
  i = 0;

  Serial.print("tru");
  Serial.println(tru[0]);

  setP[0] = sens[0];
  cur[0] = sens[2];
  cur[1] = tru[0];
  cur[2] = tru[1];
  Y = mm * cos(theta[1]); //for the ultrasonics sensor
}


void forwards(){

  if (tru[1] > 150) {
  speed_val = Controller(2);
  forward();
  
  if (abs(error[1]) > 6) {                //check if the robot is 150mm away from it
    cor_factor = Controller(0);         //if x1 > x2 -> positive so needs to added on as a negative to robot
  }
  else {
    ierror[0,1,2] = 0;
  }

  //easily change it for only reducing/increasing one side



//   if abs(er[1]) > 6 { //check if the robot is 150mm away from it
//      if er[0] > 0{ //x1 further away than x2 so rotate ccw
//        //increase power on outside wheels
//          //kp[1,2] + error*5?
//        
//        
//      }
//      else {
//        //kp[3,4] + error*5?
//      }
//      
//    
//   }
   //error is same for all its just the differing kp that changes due to x

  }
  else {
    scenario = 3;
  }  
}


void rotate()
{
  //  always turn CW, note that gyro reads -90/270 when turning exactly 90 deg CW
  //  bot should be aligned already before rotating

  float angle; // global variable

  speed_val = 200;

  if (rotations < 4) {

    if (angle > 285) { //angle[rotations];
      cw();
    }
    scenario = 1; 
    rotations ++; 
  }
  else {
    scenario = 4;
  }
}


float Controller (int i) { //ye dude

  float output = 0;
  
  error[i] = setP[i] - cur[i]; 
  ierror[i] = ierror[i] + error[i];
  
  output = kp[i] * error[i] + ki[i] * ierror[i];
  
  if (i > 0) {
    if (abs(output) > 800) { //saftey for motors????  
      output = 800.0;
    }
  }
  else {
    output = 200.0; //saftey on x correction
  }

  Serial.print(i);
  Serial.print(output);
  return output;
  
}


void readGyro() { //tom
  gyroRate = (analogRead(gyroSensor) * gyroSupplyVoltage) / 1023;

  // find the voltage offset the value of voltage when gyro is zero (still)
  gyroRate -= (gyroZeroVoltage / 1023 * gyroSupplyVoltage);
  // read out voltage divided the gyro sensitivity to calculate the angular velocity
  float angularVelocity = gyroRate / gyroSensitivity; // from Data Sheet, gyroSensitivity is 0.007 V/dps

  // if the angular velocity is less than the threshold, ignore it

  if (angularVelocity >= rotationThreshold || angularVelocity <= -rotationThreshold) {
    // we are running a loop in T (of T/1000 second).
    T = millis() - T;
    float angleChange = angularVelocity / (1000 / T);
    T = millis();
    currentAngle += angleChange;
  }

  // keep the angle between 0-360


  if (currentAngle < 0)
  {
    currentAngle += 360;
  }
  else if (currentAngle > 359)
  {
    currentAngle -= 360;
  }

  Serial.print(angularVelocity);
  Serial.print(" ");
  Serial.println(currentAngle);
}



//gyro zero

void gyroSet (){
  for (int i = 0; i < 100; i++) // read 100 values of voltage when gyro is at still, to calculate the zero-drift.
  {
    gyroSignalADC = analogRead(gyroSensor);
    sum += gyroSignalADC;
    delay(5);
  }
  gyroZeroVoltage = sum / 100; // average the sum as the zero drifting


  //delay(1000); //settling time but no really needed

}
