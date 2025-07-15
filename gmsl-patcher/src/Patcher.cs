using System.Reflection;
using System.Runtime.InteropServices;
using GMSL;
using UndertaleModLib;

namespace GMSLPatcher;

public class Patcher
{
    [UnmanagedCallersOnly(EntryPoint = "PatchData")]
    public static void PatchData(IntPtr pathPtr)
    {
        try
        {
            string path = Marshal.PtrToStringUni(pathPtr);

            var fs = File.OpenRead(path);
            UndertaleData data = UndertaleIO.Read(fs, (message, _) =>
            {
                Console.WriteLine(message);
            }, Console.WriteLine);
            fs.Dispose();

            AppDomain.CurrentDomain.AssemblyResolve += (sender, args) => AppDomain
                .CurrentDomain.GetAssemblies()
                .FirstOrDefault(assembly => assembly.FullName == args.Name);

            foreach (var dir in Directory.GetDirectories(Path.Combine(Path.GetDirectoryName(path), "gmsl", "mods")))
            {
                var modPath = Path.Combine(dir, $"{Path.GetFileName(dir)}.dll");
                if (!File.Exists(modPath))
                {
                    Console.WriteLine($"{modPath} doesn't exist... skipping...");
                    continue;
                }

                var assembly = Assembly.LoadFile(modPath);

                foreach (var type in assembly.GetTypes())
                {
                    if (type.GetInterfaces().Contains(typeof(IGMSLMod)))
                    {
                        var instance = (IGMSLMod)Activator.CreateInstance(type)!;
                        Environment.CurrentDirectory = dir;
                        instance.Load(data);
                    }
                }
            }

            fs = File.Open(Path.Combine(Path.GetDirectoryName(path), "gmsl", "patcher", "cache.win"), FileMode.Create, FileAccess.Write);
            UndertaleIO.Write(fs, data);
            fs.Dispose();

            Environment.CurrentDirectory = Path.GetDirectoryName(path);
        }
        catch (Exception e)
        {
            Console.WriteLine(e);
        }
    }
}