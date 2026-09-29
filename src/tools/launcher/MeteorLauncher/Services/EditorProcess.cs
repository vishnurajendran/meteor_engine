using System.Diagnostics;
using System.IO;

namespace MeteorLauncher.Services;

/// <summary>
/// Starts the editor for a project: working directory = project folder,
/// arguments = --p &lt;file&gt;.mtproj (see docs/project_format.md).
/// </summary>
public static class EditorProcess
{
    public const string EditorExecutableName = "Meteorite.exe";

    /// <summary>Set from the --editor command-line argument.</summary>
    public static string? EditorPathOverride { get; set; }

    /// <summary>
    /// The editor executable: --editor argument, else the METEOR_EDITOR_PATH
    /// environment variable, else Meteorite.exe next to the launcher (the
    /// normal case: CMake copies both into bin/).
    /// </summary>
    public static string EditorPath
    {
        get
        {
            if (!string.IsNullOrWhiteSpace(EditorPathOverride))
                return Path.GetFullPath(EditorPathOverride);

            var fromEnv = Environment.GetEnvironmentVariable("METEOR_EDITOR_PATH");
            if (!string.IsNullOrWhiteSpace(fromEnv))
                return Path.GetFullPath(fromEnv);

            return Path.Combine(AppContext.BaseDirectory, EditorExecutableName);
        }
    }

    /// <summary>Folder of the engine install (where the editor lives).</summary>
    public static string EngineDirectory => Path.GetDirectoryName(EditorPath) ?? AppContext.BaseDirectory;

    /// <summary>Starts the editor. Throws with a readable message on failure.</summary>
    public static void Launch(string projectFilePath)
    {
        projectFilePath = Path.GetFullPath(projectFilePath);
        if (!File.Exists(projectFilePath))
            throw new FileNotFoundException($"The project file no longer exists:\n{projectFilePath}");

        var editor = EditorPath;
        if (!File.Exists(editor))
            throw new FileNotFoundException(
                $"Couldn't find the editor at:\n{editor}\n\n" +
                "Build the editor, or start the launcher with --editor <path to Meteorite.exe>.");

        var startInfo = new ProcessStartInfo(editor)
        {
            WorkingDirectory = Path.GetDirectoryName(projectFilePath)!,
            UseShellExecute  = false,
        };
        startInfo.ArgumentList.Add("--p");
        startInfo.ArgumentList.Add(Path.GetFileName(projectFilePath));

        using var process = Process.Start(startInfo)
            ?? throw new InvalidOperationException("The editor process could not be started.");
    }

    /// <summary>Opens a folder in Explorer.</summary>
    public static void ShowInExplorer(string folder)
    {
        if (Directory.Exists(folder))
            Process.Start(new ProcessStartInfo("explorer.exe") { ArgumentList = { folder }, UseShellExecute = false });
    }
}
