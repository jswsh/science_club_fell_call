using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using ConsolCommListener.Entities;
using Newtonsoft.Json;

namespace ConsolCommListener
{
    class Program
    {
        private static SerialPort serialPort1;//本地串口1
        private static SerialPort serialPort2;//本地串口2
        private static SerialPort serialPortSms;//本地串口 sms
        private static bool bOpenSerialSms = true;
        private static string PHONE_NO = "16628525910";

        //Udp端口定义----------------------------------------------------------
        private static bool bIsListeningUdp = true; //是否监听Udp消息
        private static UdpClient UdpRcvClient; //Udp监听Client
        //private static UdpClient UdpAckClient;//不需要全局声明
        private const int iUdpListenPort = 60000; //Udp消息监听端口
        private const int iUdpReplyPort = 60001; //Udpf服务端口
        //---------------------------------------------------------------------
        private static bool bIsSaveUtpLogs = true;
        private static string sLogFilePath1 = @"c:\temp\1";

        private static string sMailTo1 = "jiashu_shanghai@163.com";
        private static string sMailTo2 = "jiashuwei_sh@163.com";

        //--------------------------------------------------------------------
        private static EntityCmd ALERT_CMD = null;
        private static DateTime ALERT_TIME = DateTime.Now;
        private static bool IS_ALERT = false;
        private static bool IsSendSms = false;
        private static int iShowSec = 0;

        private static int CANCEL_TIME = 30;

