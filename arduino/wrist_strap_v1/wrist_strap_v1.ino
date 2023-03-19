
//A6 A7作为串口透传取点口
//#define VccA6 A6
//#define VccA7 A7
//#define GndA0 A0
//连线方法
//MPU-UNO
//VCC-VCC
//GND-GND
//SCL-A5
//SDA-A4
//INT-2 (Optional)

#include <Kalman.h>
#include <Wire.h>
#include <Math.h>

#include<SoftwareSerial.h>
SoftwareSerial espSerial(3, 4); //RX,TX，接线要反着接

float fRad2Deg = 57.295779513f; //将弧度转为角度的乘数
const int MPU = 0x68; //MPU-6050的jnI2C地址
const int nValCnt = 7; //一次读取寄存器的数量

const int nCalibTimes = 1000; //校准时读数的次数
int calibData[nValCnt]; //校准数据

unsigned long nLastTime = 0; //上一次读数的时间
float fLastRoll = 0.0f; //上一次滤波得到的Roll角
float fLastPitch = 0.0f; //上一次滤波得到的Pitch角
Kalman kalmanRoll; //Roll角滤波器
Kalman kalmanPitch; //Pitch角滤波器

int iCounter = 0;


//11 12 作为串口透传取点口
const int vccPin1 = 11;
const int vccPin2 = 12;

const int buttonPin = 7;
const int ledPin =  8;
int buttonState = 0;
bool bMPU = false;

int counter = 0;
int TimeCounter1 = 0;
int TimeCounter2 = 0;
bool bStartCount = false;
String current_status = "";
float temp_r = 0;
bool bFallSend = false;

String comdata = "";
bool DEBUG_MODE  = false;

void setup() {
  //设置串口透传模块的电源和Gnd
  //pinMode(VccA6, OUTPUT);
  //pinMode(VccA7, OUTPUT);
  //pinMode(GndA0, OUTPUT);
  //digitalWrite(VccA6, HIGH);
  //digitalWrite(VccA7, HIGH);
  //digitalWrite(GndA0, LOW);
  pinMode(vccPin1, OUTPUT);
  pinMode(vccPin2, OUTPUT);


  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  // put your setup code here, to run once:
  //Serial.begin(9600); //初始化串口，指定波特率
  Wire.begin(); //初始化Wire库
  WriteMPUReg(0x6B, 0); //启动MPU6050设备

  Calibration(); //执行校准
  nLastTime = micros(); //记录当前时间

  //波特率设置一致，否则容易出现乱码问题
  Serial.begin(9600);//115200
  espSerial.begin(9600); //115200
  delay(500);
}

