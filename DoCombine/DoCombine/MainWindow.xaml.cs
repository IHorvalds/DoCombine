using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Windows;
using GongSolutions.Wpf.DragDrop;
using Microsoft.Win32;
using PdfSharp.Pdf;
using PdfSharp.Pdf.IO;
using Wpf.Ui.Controls;

namespace DoCombine
{
    public partial class MainWindow : FluentWindow, IDropTarget
    {
        public ObservableCollection<string> Documents { get; set; }
        public ObservablePrimitive<bool> HasDocuments { get; set; }

        private List<PdfPage> Pages { get; set; }
        public ObservablePrimitive<bool> PagesReordered { get; set; }

        readonly private SaveFileDialog saveFile;

        readonly static private Guid ShortcutMenuHandlerCLSID = new("73b668a5-0434-4983-bb8a-8fab7c728e64");
        readonly static string CLSIDKeyPath = $@"Software\Classes\CLSID\{ShortcutMenuHandlerCLSID.ToString("B")}";
        readonly static string FileAssocKeyPath = $@"Software\Classes\SystemFileAssociations\.pdf\ShellEx\ContextMenuHandlers\DoCombineExt";

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;
            Documents = [];
            HasDocuments = new ObservablePrimitive<bool>(false);

            Pages = [];
            PagesReordered = new ObservablePrimitive<bool>(false);

            saveFile = new SaveFileDialog
            {
                DefaultExt = ".pdf",
                Filter = "PDF files (*.pdf)|*.pdf|All files (*.*)|*.*",
                AddExtension = true,
                InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)
            };

