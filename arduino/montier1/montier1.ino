//定义输出IO口
int speakerPin = 7;
int ledPin = 8; // 
//定义传感器输入IO口
int buttonPin = 4;//取消按钮
int firePin = 11;
int humanPin = 9; //Human sensing module
int smokePin = 3;
int gasPin = 10; //天然气
int testButton = 12;

//定义全局变量
int counter = 0;
int counterCancel = 0;


bool bIrMovement = false;//缓存记录当前ir人体感应是否检测到

void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin , OUTPUT);//定义小灯接口为输出接口
  pinMode(speakerPin , OUTPUT);

  pinMode(buttonPin , INPUT);
  pinMode(firePin , INPUT);
  pinMode(humanPin , INPUT);
  pinMode(smokePin , INPUT);
  pinMode(gasPin , INPUT);//暂无处理
  pinMode(testButton , INPUT);
  
  Serial.begin(9600);//设置波特率为9600
}

void loop() {

  delay(150);//延时0.05 秒

  int smokeVal = digitalRead(smokePin); //读出数字量
  int valFire = digitalRead(firePin); //读出数字量
  int valCancel = digitalRead(buttonPin);//取消按钮
  int valHuman = digitalRead(humanPin); //读出数字量
  int valTest = digitalRead(testButton);

  if ((valFire == LOW) || (valTest == LOW))
  {
    counter++;
    digitalWrite(ledPin, HIGH);//亮灯
    tone(speakerPin, 1234);//蜂鸣器bb

    String sHasHuman = "no-movement";
    if (valHuman == HIGH) //如果有人
      sHasHuman = "movement";

    if (counter == 1) {
      Serial.println("{\"id\":1,\"signal\":\"fire\"}");//输出
      Serial.print("{\"id\":1,\"signal\":\"");
      Serial.print(sHasHuman);
      Serial.println("\"}");//输出
      
      counterCancel = 0;
    }
  }

  if (smokeVal == LOW)
  {
    counter++;
    digitalWrite(ledPin, HIGH);
    tone(speakerPin, 1234);

    String sHasHuman = "n";
    if (valHuman == HIGH)
      sHasHuman = "Y";

    if (counter == 1) {
      Serial.println("{\"id\":1,\"signal\":\"smoke\"}");
      counterCancel = 0;
    }
  }

  if (valCancel == HIGH)
  {
    counterCancel++;
    digitalWrite(ledPin, LOW);
    noTone(speakerPin);//蜂鸣器停止bb

    if (counterCancel == 1) {
      Serial.println("{\"id\":1,\"signal\":\"cancel\"}");
      counter = 0;
    }
  }
  else {
    
    if (valHuman == HIGH) {
      if (!bIrMovement) {
        bIrMovement = true;
        Serial.println("{\"id\":1,\"signal\":\"movement\"}");
        delay(100);
      }
    }
    else {
      if (bIrMovement)
        bIrMovement = false;
      delay(100);
    }
    
    delay(100);
  }

}
