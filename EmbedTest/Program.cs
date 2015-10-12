using System;
using System.Runtime.InteropServices;

namespace EmbedTest
{
  class MainClass
  {
    public static void Main(string[] args)
    {
      Console.WriteLine("Hello World!");
    }

    public static void CallbackTest()
    {
      CallbackA(1,2.5);
      CallbackB("yo");
    }


    [DllImport("__Internal", CallingConvention=CallingConvention.Cdecl )]
    internal static extern void CallbackA(int i, double d);

    [DllImport("__Internal", CallingConvention=CallingConvention.Cdecl )]
    internal static extern void CallbackB([MarshalAs(UnmanagedType.LPWStr)]string s);

  }
}