            SetShortcutMenuItemRegisteredHeader(IsShortcutMenuHandlerRegistered());
            string[] args = Environment.GetCommandLineArgs();
            if (args.Length > 1)
            {
                bool gotPdfDoc = false;
                for (int i = 1; i < args.Length; i++)
                {
                    if (File.Exists(args[i]) && PdfReader.TestPdfFile(args[i]) != 0)
                    {
                        Documents.Add(args[i]);
                        gotPdfDoc = true;
                    }
                }

                if (gotPdfDoc)
                {
                    HasDocuments.Object = true;
                    InstructionLabel.Visibility = Visibility.Hidden; // I don't want to write another ValueConverter just for this
                }
            }
        }

        void IDropTarget.DragOver(IDropInfo dropInfo)
        {
            // New thing(s) dragged
            if (dropInfo.Data is IDataObject)
            {
                if (dropInfo.Data is IDataObject dataObject && dataObject.GetDataPresent(DataFormats.FileDrop))
                {
                    string[] files = dataObject.GetData(DataFormats.FileDrop) as string[] ?? [];
                    foreach (var file in files)
                    {
                        if (PdfReader.TestPdfFile(file) != 0)
                        {
                            dropInfo.DropTargetAdorner = DropTargetAdorners.Highlight;
                            dropInfo.Effects = DragDropEffects.Copy;
                        }
                    }
                }
            }
            else
            {
                dropInfo.Effects = DragDropEffects.Move;
            }
        }

        void IDropTarget.Drop(IDropInfo dropInfo)
        {
            bool modified = false;
            // New thing(s) dropped
            if (dropInfo.Data is IDataObject)
            {
                if (dropInfo.Data is IDataObject dataObject && dataObject.GetDataPresent(DataFormats.FileDrop))
                {
                    string[] files = dataObject.GetData(DataFormats.FileDrop) as string[] ?? [];
                    foreach (var file in files)
                    {
                        if (PdfReader.TestPdfFile(file) != 0)
                        {
                            Documents.Add(file);
                            modified = true;
                        }

                    }
                }
            }
            else
            {
                GongSolutions.Wpf.DragDrop.DragDrop.DefaultDropHandler.Drop(dropInfo);
                modified = true;
            }

            if (modified)
            {
                HasDocuments.Object = true;
                InstructionLabel.Visibility = Visibility.Hidden;
                Pages.Clear();
                PagesReordered.Object = false;
            }
        }

        private void Export_Click(object sender, RoutedEventArgs e)
        {
            PdfDocument exported = new();

            if (Pages.Count == 0)
            {
                foreach (var path in Documents)
                {
                    PdfDocument doc = PdfReader.Open(path, PdfDocumentOpenMode.Import);
                    foreach (var page in doc.Pages)
                    {
                        exported.AddPage(page);
                    }
                    doc.Dispose();
                }
            }
            else
            {
                Pages.ForEach(page => exported.Pages.Add(page));
            }

            if (saveFile.ShowDialog().GetValueOrDefault(false))
            {
                exported.Info.Author = $"Combined by {System.Security.Principal.WindowsIdentity.GetCurrent().Name}";
                exported.Info.CreationDate = DateTime.Now;
                exported.Save(saveFile.FileName);
            }
        }

        private void Reorder_Click(object sender, RoutedEventArgs e)
        {
            List<PdfPage> pages;
            if (Pages.Count != 0)
            {
                pages = Pages;
            }
            else
            {
                pages = [];
                foreach (var path in Documents)
                {
                    PdfDocument doc = PdfReader.Open(path, PdfDocumentOpenMode.Import);
                    pages.AddRange(doc.Pages);
                    doc.Dispose();
                }
            }

            PdfPageReorderWindow reorderWindow = new(pages);
            if (reorderWindow.ShowDialog().GetValueOrDefault(false))
            {
                if (reorderWindow.Modified)
                {
                    Pages.Clear();
                    reorderWindow.Thumbnails.ToList().ForEach(pwt => { Pages.Add(pages[pwt.Index - 1]); });
                    PagesReordered.Object = true;
                }
            }
            pages.Clear();
            ((IDisposable)reorderWindow).Dispose();
            GC.Collect(GC.MaxGeneration);
            GC.WaitForPendingFinalizers();
        }

        private void ResetPages_Click(object sender, RoutedEventArgs e)
        {
            Pages.Clear();
            PagesReordered.Object = false;
        }

        // Event handler for opening a file
        private void MenuItem_Open_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem menuItem)
            {
                if (menuItem.DataContext is string document)
                {
                    ProcessStartInfo startInfo = new()
                    {
                        FileName = document,
                        UseShellExecute = true
                    };

                    Process.Start(startInfo);
                }
            }
        }

        // Event handler for deleting a file
        private void MenuItem_Delete_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem menuItem)
            {
                if (menuItem.DataContext is string document)
                {
                    Documents.Remove(document);
                    if (Documents.Count == 0)
                    {
                        HasDocuments.Object = false;
                        InstructionLabel.Visibility = Visibility.Visible;
                    }
                }
            }
        }

        // Shortcut Menu Handler stuff
        private void ShortcutHandlerMenuItem_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem)
            {
                if (IsShortcutMenuHandlerRegistered())
                {
                    UnregisterShortcutMenuHandler();
                    SetShortcutMenuItemRegisteredHeader(false);
                }
                else
                {
                    RegisterShortcutMenuHandler();
                    SetShortcutMenuItemRegisteredHeader(true);
                }
            }
        }

        private static bool IsShortcutMenuHandlerRegistered()
        {
            bool registered = false;

            using (RegistryKey? CLSIDKey = Registry.CurrentUser.OpenSubKey(CLSIDKeyPath + "\\InprocServer32", true))
            using (RegistryKey? FileAssocKey = Registry.CurrentUser.OpenSubKey(FileAssocKeyPath, true))
            {
                registered = CLSIDKey is not null && FileAssocKey is not null;
            }

            return registered;
        }

        private static void RegisterShortcutMenuHandler()
        {
            using (var CLSIDKey = Registry.CurrentUser.CreateSubKey(CLSIDKeyPath + "\\InprocServer32", true))
            {
                CLSIDKey.SetValue("", $"{Path.GetDirectoryName(Assembly.GetEntryAssembly()!.Location)}\\ShellExt\\DoCombineShortcutMenu.dll");
                CLSIDKey.SetValue("ThreadingModel", "Apartment");
            }

            using var FileAssocKey = Registry.CurrentUser.CreateSubKey(FileAssocKeyPath, true);
            FileAssocKey.SetValue("", ShortcutMenuHandlerCLSID.ToString("B"));
        }

        private static void UnregisterShortcutMenuHandler()
        {
            Registry.CurrentUser.DeleteSubKeyTree(CLSIDKeyPath, false);
            Registry.CurrentUser.DeleteSubKeyTree(FileAssocKeyPath, false);
        }

        private void SetShortcutMenuItemRegisteredHeader(bool isRegistered)
        {
            // Is registered, the button will be to unregister it.
            if (isRegistered)
            {
                ShortcutHandlerMenuItem.Header = "Remove DoCombine from right-click menu";
            }
            else
            {
                ShortcutHandlerMenuItem.Header = "Add DoCombine to right-click menu";
            }
        }
    }
}