#include<SoftwareSerial.h>
SoftwareSerial SoftSerial(7, 8); //RX,TX，接线要反着接

int LED = 13;
int BTN = 2;

const boolean DEBUG_MODE = true;
String comdata = "";

String PHONE_NO = "13061744909"; //去掉const 这个改为可以读取串口指令

const String AT_ACK_POWER_ON = "IIII";
const String AT_ACK_POWN_DOWN = "NORMAL POWER DOWN";
const String AT_MSG = "AT";
const String AT_ACK_OK_MSG = "OK";

//Long push down
const String ATE1_MSG = "ATE1"; //Call back message
const String ATD_ACK_BUSY = "BUSY";
const String ATD_ACK_CONNECT = "NO CARRIER";
const String ATD_ACK_NO_ANSWER = "NO ANSWER";
//Call
const String ATD_CALL_MSG_BEGIN = "ATD";
const String ATD_CALL_MSG_END = ";";
//Push Once SMS
const String AT_CMGF_MSG = "AT+CMGF=1";
const String AT_CMGS_MSG_BEGIN = "AT+CMGS=\"";
const String AT_CMGS_MSG_END = "\"";
//其他AT指令：
const String AT_CPIN = "AT+CPIN?";//然会READY
const String AT_CSQ ="AT+CSQ";//信号质量 返回+CSQ: 10,0 表示信号强度10 最大有效值31
const String AT_COPS = "AT+COPS?";//查询运营商
const String AT_CGMM = "AT+CGMM";//查询模块型号SIMCOM_SIM900A
const String AT_CGSN = "AT+CGSN";//查序列号


//
const String PushButtonCommand = "PUSH_BTN"; //按一下按钮 （发消息）
const String HoldButtonCommand = "HOLD_BTN"; //按住不放（打电话）
const String AlertFallCommand = "ALERT:FALL"; //发警报消息 跌倒
const String AlertFireCommand = "ALERT:FIRE"; //发警报消息 火警
const String AlertCallCommand = "ALERT:CALL"; //发警报消息 人工发起
const String SetPhonePrefix = "SET:"; //设置指令

const int Hold_Max_Count = 20;

String ActionCommand = "";

