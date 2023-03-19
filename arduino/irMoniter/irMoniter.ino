int humanPin = 12;//Human sensing module
int VCC = 6;
int counter=0;

void setup() {
  // put your setup code here, to run once:
  pinMode(humanPin , INPUT);

  pinMode(VCC , OUTPUT);
  digitalWrite(VCC, HIGH);
  Serial.begin(9600);//设置波特率为9600
  
}

void loop() {
  
  // put your main code here, to run repeatedly:
  delay(100);//延时0.05 秒
  counter++;
  int valHuman = digitalRead(humanPin); 
  if (valHuman == HIGH)
  {
    Serial.println("{\"id\":2,\"signal\":\"movement\"}");
    delay(900);//延时0.05 秒
  }
 if (counter == 50)
 {
   Serial.println("{\"id\":2,\"signal\":\"beep\"}");
   counter=0;
 } 
 
}