void loop() {
  //打开IO口供电
  digitalWrite(vccPin1, HIGH);
  digitalWrite(vccPin2, HIGH);

  /*
   * 高电平触发
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
    if (bMPU)
      bMPU = false;
    else
      bMPU = true;

    delay(10); //消抖
    if (bMPU)
      digitalWrite(ledPin, HIGH);// turn LED on:
    else
      digitalWrite(ledPin, LOW); // turn LED off:
  }
  */

  //Serial.print(TimeCounter1);
  
  // put your main code here, to run repeatedly:
  int readouts[nValCnt];
  ReadAccGyr(readouts); //读出测量值

  float realVals[7];
  Rectify(readouts, realVals); //根据校准的偏移量进行纠正

  //计算加速度向量的模长，均以g为单位
  float fNorm = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2]);
  float fRoll = GetRoll(realVals, fNorm); //计算Roll角
  if (realVals[1] > 0) {
    fRoll = -fRoll;
  }
  float fPitch = GetPitch(realVals, fNorm); //计算Pitch角
  if (realVals[0] < 0) {
    fPitch = -fPitch;
  }

  //计算两次测量的时间间隔dt，以秒为单位
  unsigned long nCurTime = micros();
  float dt = (double)(nCurTime - nLastTime) / 1000000.0;
  //对Roll角和Pitch角进行卡尔曼滤波
  float fNewRoll = kalmanRoll.getAngle(fRoll, realVals[4], dt);
  float fNewPitch = kalmanPitch.getAngle(fPitch, realVals[5], dt);
  //跟据滤波值计算角度速
  float fRollRate = (fNewRoll - fLastRoll) / dt;
  float fPitchRate = (fNewPitch - fLastPitch) / dt;

  //更新Roll角和Pitch角
  fLastRoll = fNewRoll;
  fLastPitch = fNewPitch;
  //更新本次测的时间
  nLastTime = nCurTime;
  //===============================================================

  //读取串口消息,输入DEBUG设置为调试模式，输出Roll Pitch rate--------------
  String commandMsg = GetSerialData() ;
  commandMsg.trim();
  if (commandMsg == "DEBUG"){
      DEBUG_MODE = true;
      Serial.println("Debug on");
  }
  else if (commandMsg == "DEBUG_OFF"){
      DEBUG_MODE = false;
      Serial.println("Debug off");
  }

  //读取按钮，判断长按（2s）还是短按-------------------------------------
  bool KEY_HOLD_DOWN = false;
  bool KEY_PRESS = false;
  
  buttonState = digitalRead(buttonPin);
  if  (buttonState == LOW){
    int iHoldCount = 0;
    while((iHoldCount < 200) &&  (buttonState == LOW)) {
      delay(10); //消抖
      buttonState = digitalRead(buttonPin);
      iHoldCount ++;
      //Serial.print(iHoldCount);
    }
    if (iHoldCount < 100){
      KEY_PRESS = true;
      KEY_HOLD_DOWN = false;
    }
    else {
      KEY_PRESS = false;
      KEY_HOLD_DOWN = true;
    }
    
    if  (KEY_HOLD_DOWN){
      if (bMPU)
        bMPU = false;
      else
        bMPU = true;

      if (bMPU)
        digitalWrite(ledPin, HIGH);// turn LED on:
      else
        digitalWrite(ledPin, LOW); // turn LED off:

      Serial.println("{\"id\":0,\"signal\":\"key_hold\"}");
    }
    else if (KEY_PRESS){
      Serial.println("{\"id\":0,\"signal\":\"key_press\"}");
      if (bFallSend){
        Serial.println("{\"id\":0,\"signal\":\"cancel\"}");
      }
      else{
        Serial.println("{\"id\":0,\"signal\":\"call\"}");
      }
    }
  }
  //------------------------------------------------------------------
  if (!bMPU)
    return;
  

  /*
  
  */
  if (DEBUG_MODE){
  //向串口打印输出Roll角和Pitch角，运行时在Arduino的串口监视器中查看
    /*
    Serial.print("Roll:");
    Serial.print(fNewRoll); Serial.print('(');
    Serial.print(fRollRate); Serial.print("),\tPitch:");
    Serial.print(fNewPitch); Serial.print('(');
    Serial.print(fPitchRate); Serial.print(")\n");
    */
    Serial.print("Roll:");
    Serial.print(fRollRate); Serial.print(",\tPitch:");
    Serial.print(fPitchRate); Serial.print("\n");

    return; //调试模式时,输出MPU6050信息后返回
  }
  

  float fRollRate_2 = fRollRate;
  if (fRollRate < 0)
    fRollRate_2 = -fRollRate;

  counter ++;
  if (current_status != "fall")
  {
    if (fRollRate_2 > 140 ) {
      current_status = "fall";
      temp_r = fRollRate_2;
    }
    else if (fRollRate_2 > 80) {
      current_status = "walk";
    }
    else {
      current_status = "stop";
    }
  }

  if (current_status == "fall") {
    Serial.println(current_status);
    Serial.println(temp_r);

    espSerial.println(current_status);
    espSerial.println(temp_r);

    bStartCount = true;
    current_status = "";
  }

  if (bStartCount ) {
    TimeCounter1++;
    TimeCounter2++;
  }

  if (current_status != "stop")
  {
    TimeCounter2 = 0;
  }

  if (TimeCounter2 > 500) {
    Serial.print(TimeCounter1);
    Serial.print("/");
    Serial.print(TimeCounter2);
    Serial.print(":");
    Serial.println("{\"id\":0,\"signal\":\"fall\"}");

    espSerial.print(TimeCounter1);
    espSerial.print("/");
    espSerial.print(TimeCounter2);
    espSerial.print(":");
    espSerial.println("{\"id\":0,\"signal\":\"fall\"}");

    bStartCount = false;
    TimeCounter1 = 0;
    TimeCounter2 = 0;

    bFallSend = true; //已发送跌倒消息
  }

  if (0 == (counter % 100))
  {
    Serial.print(TimeCounter2);
    Serial.print(":");
    Serial.print(current_status);
    Serial.print(":");
    Serial.println(fRollRate_2);

    espSerial.print(TimeCounter2);
    espSerial.print(":");
    espSerial.print(current_status);
    espSerial.print(":");
    espSerial.println(fRollRate_2);
  }


  iCounter ++;
  String sCounter = ""; // String(iCounter);
  String sRollRate = String(fRollRate);
  String sPitchRate = String(fPitchRate);
  String sMsg = sCounter + "Roll," + sRollRate + ",Pitch," + sPitchRate + " ;";
  //Serial.println(sMsg);

  /*
    bool bSendUdp = false;
    if ((fPitchRate > 20) || (fRollRate > 20))
    bSendUdp = true;

    if (bSendUdp) {
    espSerial.println(sMsg);
    }
  */

  /*
    //向串口打印输出Roll角和Pitch角，运行时在Arduino的串口监视器中查看
    Serial.print("Roll:");
    Serial.print(fNewRoll); Serial.print('(');
    Serial.print(fRollRate); Serial.print("),\tPitch:");
    Serial.print(fNewPitch); Serial.print('(');
    Serial.print(fPitchRate); Serial.print(")\n");
  */
  /*
    if (Serial.available())
    {
    //转发串口数据给8266
    espSerial.write(Serial.read());
    }
  */
  delay(10);
}

