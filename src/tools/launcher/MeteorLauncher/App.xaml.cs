using System.Reflection;
using System.Windows;
using MeteorLauncher.Services;

namespace MeteorLauncher;

/// <summary>
/// Command line:
///   MeteorLauncher.exe [--editor &lt;path to Meteorite.exe&gt;]
///
/// --editor is only needed when the launcher does not sit next to the editor
/// (e.g. when run from the IDE). CMake copies both into bin/.
/// </summary>
public partial class App : Application
{
    /// <summary>"2026.1.0-Beta" - written into new projects and shown in the sidebar.</summary>
    public static string Version { get; } =
        Assembly.GetExecutingAssembly()
                .GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion
        ?? Assembly.GetExecutingAssembly().GetName().Version?.ToString()
        ?? "dev";

    protected override void OnStartup(StartupEventArgs e)
    {
        for (var i = 0; i < e.Args.Length; ++i)
        {
            if (e.Args[i] == "--editor" && i + 1 < e.Args.Length)
                EditorProcess.EditorPathOverride = e.Args[++i];
        }

        base.OnStartup(e);
    }
}
