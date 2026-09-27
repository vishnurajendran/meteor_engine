using System.IO;

namespace MeteorLauncher.Services;

/// <summary>
/// Creates a new project folder in the layout described by docs/project_format.md.
///
/// The launcher only creates the folders, the .mtproj and a .gitignore.
/// The editor fills in the rest every time it opens a project
/// (MEditorProjectManager::prepare): it syncs .engine_data/scripting from
/// the engine install and writes .vscode/settings.json if missing. Keeping
/// that logic in one place means engine upgrades reach old projects too.
/// </summary>
public static class ProjectScaffolder
{
    public static readonly string[] AssetSubfolders =
    {
        "assets/scenes",
        "assets/materials",
        "assets/shaders",
        "assets/scripts",
        "assets/textures",
    };

    public static readonly string[] EngineDataFolders =
    {
        ".engine_data/settings",
        ".engine_data/thumbnail_cache",
        ".engine_data/temp",
    };

    private const string GitIgnore =
        "# Meteor editor caches and engine-managed copies (regenerated on open)\n" +
        ".engine_data/thumbnail_cache/\n" +
        ".engine_data/temp/\n" +
        ".engine_data/scripting/\n";

    /// <summary>Returns an error message, or null if a project can be created.</summary>
    public static string? Validate(string parentFolder, string name)
    {
        name = name.Trim();
        if (name.Length == 0)
            return "Enter a project name.";
        if (name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || name.EndsWith('.'))
            return "The project name contains characters that can't be used in a folder name.";
        if (string.IsNullOrWhiteSpace(parentFolder))
            return "Choose a location.";
        if (!Path.IsPathFullyQualified(parentFolder))
            return "The location must be a full path, e.g. D:\\Games.";

        var projectDir = Path.Combine(parentFolder, name);
        if (File.Exists(projectDir))
            return "A file with that name already exists at this location.";
        if (Directory.Exists(projectDir) && Directory.EnumerateFileSystemEntries(projectDir).Any())
            return "A folder with that name already exists and isn't empty.";
        return null;
    }

    /// <summary>Creates the project and returns the path of its .mtproj.</summary>
    public static string Create(string parentFolder, string name, string engineVersion)
    {
        name = name.Trim();
        var error = Validate(parentFolder, name);
        if (error != null)
            throw new InvalidOperationException(error);

        var projectDir = Path.Combine(parentFolder, name);
        Directory.CreateDirectory(projectDir);

        foreach (var folder in AssetSubfolders.Concat(EngineDataFolders))
            Directory.CreateDirectory(Path.Combine(projectDir, folder));

        var gitIgnorePath = Path.Combine(projectDir, ".gitignore");
        if (!File.Exists(gitIgnorePath))
            File.WriteAllText(gitIgnorePath, GitIgnore);

        var projectFilePath = Path.Combine(projectDir, name + ProjectFile.Extension);
        new ProjectFile
        {
            Name          = name,
            Id            = Guid.NewGuid().ToString("B"),
            EngineVersion = engineVersion,
        }.Save(projectFilePath);

        return projectFilePath;
    }
}
