using System.IO;
using MeteorLauncher.Services;

namespace MeteorLauncher.Models;

/// <summary>One row in the recent-projects list.</summary>
public sealed class RecentProject
{
    public required string ProjectFile { get; init; }
    public required string Name { get; init; }
    public required DateTime LastOpenedUtc { get; init; }
    public required bool Exists { get; init; }

    public string Folder => Path.GetDirectoryName(ProjectFile) ?? ProjectFile;

    public string LastOpenedText => Exists
        ? "Opened " + Relative(DateTime.UtcNow - LastOpenedUtc)
        : "Missing";

    public static RecentProject From(LauncherStore.RecentEntry entry)
    {
        var exists = File.Exists(entry.ProjectFile);
        var name = Path.GetFileNameWithoutExtension(entry.ProjectFile);
        if (exists)
        {
            // Fully qualified: the ProjectFile property hides the ProjectFile type here.
            try { name = MeteorLauncher.Services.ProjectFile.Load(entry.ProjectFile).Name; }
            catch (Exception) { /* unreadable file: fall back to the file name */ }
        }

        return new RecentProject
        {
            ProjectFile   = entry.ProjectFile,
            Name          = name,
            LastOpenedUtc = entry.LastOpenedUtc,
            Exists        = exists,
        };
    }

    private static string Relative(TimeSpan age)
    {
        if (age.TotalMinutes < 1)  return "just now";
        if (age.TotalHours   < 1)  return Plural((int)age.TotalMinutes, "minute") + " ago";
        if (age.TotalDays    < 1)  return Plural((int)age.TotalHours, "hour") + " ago";
        if (age.TotalDays    < 30) return Plural((int)age.TotalDays, "day") + " ago";
        return "on " + DateTime.Now.Subtract(age).ToString("d MMM yyyy");
    }

    private static string Plural(int n, string unit) => $"{n} {unit}{(n == 1 ? "" : "s")}";
}