        static void Main(string[] args) {
            string sCancelTime = System.Configuration.ConfigurationManager.AppSettings["LogFilePath1"];//从配置文件中获取取消时间
            try {
                CANCEL_TIME = int.Parse(sCancelTime);
            }
            catch { }
            
            Console.WriteLine("service started.");
            sLogFilePath1 = System.Configuration.ConfigurationManager.AppSettings["LogFilePath1"];//从配置文件中获取串口
            sMailTo1 = System.Configuration.ConfigurationManager.AppSettings["MailTo1"];
            sMailTo2 = System.Configuration.ConfigurationManager.AppSettings["MailTo2"];
            List<string> lstMailTo = new List<string>();
            if (sMailTo1.Trim().Length > 0)
                lstMailTo.Add(sMailTo1.Trim());
            if (sMailTo2.Trim().Length > 0)
                lstMailTo.Add(sMailTo2.Trim());

            //串口监听打开-----------------------------------------------------------
            bool bOpenSerial1 = true;
            //打开串口
            string sSerialPort1 = "COM3";
            sSerialPort1 = System.Configuration.ConfigurationManager.AppSettings["SerialPort1"];//从配置文件中获取串口
            //var conn = System.Configuration.ConfigurationManager.ConnectionStrings["conn"];
            if ("Y" == System.Configuration.ConfigurationManager.AppSettings["IsListenSerialPort1"]) //从配置中获取是否要打开串口
                bOpenSerial1 = true;
            else
                bOpenSerial1 = false;
            if (bOpenSerial1) {
                InitOpenSerial1(sSerialPort1);
                Console.WriteLine($"serial 1 {sSerialPort1} listener service started.");
            }

            bool bOpenSerial2 = true;
            string sSerialPort2 = "COM4";
            sSerialPort2 = System.Configuration.ConfigurationManager.AppSettings["SerialPort2"];//从配置文件中获取串口
            //var conn = System.Configuration.ConfigurationManager.ConnectionStrings["conn"];
            if ("Y" == System.Configuration.ConfigurationManager.AppSettings["IsListenSerialPort2"]) //从配置中获取是否要打开串口
                bOpenSerial2 = true;
            else
                bOpenSerial2 = false;

            if (bOpenSerial2) {
                InitOpenSerial2(sSerialPort2);
                Console.WriteLine($"serial 2 {sSerialPort2} listener service started.");
            }

            //bool bOpenSerialSms = true; //放到全局
            PHONE_NO = System.Configuration.ConfigurationManager.AppSettings["SmsPhoneNo"];//从配置文件中获取短信发送电话
            string sSerialPortSms = "COM8";
            sSerialPortSms = System.Configuration.ConfigurationManager.AppSettings["SerialPortSms"];//从配置文件中获取串口
            //var conn = System.Configuration.ConfigurationManager.ConnectionStrings["conn"];
            if ("Y" == System.Configuration.ConfigurationManager.AppSettings["IsOpenSerialPortSms"]) //从配置中获取是否要打开串口
                bOpenSerialSms = true;
            else
                bOpenSerialSms = false;

            if (bOpenSerialSms) {
                InitOpenSerialSms(sSerialPortSms);
                Console.WriteLine($"serial 2 {sSerialPortSms} listener service started.");
            }
            //-----------------------------------------------------------------------

            //Udp端口监听打开--------------------------------------------------------
            List<string> lstIp = Common.CommonUtil.GetLocalIp();
            int idx_ip = lstIp.Count - 1;
            string sIp = lstIp[idx_ip];
            IPAddress ipLocal= IPAddress.Parse(sIp); //获得本机ip，也可以自己写死
            IPEndPoint ipLocalEndPoint = new IPEndPoint(ipLocal, iUdpListenPort);
            UdpRcvClient = new UdpClient(ipLocalEndPoint);
            //
            Thread threadReceive = new Thread(UdpMessagesReceive);
            threadReceive.IsBackground = true;
            threadReceive.Start();
            //-----------------------------------------------------------------------
            int itc = 0;
            //接收exit退出命令-------------------------------------------------------
            Console.WriteLine("Input 'exit' to quit...");
            bool bStopRunning = false;
            while (!bStopRunning) {
                string sInput = Console.ReadLine();
                if (sInput.ToLower() == "exit") {
                    bStopRunning = true;
                    bIsListeningUdp = false;
                }
                else if (sInput.ToLower() == "sendmail") {
                    if ((lstMailTo != null) && (lstMailTo.Count > 0))
                        Common.CommonUtil.SendMail(lstMailTo, "", "让我们来发一封邮件，试一下吧。");
                }
                else if (sInput.ToLower() == "sendsms") {
                    if (SendSms(PHONE_NO,"ALERT:CALL"))
                        Console.WriteLine("Send sms ok");
                    else
                        Console.WriteLine("Send sms error");
                }
                else if (sInput.ToLower() == "testfire") {
                    EntityCmd tc = new EntityCmd();
                    tc.id = 9;
                    tc.signal = "fire";
                    StartAlertTimer(tc);
                }

                itc++;
                if (IS_ALERT) {//如果之前有过警报，则要开始计时，1分钟之类若没有取消则要开始发消息-改为30s
                    TimeSpan ts = DateTime.Now - ALERT_TIME;
                    double dSec = ts.TotalSeconds;
                    //if ((itc % 10) == 0)
                        Console.WriteLine(dSec);
                    //double dSecFff = ts.TotalMilliseconds;
                    if (!IsSendSms && (dSec > CANCEL_TIME)) {
                        SendAlertMsg(lstMailTo, PHONE_NO,ALERT_CMD);
                        IsSendSms = true;
                        IS_ALERT = false;
                    }
                    if (iShowSec == 0) {
                        iShowSec = (int)dSec;
                        Console.Write(iShowSec.ToString() + " . ");
                    }
                    else if (iShowSec < ((int)dSec)) {
                        iShowSec = (int)dSec;
                        Console.Write(iShowSec.ToString() + " . ");
                    }
                    Thread.Sleep(10);
                }
            }
            //-----------------------------------------------------------------------

            //关闭串口--------------------------------------------------------
            if (bOpenSerial1) 
                CloseSerialPort1();
            if (bOpenSerial2)
                CloseSerialPort2();
            if (bOpenSerial2)
                CloseSerialPortSms();
            //退出监听Udp-----------------------------------------------------
            try {
                if (threadReceive != null) {
                    threadReceive.Interrupt();
                    threadReceive.Abort();
                }
            }
            catch(Exception ex) {
                Console.WriteLine("e:" + ex.Message);
            }
            //----------------------------------------------------------------
            Console.WriteLine("Exit app running.");
        }

        

