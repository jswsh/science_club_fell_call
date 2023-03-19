using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Text;

namespace ConsolCommListener.Biz
{
    class ListenerBiz
    {
        /// <summary>
        /// 分析处理消息
        /// </summary>
        /// <param name="sSerialRead"></param>
        internal static List<Entities.EntityCmd> GetCmd(string sSerialRead) {
            //todo 将收到的消息体进行反序列化并分析处理
            List<Entities.EntityCmd> lstCmd = new List<Entities.EntityCmd>();

            string[] lines = sSerialRead.Split('\n');

            if (lines.Length > 0) {
                for (int i = 0; i < lines.Length; i++) {
                    string s = lines[i];
                    if (s.Trim().Length == 0)
                        continue;

                    Entities.EntityCmd eCmd = new Entities.EntityCmd();
                    try {
                        eCmd = JsonConvert.DeserializeObject<Entities.EntityCmd>(s);
                        lstCmd.Add(eCmd);
                    }
                    catch (Exception ex){
                        Console.WriteLine($"[{s}]");
                        continue;
                    }
                }
            }
            Console.WriteLine("----------------------------------");
            Console.WriteLine(JsonConvert.SerializeObject(lstCmd));
            Console.WriteLine("----------------------------------");
            return lstCmd;
        }

        
    }
}
