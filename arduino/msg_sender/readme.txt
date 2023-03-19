串口发送指令格式：

const String PushButtonCommand = "PUSH_BTN"; //按一下按钮 （发消息）
const String HoldButtonCommand = "HOLD_BTN"; //按住不放（打电话）
const String AlertFallCommand = "ALERT:FALL"; //发警报消息 跌倒
const String AlertFireCommand = "ALERT:FIRE"; //发警报消息 火警
const String AlertCallCommand = "ALERT:CALL"; //发警报消息 人工发起