        /// <summary>
        /// 发送Sms短消息
        /// </summary>
        /// <param name="sPhoneNo"></param>
        /// <param name="sCmd"></param>
        /// <returns></returns>
        private static bool SendSms(string sPhoneNo, string sCmd) {
            bool bSendSms = false;
            if (!bOpenSerialSms)
                return false;

            if (!serialPortSms.IsOpen)
                return false;

            byte[] WriteBufferSetCmd = Encoding.ASCII.GetBytes("SET:" + sPhoneNo); //+ "\r\n"
            serialPortSms.Write(WriteBufferSetCmd, 0, WriteBufferSetCmd.Length);
            serialPortSms.WriteLine("SET:" + sPhoneNo);
            Thread.Sleep(500);
            byte[] WriteBufferMsgCmd = Encoding.ASCII.GetBytes(sCmd ); //+"\n"
            serialPortSms.Write(WriteBufferMsgCmd, 0, WriteBufferMsgCmd.Length);
            Thread.Sleep(300);
            Console.WriteLine(" [Send sms ok]");
            bSendSms = true;

            return bSendSms;
        }

        /// <summary>
        /// Udp端口收到消息处理
        /// </summary>
        /// <param name="obj"></param>
        private static void UdpMessagesReceive(object obj) {
            //string ssUdp = "";
            bIsSaveUtpLogs = true;

            int iCount = 0;
            string sLog = "";
            IPEndPoint remoteIpEndPoint = new IPEndPoint(IPAddress.Any, 0);
            while (bIsListeningUdp) {
                try {
                    iCount++;
                    byte[] receiveBytes = UdpRcvClient.Receive(ref remoteIpEndPoint);
                    string sUdpMessage = System.Text.Encoding.UTF8.GetString(receiveBytes, 0, receiveBytes.Length);
                    sLog += sUdpMessage;
                    Console.Write(remoteIpEndPoint.Address.ToString() + " : ");
                    Console.Write(sUdpMessage);//发送方信息println带了回车，这里不需要 WriteLine

                    if ((bIsSaveUtpLogs) && ((iCount % 10) == 0))
                        Common.CommonLogs.SaveLogs(sLogFilePath1, sLog);
                    continue; //暂不处理消息

                    //bool bFin = false;
                    //if (sUdpMessage != ";") {
                    //    ssUdp += sUdpMessage;
                    //    continue;
                    //}
                    //else
                    //    bFin = true;

                    //if (bFin) {
                    //    //string[] date = sUdpMessage.Split('$');
                    //    Console.Write(remoteIpEndPoint.Address.ToString() + " : ");
                    //    Console.WriteLine(ssUdp);
                    //}

                    IPAddress ipRemote = remoteIpEndPoint.Address; //获得发送方ip
                    IPEndPoint ipRemoteEndPoint = new IPEndPoint(ipRemote, iUdpReplyPort);
                    using (UdpClient UdpAckClient = new UdpClient()) { //ipRemoteEndPoint
                        byte[] replyAck = Encoding.UTF8.GetBytes("{\"ack\"}");
                        UdpAckClient.Send(replyAck, replyAck.Length,new IPEndPoint(ipRemote, iUdpReplyPort));//发送ack回复消息
                    }
                    //开始分析处理消息
                    List<Entities.EntityCmd> lstCmd = Biz.ListenerBiz.GetCmd(sUdpMessage);

                    AnalysisProcessingMessage(lstCmd);
                }
                catch(Exception ex) {
                    Console.WriteLine("e:" + ex.Message);
                }
            }
        }

