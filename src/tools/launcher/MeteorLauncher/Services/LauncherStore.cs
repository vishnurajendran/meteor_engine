using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace MeteorLauncher.Services;

/// <summary>
/// Launcher state that survives restarts: recent projects and the last
/// folder a project was created in. Stored per user at
/// %APPDATA%\MeteorEngine\Launcher\launcher.json.
/// </summary>
public sealed class LauncherStore
{
    public const int MaxRecentProjects = 20;

    public sealed class RecentEntry
    {
        public string ProjectFile { get; set; } = "";
        public DateTime LastOpenedUtc { get; set; }
    }

    private sealed class Data
    {
        public List<RecentEntry> Recent { get; set; } = new();
        public string? LastProjectLocation { get; set; }
    }

    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
    };

    private readonly string storePath;
    private Data data = new();

    public LauncherStore(string? storePath = null)
    {
        this.storePath = storePath ?? Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "MeteorEngine", "Launcher", "launcher.json");
        Load();
    }

    public IReadOnlyList<RecentEntry> Recent => data.Recent;

    public string LastProjectLocation
    {
        get => data.LastProjectLocation ?? Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "MeteorProjects");
        set { data.LastProjectLocation = value; Save(); }
    }

    /// <summary>Moves a project to the top of the recent list (adding it if needed).</summary>
    public void Touch(string projectFile)
    {
        projectFile = Path.GetFullPath(projectFile);
        data.Recent.RemoveAll(e => SamePath(e.ProjectFile, projectFile));
        data.Recent.Insert(0, new RecentEntry { ProjectFile = projectFile, LastOpenedUtc = DateTime.UtcNow });
        if (data.Recent.Count > MaxRecentProjects)
            data.Recent.RemoveRange(MaxRecentProjects, data.Recent.Count - MaxRecentProjects);
        Save();
    }

    public void Remove(string projectFile)
    {
        data.Recent.RemoveAll(e => SamePath(e.ProjectFile, projectFile));
        Save();
    }

    private void Load()
    {
        try
        {
            if (File.Exists(storePath))
                data = JsonSerializer.Deserialize<Data>(File.ReadAllText(storePath), JsonOptions) ?? new Data();
        }
        catch (Exception)
        {
            // A corrupt store should not stop the launcher; start fresh.
            data = new Data();
        }
    }

    private void Save()
    {
        Directory.CreateDirectory(Path.GetDirectoryName(storePath)!);
        File.WriteAllText(storePath, JsonSerializer.Serialize(data, JsonOptions));
    }

    private static bool SamePath(string a, string b) =>
        string.Equals(Path.GetFullPath(a), Path.GetFullPath(b), StringComparison.OrdinalIgnoreCase);
}
