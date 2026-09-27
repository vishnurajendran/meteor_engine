using System.IO;
using System.Windows;
using System.Windows.Controls;
using MeteorLauncher.Services;
using Microsoft.Win32;

namespace MeteorLauncher;

public partial class NewProjectWindow : Window
{
    private readonly LauncherStore store;

    /// <summary>Path of the created .mtproj when the dialog returns true.</summary>
    public string? CreatedProjectFile { get; private set; }

    public NewProjectWindow(LauncherStore store)
    {
        this.store = store;
        InitializeComponent();

        NameBox.Text = "MyGame";
        LocationBox.Text = store.LastProjectLocation;

        Loaded += (_, _) =>
        {
            NameBox.Focus();
            NameBox.SelectAll();
        };
        UpdateState();
    }

    private void Input_TextChanged(object sender, TextChangedEventArgs e) => UpdateState();

    private void UpdateState()
    {
        // Called from TextChanged during InitializeComponent, before all controls exist.
        if (NameBox == null || LocationBox == null || PreviewText == null || CreateButton == null)
            return;

        var name = NameBox.Text.Trim();
        var location = LocationBox.Text.Trim();
        var error = ProjectScaffolder.Validate(location, name);

        PreviewText.Text = name.Length > 0 && location.Length > 0
            ? "Will create: " + Path.Combine(location, name, name + ProjectFile.Extension)
            : "";

        ShowError(error);
        CreateButton.IsEnabled = error == null;
    }

    private void ShowError(string? message)
    {
        ErrorText.Text = message ?? "";
        ErrorText.Visibility = string.IsNullOrEmpty(message) ? Visibility.Collapsed : Visibility.Visible;
    }

    private void Browse_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFolderDialog
        {
            Title = "Choose where to create the project",
            InitialDirectory = Directory.Exists(LocationBox.Text) ? LocationBox.Text : "",
        };
        if (dialog.ShowDialog(this) == true)
            LocationBox.Text = dialog.FolderName;
    }

    private void Create_Click(object sender, RoutedEventArgs e)
    {
        var name = NameBox.Text.Trim();
        var location = LocationBox.Text.Trim();

        try
        {
            CreatedProjectFile = ProjectScaffolder.Create(location, name, App.Version);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
            return;
        }

        store.LastProjectLocation = location;
        DialogResult = true;
    }
}