        /// <summary>
        /// 关闭串口连接
        /// </summary>
        private static void CloseSerialPort1() {
            try {
                if (serialPort1.IsOpen)
                    serialPort1.Close();
                Console.WriteLine("Serial port1 closed.");
            }
            catch (Exception ex) {
                Console.WriteLine("e:" + ex.Message);
            }
        }
        private static void CloseSerialPort2() {
            try {
                if (serialPort2.IsOpen)
                    serialPort2.Close();
                Console.WriteLine("Serial port2 closed.");
            }
            catch (Exception ex) {
                Console.WriteLine("e:" + ex.Message);
            }
        }

        private static void CloseSerialPortSms() {
            try {
                if (serialPortSms.IsOpen)
                    serialPortSms.Close();
                Console.WriteLine("Serial port Sms closed.");
            }
            catch (Exception ex) {
                Console.WriteLine("e:" + ex.Message);
            }
        }

        /// <summary>
        /// 串口初始化
        /// </summary>
        /// <param name="sSerialPort"></param>
        private static void InitOpenSerial1(string sSerialPort) {
            serialPort1 = new SerialPort(sSerialPort);//端口
            serialPort1.BaudRate = 9600;//波特率
            serialPort1.Parity = Parity.None;//校验位
            serialPort1.StopBits = StopBits.One;//停止位
            serialPort1.DataBits = 8;//数据位
            serialPort1.Handshake = Handshake.None;
            serialPort1.ReadTimeout = 1500;
            serialPort1.DtrEnable = true;//启用数据终端就绪信息
            serialPort1.Encoding = Encoding.UTF8;
            serialPort1.ReceivedBytesThreshold = 1;//DataReceived触发前内部输入缓冲器的字节数
            serialPort1.DataReceived += new SerialDataReceivedEventHandler(port_DataReceived);

            try {
                serialPort1.Open();
            }
            catch (Exception ex) {
                Console.WriteLine("e:" + ex.Message);
                return;
            }

            Console.WriteLine(sSerialPort + " opened.");
        }
        private static void InitOpenSerial2(string sSerialPort) {
            serialPort2 = new SerialPort(sSerialPort);//端口
            serialPort2.BaudRate = 9600;//波特率
            serialPort2.Parity = Parity.None;//校验位
            serialPort2.StopBits = StopBits.One;//停止位
            serialPort2.DataBits = 8;//数据位
            serialPort2.Handshake = Handshake.None;
            serialPort2.ReadTimeout = 1500;
            serialPort2.DtrEnable = true;//启用数据终端就绪信息
            serialPort2.Encoding = Encoding.UTF8;
            serialPort2.ReceivedBytesThreshold = 1;//DataReceived触发前内部输入缓冲器的字节数
            serialPort2.DataReceived += new SerialDataReceivedEventHandler(port_DataReceived);

            try {
                serialPort2.Open();
            }
            catch (Exception ex) {
                Console.WriteLine("e:" + ex.Message);
                return;
            }

            Console.WriteLine(sSerialPort + " opened.");
        }

        private static void InitOpenSerialSms(string sSerialPort) {
            serialPortSms = new SerialPort(sSerialPort);//端口
            serialPortSms.BaudRate = 9600;//波特率
            serialPortSms.Parity = Parity.None;//校验位
            serialPortSms.StopBits = StopBits.One;//停止位
            serialPortSms.DataBits = 8;//数据位
            serialPortSms.Handshake = Handshake.None;
            serialPortSms.ReadTimeout = 1500;
            serialPortSms.DtrEnable = true;//启用数据终端就绪信息
            serialPortSms.Encoding = Encoding.UTF8;
            serialPortSms.ReceivedBytesThreshold = 1;//DataReceived触发前内部输入缓冲器的字节数
            serialPortSms.DataReceived += new SerialDataReceivedEventHandler(port_DataReceived);

            try {
                serialPortSms.Open();
            }
            catch (Exception ex) {
                Console.WriteLine("e:serialPortSms-" + ex.Message);
                return;
            }

            Console.WriteLine(sSerialPort + " opened(serialPortSms).");
        }


