using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace GMSL;

public static class Cache
{

    private static Dictionary<string, string> _hashes = [];

    public static void AddFileToCache(string path)
    {
        if (!File.Exists(path)) return;
        _hashes.Remove(path);
        _hashes.Add(path, GetHashForFile(path));
    }

    public static void SaveToFile()
    {
        var cachePath = Path.Combine(Environment.CurrentDirectory, "gmsl", "cache", "cache.json");
        File.WriteAllText(cachePath, JsonSerializer.Serialize(_hashes));
    }

    public static bool IsCacheOutdated()
    {
        var cachePath = Path.Combine(Environment.CurrentDirectory, "gmsl", "cache", "cache.json");
        if (!File.Exists(cachePath)) return true;

        _hashes = JsonSerializer.Deserialize<Dictionary<string, string>>(File.ReadAllText(cachePath));

        foreach (var (path, hash) in _hashes)
        {
            if (!File.Exists(path)) return true;
            if (GetHashForFile(path) != hash) return true;
        }

        return false;
    }

    private static string GetHashForFile(string path)
    {
        using SHA256 sha256 = SHA256.Create();
        using FileStream fs = File.OpenRead(path);

        return BitConverter.ToString(sha256.ComputeHash(fs)).Replace("-", "").ToLowerInvariant();
    }
}