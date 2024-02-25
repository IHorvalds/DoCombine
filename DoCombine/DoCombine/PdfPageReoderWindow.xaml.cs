using Docnet.Core;
using Docnet.Core.Converters;
using Docnet.Core.Models;
using Docnet.Core.Readers;
using GongSolutions.Wpf.DragDrop;
using PdfSharp.Pdf;
using System.Collections.ObjectModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media.Imaging;
using Wpf.Ui.Controls;

namespace DoCombine
{
    public class IndexToTextConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            DependencyObject? depObj = value as DependencyObject;
            if (depObj != null)
            {
                ItemsControl itemsControl = ItemsControl.ItemsControlFromItemContainer(depObj);
                if (itemsControl != null)
                {
                    int index = itemsControl.ItemContainerGenerator.IndexFromContainer(depObj);
                    return $"{index + 1}";
                }
            }
            return string.Empty;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotSupportedException();
        }
    }

    public sealed class PageWithThumbnail
    {
        public PdfPage Page;
        private WeakReference _weakReference = new WeakReference(null);

        public PageWithThumbnail(ref PdfPage page)
        {
            Page = page;
        }

        public object SlowThumbnail
        {
            get
            {
                return _weakReference.Target ?? (_weakReference.Target = GetThumbnail());
            }
        }

        public object FastThumbnail
        {
            get
            {
                return _weakReference.Target ?? DependencyProperty.UnsetValue;
            }
        }

        private BitmapImage GetThumbnail()
        {
            Bitmap bmp;
            using (MemoryStream stream = new MemoryStream())
            {
                PdfDocument tmp = new PdfDocument();
                tmp.AddPage(Page);
                tmp.Save(stream);
                IDocReader docReader =
                    DocLib.Instance.GetDocReader(stream.ToArray(), new PageDimensions(PdfPageReorderWindow.ITEM_MAX_SIZE, PdfPageReorderWindow.ITEM_MAX_SIZE));
                IPageReader pageReader = docReader.GetPageReader(0);

                var width = pageReader.GetPageWidth();
                var height = pageReader.GetPageHeight();
                var rawBytes = pageReader.GetImage(new NaiveTransparencyRemover(0xFF, 0xFF, 0xFF));

                bmp = new Bitmap(width, height, PixelFormat.Format32bppRgb);

                AddBytes(bmp, rawBytes);
            }
            using (MemoryStream imgStream = new MemoryStream())
            {
                bmp.Save(imgStream, ImageFormat.Png);
                imgStream.Position = 0;

                var bitmapImage = new BitmapImage();
                bitmapImage.BeginInit();
                bitmapImage.StreamSource = imgStream;
                bitmapImage.CacheOption = BitmapCacheOption.OnLoad;
                bitmapImage.EndInit();
                bitmapImage.Freeze();

                return bitmapImage;
            }
        }

        private static void AddBytes(Bitmap bmp, byte[] rawBytes)
        {
            var rect = new Rectangle(0, 0, bmp.Width, bmp.Height);

            var bmpData = bmp.LockBits(rect, ImageLockMode.WriteOnly, bmp.PixelFormat);
            var pNative = bmpData.Scan0;

            Marshal.Copy(rawBytes, 0, pNative, rawBytes.Length);
            bmp.UnlockBits(bmpData);
        }

    }

    public partial class PdfPageReorderWindow : FluentWindow, IDropTarget, IDisposable
    {
        public const int ITEM_MIN_SIZE = 100;
        public const int ITEM_MAX_SIZE = 200;
        public ObservableCollection<PageWithThumbnail> Pages { get; set; }
        private List<PdfPage> InitialPages = [];
        public ObservablePrimitive<bool> Modified { get; set; }
        private DocLib docLib { get; }

        public PdfPageReorderWindow(List<PdfPage> initialPages)
        {
            InitializeComponent();
            DataContext = this;
            Pages = new ObservableCollection<PageWithThumbnail>();
            InitialPages = initialPages;
            InitialPages.ForEach(page => Pages.Add(new PageWithThumbnail(ref page)));
            Modified = new ObservablePrimitive<bool>(false);
            docLib = DocLib.Instance;
        }

        void IDisposable.Dispose()
        {
            docLib.Dispose();
        }

        private void OkButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
            Close();
        }

        private void ResetButton_Click(object sender, RoutedEventArgs e)
        {
            Modified.Object = false;
            Pages.Clear();
            InitialPages.ForEach(page => Pages.Add(new PageWithThumbnail(ref page)));
        }

        private void PageDelete_Click(object sender, RoutedEventArgs e)
        {
            Modified.Object = true;
            Wpf.Ui.Controls.MenuItem? menuItem = sender as Wpf.Ui.Controls.MenuItem;
            if (menuItem != null)
            {
                PageWithThumbnail? page = menuItem.DataContext as PageWithThumbnail;
                if (page != null)
                {
                    Pages.Remove(page);
                }
            }
        }

        void IDropTarget.DragOver(IDropInfo dropInfo)
        {
            GongSolutions.Wpf.DragDrop.DragDrop.DefaultDropHandler.DragOver(dropInfo);
        }

        void IDropTarget.Drop(IDropInfo dropInfo)
        {
            if (dropInfo.Data is PageWithThumbnail[] || dropInfo.InsertIndex != dropInfo.DragInfo.SourceIndex)
            {
                Modified.Object = true;
            }
            GongSolutions.Wpf.DragDrop.DragDrop.DefaultDropHandler.Drop(dropInfo);
        }

        private void WrapPanel_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            int width = int.Clamp((int)(e.NewSize.Width * 0.75), ITEM_MIN_SIZE, ITEM_MAX_SIZE);
            WrapPanel? wrapPanel = sender as WrapPanel;
            if (wrapPanel != null)
            {
                wrapPanel.ItemWidth = width;
                wrapPanel.ItemHeight = width;
            }
        }
    }
}