        /// <summary>
        /// 接收倒串口消息
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private static void port_DataReceived(object sender, SerialDataReceivedEventArgs e) {
            SerialPort sp = (SerialPort)sender;
            string sSerialRead = sp.ReadExisting();
            Console.Write(sSerialRead);
            //开始分析处理消息
            List<Entities.EntityCmd> lstCmd = null;
            try {
                lstCmd = Biz.ListenerBiz.GetCmd(sSerialRead);
            }
            catch (Exception ex){
                Console.WriteLine("ex:" + sSerialRead);
                Console.WriteLine(ex.Message);
            }
            if (lstCmd != null)
                AnalysisProcessingMessage(lstCmd);
        }

        /// <summary>
        /// 解析串口消息：如果是报警则设置计时器开始，是取消则清除计时器计时状态
        /// </summary>
        /// <param name="lstCmd"></param>
        private static void AnalysisProcessingMessage(List<EntityCmd> lstCmd) {
            if (lstCmd == null) {
                Console.WriteLine("lstCmd:null");
                return;
            }
            Console.WriteLine(lstCmd.Count);
            if (lstCmd.Count ==0) {
                Console.WriteLine("lstCmd:length 0");
                return;
            }
            Console.WriteLine(JsonConvert.SerializeObject(lstCmd));

            foreach (EntityCmd eCmd in lstCmd) {
                /*
                 跌倒报警：{"id":1,"signal":"fall"}
                 火警报警：{"id":1,"signal":"fire"}
                 烟雾报警：{"id":1,"signal":"smoke"}
                 人体监测：{"id":1,"signal":"movement"}
                 取消警报：{"id":1,"signal":"cancel"}
                 */

                if (eCmd.signal == "cancel") { //如果是cancel，则取消之前的状态
                    ALERT_CMD = null;
                    IS_ALERT = false;
                    Console.WriteLine("alert canceled.");
                    return;
                }
                Console.WriteLine(eCmd.signal);
                if ((eCmd.signal != "movement") && (eCmd.signal != "no-movement")) { //不是有人移动状态，则开始计时
                    StartAlertTimer(eCmd); //开始计时
                }
            }
            //if (eCmd.signal == )
        }

        /// <summary>
        /// 开始计时 
        /// </summary>
        /// <param name="eCmd"></param>
        private static void StartAlertTimer(EntityCmd eCmd) {
            ALERT_TIME = DateTime.Now;
            ALERT_CMD = eCmd;
            IS_ALERT = true;
            Console.WriteLine("alert timer start." + ALERT_TIME.ToString("yy-MM-dd HH:mm:ss.fff"));
        }

        /// <summary>
        /// 发送短信和邮件报警消息
        /// </summary>
        /// <param name="lstMailTo"></param>
        /// <param name="sPhoneNo"></param>
        /// <param name="alertCmd"></param>
        private static void SendAlertMsg(List<string> lstMailTo, string sPhoneNo, EntityCmd alertCmd) {
            string sCmd = "ALERT:CALL";

            if (alertCmd.signal == "fall")
                sCmd = "ALERT:FALL";
            else if (alertCmd.signal == "call")
                sCmd = "ALERT:CALL";
            else if ((alertCmd.signal == "fire") || (alertCmd.signal == "smoke"))
                sCmd = "ALERT:FIRE";

            SendSms(sPhoneNo, sCmd);

            string sMailContent = $"{DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss")} : 警报:{alertCmd.signal}";
            if ((lstMailTo != null) && (lstMailTo.Count > 0))
                Common.CommonUtil.SendMail(lstMailTo, alertCmd.signal, sMailContent);
        }
    }
}