String GetSerialData() {
  String ReceivedMsg = "";
  while (Serial.available() > 0)
  {
    comdata += char(Serial.read());
    delay(2);
  }
  if (comdata.length() > 0)
  {
    //Serial1.println(ACK_MSG);
    //Serial1.println("{\"" + RECEIVED_FLAG + "\":" + comdata + "}");

    ReceivedMsg = comdata;

    //Serial.println(comdata);
    //Serial.println(GetAction(commandMsg));
    //Serial.println(GetPin(commandMsg));
    //Serial.println(GetStatus(commandMsg));
    //Serial.flush();
    comdata = "";
  }

  return ReceivedMsg;
}

//向MPU6050写入一个字节的数据
//指定寄存器地址与一个字节的值
void WriteMPUReg(int nReg, unsigned char nVal) {
  Wire.beginTransmission(MPU);
  Wire.write(nReg);
  Wire.write(nVal);
  Wire.endTransmission(true);
}

//从MPU6050读出一个字节的数据
//指定寄存器地址，返回读出的值
unsigned char ReadMPUReg(int nReg) {
  Wire.beginTransmission(MPU);
  Wire.write(nReg);
  Wire.requestFrom(MPU, 1, true);
  Wire.endTransmission(true);
  return Wire.read();
}

//从MPU6050读出加速度计三个分量、温度和三个角速度计
//保存在指定的数组中
void ReadAccGyr(int *pVals) {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.requestFrom(MPU, nValCnt * 2, true);
  Wire.endTransmission(true);
  for (long i = 0; i < nValCnt; ++i) {
    pVals[i] = Wire.read() << 8 | Wire.read();
  }
}

//对大量读数进行统计，校准平均偏移量
void Calibration()
{
  float valSums[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0};
  //先求和
  for (int i = 0; i < nCalibTimes; ++i) {
    int mpuVals[nValCnt];
    ReadAccGyr(mpuVals);
    for (int j = 0; j < nValCnt; ++j) {
      valSums[j] += mpuVals[j];
    }
  }
  //再求平均
  for (int i = 0; i < nValCnt; ++i) {
    calibData[i] = int(valSums[i] / nCalibTimes);
  }
  calibData[2] += 16384; //设芯片Z轴竖直向下，设定静态工作点。
}

//算得Roll角。算法见文档。
float GetRoll(float *pRealVals, float fNorm) {
  float fNormXZ = sqrt(pRealVals[0] * pRealVals[0] + pRealVals[2] * pRealVals[2]);
  float fCos = fNormXZ / fNorm;
  return acos(fCos) * fRad2Deg;
}

//算得Pitch角。算法见文档。
float GetPitch(float *pRealVals, float fNorm) {
  float fNormYZ = sqrt(pRealVals[1] * pRealVals[1] + pRealVals[2] * pRealVals[2]);
  float fCos = fNormYZ / fNorm;
  return acos(fCos) * fRad2Deg;
}

//对读数进行纠正，消除偏移，并转换为物理量。公式见文档。
void Rectify(int *pReadout, float *pRealVals) {
  for (int i = 0; i < 3; ++i) {
    pRealVals[i] = (float)(pReadout[i] - calibData[i]) / 16384.0f;
  }
  pRealVals[3] = pReadout[3] / 340.0f + 36.53;
  for (int i = 4; i < 7; ++i) {
    pRealVals[i] = (float)(pReadout[i] - calibData[i]) / 131.0f;
  }
}
