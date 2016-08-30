using System;
using System.Runtime.InteropServices;
using System.Text;

namespace EmbedTest
{
  class MainClass
  {
    public static void Main(string[] args)
    {
      Console.WriteLine("Hello World!");
      var sn = SystemSerialNumber ();
      Console.WriteLine ("System Serialn Number: {0}", sn);
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

    static string SystemSerialNumber ()
    {
      // Try to get the serial number by shell executing the Mac ioreg utility
      var result = MacSerialNumberUsingIoreg ();
      if (string.IsNullOrEmpty (result)) {
        // The ioreg utility faild to return a value so fall back on machine name
        result = LastChanceAtSerialNumber ();
      }
      if (string.IsNullOrWhiteSpace (result)) {
        System.Diagnostics.Trace.WriteLine ("Unable to determine the system serial number and computer or user name");
        throw new Exception ("Unable to determine the system serial number and computer or user name");
      }
      return result;
    }

    /// <summary>
    /// If all else fails get the computer or user name
    /// </summary>
    /// <returns>The chance at serial number.</returns>
    private static string LastChanceAtSerialNumber ()
    {
      var result = Environment.MachineName;
      // If machine name is blank then fall back on user name
      if (string.IsNullOrWhiteSpace (result))
        result = Environment.UserName;
      return result;
    }

    /// <summary>
    /// Get the serial number using ioreg.
    /// </summary>
    /// <returns>The serial number using ioreg.</returns>
    private static string MacSerialNumberUsingIoreg ()
    {
      try {
        using (var process = new System.Diagnostics.Process ()) {
          // http://mcneel.myjetbrains.com/youtrack/issue/MR-1910
          // See comment from Dale Fugier
          //   ioreg -rd1 -c IOPlatformExpertDevice
          // Using the -rd1 might make the output a lot more managable but
          // we are to chicken to change it at this point in the process.
          process.StartInfo = new System.Diagnostics.ProcessStartInfo ("/usr/sbin/ioreg", "-c IOPlatformExpertDevice");
          process.StartInfo.UseShellExecute = false;
          process.StartInfo.RedirectStandardOutput = true;
          process.Start ();

          //
          // Read in all the text from the process with the StreamReader.
          //
          var string_builder = new StringBuilder ();
          var platform_serial_number = string.Empty;
          var board_id = string.Empty;
          var io_platform_id = string.Empty;
          var length = 0;
          using (var reader = process.StandardOutput) {
            while (string.IsNullOrWhiteSpace (platform_serial_number) && reader.Peek () >= 0) {
              var line = reader.ReadLine ();
              if (string.IsNullOrEmpty (line))
                continue;
              length += line.Length;
              platform_serial_number = ExtractKeyValueString (line, "\"IOPlatformSerialNumber\"");
              if (string.IsNullOrEmpty (board_id))
                board_id = ExtractKeyValueString (line, "\"board-id\"");
              if (string.IsNullOrEmpty (io_platform_id))
                io_platform_id = ExtractKeyValueString (line, "\"IOPlatformUUID\"");
              if (length < 3000)
                string_builder.AppendLine (line);
            }
          }
          process.WaitForExit ();
          var serial_number = platform_serial_number;
          if (string.IsNullOrEmpty (serial_number))
            serial_number = string.IsNullOrEmpty (board_id) ? io_platform_id : board_id;
          if (string.IsNullOrWhiteSpace (serial_number)) {
            System.Diagnostics.Trace.WriteLine ("SystemSerialNumber not found in the following:");
            System.Diagnostics.Trace.WriteLine (string_builder.ToString ());
          }
          return serial_number;
        }
      } catch (Exception) {
        return null;
      }
    }

    static string ExtractKeyValueString (string stream, string key)
    {
      var index = stream.IndexOf (key, StringComparison.Ordinal);
      if (index < 0)
        return string.Empty;
      var open_quote = stream.IndexOf ('\"', index + key.Length);
      if (open_quote < 0)
        return string.Empty;
      var end_quote = stream.IndexOf ('\"', open_quote + 1);
      if (end_quote < 0)
        return string.Empty;
      var result = stream.Substring (open_quote + 1, end_quote - open_quote - 1);
      return result;
    }
  }


}
