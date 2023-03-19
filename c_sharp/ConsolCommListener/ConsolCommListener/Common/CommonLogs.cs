using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace ConsolCommListener.Common
{
    class CommonLogs
    {
        internal static void SaveLogs(string sLogFilePath1, string sLog, bool bIsNewLine = false) {

            string sFilePath = sLogFilePath1 + @".txt";
            // This text is added only once to the file.
            if (!File.Exists(sFilePath)) {
                // Create a file to write to.
                using (StreamWriter sw = File.CreateText(sFilePath)) {
                }
            }
            using (StreamWriter sw = File.AppendText(sFilePath)) {
                if (!bIsNewLine)
                    sw.Write(sLog);
                else
                    sw.WriteLine(sLog);
            }
        }
    }
}