boolean Push_Status = false;
boolean Hold_Status = false;
boolean Fire_Status = false;
boolean Fall_Status = false;
boolean Call_Status = false;
int Hold_Count = 0;
boolean ActionFlag = false; //是否触发动作的标记

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  pinMode(BTN, INPUT);

  //波特率设置一致，否则容易出现乱码问题
  Serial.begin(9600);//115200
  SoftSerial.begin(9600); //115200
  SoftSerial.listen();
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  String commandMsg = "";
  String returnMsg = "";

  if (DEBUG_MODE)
  {
    commandMsg = GetSerialData() ;
    commandMsg.trim();
    if (commandMsg == "")
      return;

    Serial.println("rcv:" + commandMsg);

    //根据发送的指令判断是发送哪种消息：按钮按下（测试），跌倒，火警，呼叫
    if (PushButtonCommand == commandMsg) {
      Push_Status = true;
      ActionFlag = true;
      Serial.println("ActionFlag:Push_Status:true");
    }
    else if (commandMsg == AlertFallCommand) {
      Fall_Status = true;
      ActionFlag = true;
      Serial.println("Fall_Status:true");
    }
    else if (commandMsg == AlertFireCommand) {
      Fire_Status = true;
      ActionFlag = true;
      Serial.println("Fire_Status:true");
    }
    else if (commandMsg == AlertCallCommand) {
      Call_Status = true;
      ActionFlag = true;
      Serial.println("Call_Status:true");
    }
    else if (commandMsg.substring(0,4) == SetPhonePrefix)
    {
      //设置号码示例：
      //SET:166*****910
      PHONE_NO = commandMsg.substring(4,15);
      Serial.println(PHONE_NO);
    }
    else if (commandMsg == "GET") {
      Serial.println(PHONE_NO);
    }
    else{
      //其他指令，则表示发送at指令
      Serial.println(commandMsg);
      SoftSerial.println(commandMsg);
      delay(300);
      returnMsg = GetSoftSerialData(); //读取返回信息
      Serial.println(returnMsg);
    }
    /*
      else{
      SoftSerial.println(commandMsg);
      delay(100);
      ///发送回调试串口0/
      delay(100);
      returnMsg = GetSoftSerialData() ;
      Serial.println(returnMsg);
      }
    */

    if (ActionFlag) {
      Serial.println(ActionCommand);
      if (Hold_Status) {
        SoftSerial.println(ATE1_MSG);
        delay(100);
        //returnMsg = GetSerialData() ;
        //ATD18601780171;
        SoftSerial.println(ATD_CALL_MSG_BEGIN + PHONE_NO + ATD_CALL_MSG_END);
        lightHold(); //灯光亮一段时间
      }
      else if (Push_Status) {
        SoftSerial.println(AT_CMGF_MSG);
        delay(100);

        /*发送回调试串口0*/
        delay(100);
        returnMsg = GetSoftSerialData() ;
        Serial.println(returnMsg);
        /*===============*/

        SoftSerial.println(AT_CMGS_MSG_BEGIN + PHONE_NO + AT_CMGS_MSG_END);
        //Serial.println(AT_CMGS_MSG_BEGIN + PHONE_NO + AT_CMGS_MSG_END);
        delay(100);

        /*发送回调试串口0*/
        delay(100);
        returnMsg = GetSoftSerialData() ;
        Serial.println(returnMsg);
        /*===============*/

        SoftSerial.println("I MISS U.");
        //Serial.println("I MISS U.JSW");
        delay(100);
        //char hex1a = char(26);
        //Serial.print(26,HEX);
        //Serial.print(49,HEX);
        SoftSerial.write(26);
        delay(100);
        lightFlash(); //灯光闪烁
      }
      else if (Fall_Status) {
        Serial.println("Fall_Status sms.");
        delay(100);
        SoftSerial.println(AT_CMGF_MSG);
        delay(100);
        delay(100);
        SoftSerial.println(AT_CMGS_MSG_BEGIN + PHONE_NO + AT_CMGS_MSG_END);
        delay(100);
        delay(100);
        SoftSerial.println("Alert Fell down!2-701");
        delay(100);
        SoftSerial.write(26);
        delay(100);
        lightFlash(); //灯光闪烁
      }
      else if (Fire_Status) {
        SendSms("Alert Fire alarm!2-702");
      }
      else if (Call_Status) {
        SendSms("Alert Need help!2-703");
      }
      clearStatus();
    }
  }

}

//发送短信
void SendSms(String sContext) {
  SoftSerial.println(AT_CMGF_MSG);
  delay(300);
  SoftSerial.println(AT_CMGS_MSG_BEGIN + PHONE_NO + AT_CMGS_MSG_END);
  delay(300);
  SoftSerial.println(sContext);
  delay(150);
  //char CTL_SEND[] = "1A";
  //Serial.print(CTL_SEND[0],HEX);
  //Serial.print(CTL_SEND[1],HEX);
  SoftSerial.write(26);
  delay(100);
  lightFlash(); //灯光闪烁
}

void clearLight() {
  digitalWrite(LED, LOW);
}

String GetSoftSerialData() {
  String ReceivedMsg = "";
  while (SoftSerial.available() > 0)
  {
    comdata += char(SoftSerial.read());
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

//灯光亮一段时间
void lightHold() {
  digitalWrite(LED, HIGH);
  delay(5500);
  digitalWrite(LED, LOW);
}

//灯光闪烁
void lightFlash() {
  delay(200);
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
}

void clearStatus() {
  digitalWrite(LED, LOW);
  ActionFlag = false;
  Push_Status = false;
  Hold_Status = false;
  Fire_Status = false;
  Fall_Status = false;
  Call_Status = false;
  Hold_Count = 0;
  ActionCommand = "";
}
