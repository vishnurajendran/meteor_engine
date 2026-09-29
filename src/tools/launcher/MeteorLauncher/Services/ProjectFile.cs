using System.IO;
using System.Xml.Linq;

namespace MeteorLauncher.Services;

/// <summary>
/// Reads and writes &lt;name&gt;.mtproj. Must stay in sync with the engine's
/// SProjectDescriptor (src/core/project/project_descriptor.h) and
/// docs/project_format.md.
/// </summary>
public sealed class ProjectFile
{
    public const string Extension = ".mtproj";
    public const int CurrentFormatVersion = 1;

    public int FormatVersion { get; set; } = CurrentFormatVersion;
    public string Name { get; set; } = "";
    public string Id { get; set; } = "";
    public string EngineVersion { get; set; } = "";
    public List<string> AssetFolders { get; set; } = new() { "assets/" };
    public string StartupScene { get; set; } = "";

    /// <summary>Loads a project file. Missing values keep their defaults.</summary>
    public static ProjectFile Load(string path)
    {
        var doc = XDocument.Load(path);
        var root = doc.Element("project")
                   ?? throw new InvalidDataException($"'{path}' is not a Meteor project file (no <project> element).");

        var project = new ProjectFile
        {
            FormatVersion = (int?)root.Attribute("formatVersion") ?? CurrentFormatVersion,
            Name          = (string?)root.Attribute("name") ?? Path.GetFileNameWithoutExtension(path),
            Id            = (string?)root.Attribute("id") ?? "",
            EngineVersion = (string?)root.Attribute("engineVersion") ?? "",
            StartupScene  = (string?)root.Element("startupScene")?.Attribute("path") ?? "",
        };

        var folders = root.Element("assetFolders")?
            .Elements("folder")
            .Select(f => (string?)f.Attribute("path") ?? "")
            .Where(p => p.Length > 0)
            .Select(p => p.EndsWith('/') ? p : p + "/")
            .ToList();
        if (folders is { Count: > 0 })
            project.AssetFolders = folders;

        return project;
    }

    public void Save(string path)
    {
        var root = new XElement("project",
            new XAttribute("formatVersion", FormatVersion),
            new XAttribute("name", Name),
            new XAttribute("id", Id),
            new XAttribute("engineVersion", EngineVersion),
            new XElement("assetFolders",
                AssetFolders.Select(f => new XElement("folder", new XAttribute("path", f)))));

        if (!string.IsNullOrEmpty(StartupScene))
            root.Add(new XElement("startupScene", new XAttribute("path", StartupScene)));

        new XDocument(new XDeclaration("1.0", null, null), root).Save(path);
    }
}
