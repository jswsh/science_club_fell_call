using System;
using System.Collections.Generic;
using System.Text;

namespace ConsolCommListener.Entities
{
    class EntityCmd
    {/// <summary>
     /// 
     /// </summary>
        public int id { get; set; }

        /// <summary>
        /// 
        /// </summary>
        public string signal { get; set; }

        private int id2 { get; set; }

        public void SetId2(int i) {
            id2 = i;
        }
    }
}
