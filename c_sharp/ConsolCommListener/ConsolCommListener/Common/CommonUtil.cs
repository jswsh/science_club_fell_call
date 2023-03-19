using System;
using System.Collections.Generic;
using System.Net;
using System.Text;

namespace ConsolCommListener.Common
{
    class CommonUtil
    {
        /// <summary>
        /// .net 5 的获取本机ip
        /// </summary>
        /// <returns></returns>
        internal static List<string> GetLocalIp() {
            List<string> lstIps = new List<string>();
            try {

                String hostName = Dns.GetHostName();
                IPHostEntry iphe = Dns.GetHostEntry(hostName);
                IPAddress[] localIps = iphe.AddressList;
                foreach (var ip in localIps) {
                    if (ip.IsIPv6LinkLocal)
                        continue;
                    if (ip.MapToIPv4().ToString() != "127.0.0.1") {
                        string sIp = ip.MapToIPv4().ToString();
                        lstIps.Add(sIp);
                    }
                }
            }
            catch (Exception ex) {
                Console.WriteLine("获取本机ip异常：" + ex.Message);
            }

            return lstIps;
        }

        internal static void SendMail(List<string> lstMailTo, string sSubject, string sContext) {
            if ((lstMailTo == null) || (lstMailTo.Count == 0))
                return;

            if (sSubject.Trim() == "")
                sSubject = "信息发送：" + DateTime.Now.ToString("yyyy-MM-dd") + " 的消息";

            //todo 发送通知邮件
            System.Net.Mail.SmtpClient client = new System.Net.Mail.SmtpClient();
            client.Host = "smtp.163.com";//使用163的SMTP服务器发送邮件
            client.UseDefaultCredentials = true;
            client.DeliveryMethod = System.Net.Mail.SmtpDeliveryMethod.Network;
            client.Credentials = new System.Net.NetworkCredential("sunny_support@163.com", "TDRNXKSSCQKVVLJT");//163的SMTP服务器需要用163邮箱的用户名和密码作认证，如果没有需要去163申请个, jsw@2021Feb
                                                                                                               //这里假定你已经拥有了一个163邮箱的账户，用户名为abc，密码为*******
            System.Net.Mail.MailMessage Message = new System.Net.Mail.MailMessage();
            Message.From = new System.Net.Mail.MailAddress("sunny_support@163.com");//这里需要注意，163似乎有规定发信人的邮箱地址必须是163的，而且发信人的邮箱用户名必须和上面SMTP服务器认证时的用户名相同
                                                                                    //因为上面用的用户名abc作SMTP服务器认证，所以这里发信人的邮箱地址也应该写为abc@163.com
            if (lstMailTo.Count > 0) {
                foreach (string sMailTo in lstMailTo) {
                    Message.To.Add(sMailTo);
                }
                //Message.To.Add("jiashu_shanghai@163.com");//将邮件发送给Gmail
                //Message.To.Add("jiashuwei_sh@163.com");//将邮件发送给QQ邮箱
                Message.Subject = sSubject;
                Message.Body = sContext;
                Message.SubjectEncoding = System.Text.Encoding.UTF8;
                Message.BodyEncoding = System.Text.Encoding.UTF8;
                Message.Priority = System.Net.Mail.MailPriority.High;
                Message.IsBodyHtml = true;
                try {
                    client.Send(Message);
                }
                catch (Exception ex) {
                    Console.WriteLine(ex.Message);
                }

                Console.WriteLine("Mail send.");
            }
        }
    }
}
