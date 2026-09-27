using System.Collections.ObjectModel;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using MeteorLauncher.Models;
using MeteorLauncher.Services;
using Microsoft.Win32;

namespace MeteorLauncher;

public partial class MainWindow : Window
{
    private readonly LauncherStore store = new();
    private readonly ObservableCollection<RecentProject> recent = new();

    public MainWindow()
    {
        InitializeComponent();

        RecentList.ItemsSource = recent;
        VersionText.Text = "Version " + App.Version;
        EditorPathText.Text = EditorProcess.EditorPath;
        EditorPathText.ToolTip = "Editor: " + EditorProcess.EditorPath;

        RefreshRecent();
    }

    // -- Recent list ---------------------------------------------------------

    private RecentProject? Selected => RecentList.SelectedItem as RecentProject;

    private void RefreshRecent()
    {
        var previouslySelected = Selected?.ProjectFile;

        recent.Clear();
        foreach (var entry in store.Recent)
            recent.Add(RecentProject.From(entry));

        EmptyText.Visibility = recent.Count == 0 ? Visibility.Visible : Visibility.Collapsed;

        RecentList.SelectedItem =
            recent.FirstOrDefault(p => p.ProjectFile == previouslySelected) ??
            recent.FirstOrDefault(p => p.Exists);
        UpdateLaunchButton();
    }

    private void UpdateLaunchButton() => LaunchButton.IsEnabled = Selected is { Exists: true };

    private void RecentList_SelectionChanged(object sender, SelectionChangedEventArgs e) => UpdateLaunchButton();

    private void RecentList_MouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        // Only react to double-clicks on an item, not on empty space.
        if (e.OriginalSource is DependencyObject source &&
            ItemsControl.ContainerFromElement(RecentList, source) is ListBoxItem &&
            Selected != null)
        {
            OpenRecent(Selected);
        }
    }

    private void RecentList_KeyDown(object sender, KeyEventArgs e)
    {
        if (Selected == null) return;

        if (e.Key == Key.Enter)
        {
            OpenRecent(Selected);
            e.Handled = true;
        }
        else if (e.Key == Key.Delete)
        {
            RemoveFromList(Selected);
            e.Handled = true;
        }
    }

    private void OpenRecent(RecentProject project)
    {
        if (!project.Exists)
        {
            var answer = MessageBox.Show(this,
                $"This project can't be found:\n{project.ProjectFile}\n\nRemove it from the list?",
                "Project missing", MessageBoxButton.YesNo, MessageBoxImage.Warning);
            if (answer == MessageBoxResult.Yes)
                RemoveFromList(project);
            return;
        }

        OpenProject(project.ProjectFile);
    }

    private void RemoveFromList(RecentProject project)
    {
        store.Remove(project.ProjectFile);
        RefreshRecent();
    }

    // -- Context menu ----------------------------------------------------------

    private void RecentList_ContextMenuOpening(object sender, ContextMenuEventArgs e)
    {
        // Nothing to act on (empty list or click on empty space with no selection).
        if (Selected == null)
            e.Handled = true;
    }

    private void ContextOpen_Click(object sender, RoutedEventArgs e)
    {
        if (Selected is { } project)
            OpenRecent(project);
    }

    private void ContextShowInExplorer_Click(object sender, RoutedEventArgs e)
    {
        if (Selected is { } project)
            EditorProcess.ShowInExplorer(project.Folder);
    }

    private void ContextRemove_Click(object sender, RoutedEventArgs e)
    {
        if (Selected is { } project)
            RemoveFromList(project);
    }

    // -- Actions ---------------------------------------------------------------

    private void Launch_Click(object sender, RoutedEventArgs e)
    {
        if (Selected != null)
            OpenRecent(Selected);
    }

    private void NewProject_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new NewProjectWindow(store) { Owner = this };
        if (dialog.ShowDialog() == true && dialog.CreatedProjectFile != null)
            OpenProject(dialog.CreatedProjectFile);
    }

    private void OpenExisting_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog
        {
            Title  = "Open Meteor project",
            Filter = $"Meteor project (*{ProjectFile.Extension})|*{ProjectFile.Extension}",
            InitialDirectory = Directory.Exists(store.LastProjectLocation) ? store.LastProjectLocation : "",
        };
        if (dialog.ShowDialog(this) != true)
            return;

        try
        {
            // Reject files that aren't projects before handing them to the editor.
            ProjectFile.Load(dialog.FileName);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Not a Meteor project", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        OpenProject(dialog.FileName);
    }

    /// <summary>Starts the editor for the project and closes the launcher.</summary>
    private void OpenProject(string projectFile)
    {
        try
        {
            EditorProcess.Launch(projectFile);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Couldn't open project", MessageBoxButton.OK, MessageBoxImage.Error);
            RefreshRecent();
            return;
        }

        store.Touch(projectFile);
        Application.Current.Shutdown();
    }
}
