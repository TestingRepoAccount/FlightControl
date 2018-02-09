#include<Wire.h>
#include <SPI.h>
#include <SD.h>
#include "Filters.h"
//#include <I2C.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_HMC5883_U.h>

const int MPU=0x68;  // I2C address of the MPU-6050
const int Mag = 0x1E;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ,MgX,MgY,MgZ;
float fAcX,fAcY,fAcZ,fGyX,fGyY,fGyZ,fMgX,fMgY,fMgZ;

float lastUpdate = 0;    // used to calculate integration interval
float Now = 0;           // used to calculate integration interval
float deltat = 0.0f;        // integration interval for both filter scheme
//#define OutputAccel
//#define OutputGyro
//#define OutputMag
#define OutputYawPitchRoll
//#define OutputJSON
//#define OutputQuat
float yaw, pitch, roll;
void setup(){
  Wire.begin();

  Serial.begin(115200);
  if(!setupSensors()){
  Serial.print(".");
  }
  Serial.println("Sensors Started");
//  mag.begin();
 // Wire.beginTransmission(MPU);
//  Wire.write(0x6B);  // PWR_MGMT_1 register
//  Wire.write(0);     // set to zero (wakes up the MPU-6050)
//  Wire.endTransmission(true);
}

void loop(){

readGyroAccel();
readMag();
  Now = micros();
  deltat = ((Now - lastUpdate)/1000000.0f); // set integration time by time elapsed since last filter update

MahonyQuaternionUpdate(fAcX, fAcY, fAcZ, fGyX*PI/180.0f, fGyY*PI/180.0f, fGyZ*PI/180.0f, MgX, MgY, MgZ, deltat);
lastUpdate = micros();
yaw = *getYPR();
pitch = *(getYPR()+1);
roll = *(getYPR()+2);
#ifdef OutputAccel
Serial.print("\t");
Serial.print(fAcX);
Serial.print("\t");
Serial.print(fAcY);
Serial.print("\t");
Serial.println(fAcZ);
#endif

#ifdef OutputGyro
Serial.print("\t");
Serial.print(GyX);
Serial.print("\t");
Serial.print(GyY);
Serial.print("\t");
Serial.println(GyZ);
#endif

#ifdef OutputMag
Serial.print("\t");
Serial.print(MgX);
Serial.print("\t");
Serial.print(MgY);
Serial.print("\t");
Serial.println(MgZ);
#endif

#ifdef OutputQuat
Serial.print("\t");
Serial.print(q[0]);
Serial.print("\t");
Serial.print(q[1]);
Serial.print("\t");
Serial.print(q[2]);
Serial.print("\t");
Serial.print(q[3]);
Serial.println("\t");

#endif

#ifdef OutputYawPitchRoll
Serial.print(yaw);
Serial.print("\t");
Serial.print(pitch);
Serial.print("\t");
Serial.println(roll);
#endif
#ifdef OutputJSON

String json;
json = "{\"q0\":";
json = json + *getQ();
json = json + ",\"q1\":";
json = json + *(getQ()+1);
json = json + ",\"q2\":";
json = json + *(getQ()+2);
json = json + ",\"q3\":";
json = json + *(getQ()+3);
json = json + "}";
Serial.println(json);
#endif
}

bool setupSensors() {
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  
  Wire.beginTransmission(MPU);
  Wire.write(0x6A);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  
  Wire.beginTransmission(MPU);
  Wire.write(0x37);  // PWR_MGMT_1 register
  Wire.write(0x02);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  Wire.beginTransmission(Mag); //open communication with HMC5883
  Wire.write(0x02); //select mode register
  Wire.write(0x00); //continuous measurement mode
  Wire.endTransmission();

 Wire.beginTransmission(MPU); //Set Acceleration to +-16g
  Wire.write(0x1C);//Register
  Wire.write(0x18);//Afs = 3
  Wire.endTransmission(); 
  return true;
}

void readGyroAccel(){
  //Start Recieving Data
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(0x68,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  fAcX = AcX;
  fAcY = AcY;
  fAcZ = AcZ;
  fGyX = GyX;
  fGyY = GyY;
  fGyZ = GyZ;
  //fAcX -= 800.0f;//FYI I just found what seemed to be the best 
  //fAcZ += 1449.0f;//and got me the most stable data with the noise centered around the 0
  fGyX += 250.0f;//Your values may be different
  fGyY -= 30.0f;
  fGyZ += 40.0f;
//divide it by scaling factor as per datasheet
  fAcX /= 2048.0f;
  fAcY /= 2048.0f;
  fAcZ /= 2048.0f;
  fGyX /= 131;
  fGyY /= 131;
  fGyZ /= 131;
}


void readMag(){
/*sensors_event_t event; 
mag.getEvent(&event);

MgX = event.magnetic.x;
MgY = event.magnetic.y;
MgZ = event.magnetic.z;
*/
  Wire.beginTransmission(Mag);//Address of HMC5883L Magnetometer
  Wire.write(0x3);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(Mag,6,true);
  MgX=Wire.read()<<8|Wire.read();//read registar 3 and 4  
  MgZ=Wire.read()<<8|Wire.read();//read registar 5 and 6
  MgY=Wire.read()<<8|Wire.read();//read registar 6 and 7
  MgX *= .01;
  MgY *= .01;
  MgZ *= .01;
}

