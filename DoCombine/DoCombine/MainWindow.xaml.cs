﻿using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using GongSolutions.Wpf.DragDrop;
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

        private Microsoft.Win32.SaveFileDialog saveFile;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;
            Documents = new ObservableCollection<string>();
            HasDocuments = new ObservablePrimitive<bool>(false);

            Pages = new List<PdfPage>();
            PagesReordered = new ObservablePrimitive<bool>(false);

            saveFile = new Microsoft.Win32.SaveFileDialog();
            saveFile.DefaultExt = ".pdf";
            saveFile.Filter = "PDF files (*.pdf)|*.pdf|All files (*.*)|*.*";
            saveFile.AddExtension = true;
            saveFile.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);

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
                var dataObject = dropInfo.Data as IDataObject;
                if (dataObject != null && dataObject.GetDataPresent(DataFormats.FileDrop))
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
                var dataObject = dropInfo.Data as IDataObject;
                if (dataObject != null && dataObject.GetDataPresent(DataFormats.FileDrop))
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
            PdfDocument exported = new PdfDocument();

            if (Pages.Count == 0)
            {
                foreach (var path in Documents)
                {
                    PdfDocument doc = PdfReader.Open(path, PdfDocumentOpenMode.Import);
                    foreach (var page in doc.Pages)
                    {
                        exported.AddPage(page);
                    }
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
                pages = new List<PdfPage>();
                foreach (var path in Documents)
                {
                    PdfDocument doc = PdfReader.Open(path, PdfDocumentOpenMode.Import);
                    pages.AddRange(doc.Pages);
                }
            }

            PdfPageReorderWindow reorderWindow = new PdfPageReorderWindow(pages);
            if (reorderWindow.ShowDialog().GetValueOrDefault(false))
            {
                if (reorderWindow.Modified)
                {
                    Pages.Clear();
                    reorderWindow.Pages.ToList().ForEach(pwt => { Pages.Add(pwt.Page); });
                    PagesReordered.Object = true;
                }
            }
        }

        private void ResetPages_Click(object sender, RoutedEventArgs e)
        {
            Pages.Clear();
            PagesReordered.Object = false;
        }

        // Event handler for opening a file
        private void MenuItem_Open_Click(object sender, RoutedEventArgs e)
        {
            Wpf.Ui.Controls.MenuItem? menuItem = sender as Wpf.Ui.Controls.MenuItem;
            if (menuItem != null)
            {
                string? document = menuItem.DataContext as string;
                if (document != null)
                {
                    ProcessStartInfo startInfo = new ProcessStartInfo
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
            Wpf.Ui.Controls.MenuItem? menuItem = sender as Wpf.Ui.Controls.MenuItem;
            if (menuItem != null)
            {
                string? document = menuItem.DataContext as string;
                if (document != null)
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
    }
}