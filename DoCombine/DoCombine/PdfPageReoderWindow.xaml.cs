using GongSolutions.Wpf.DragDrop;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Imaging;
using Windows.Data.Pdf;
using Windows.Storage.Streams;
using Wpf.Ui.Controls;

using PdfSharpDocument = PdfSharp.Pdf.PdfDocument;
using PdfSharpPage = PdfSharp.Pdf.PdfPage;

namespace DoCombine
{
    public sealed class PageThumbnail : INotifyPropertyChanged
    {
        private int _index;
        private StrongBox<List<Tuple<PdfSharpPage, WeakReference>>> _pages;
        public int Index
        {
            get
            {
                return _index + 1;
            }
            private set
            {
                _index = value;
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        public object? Thumbnail
        {
            get
            {
                if (!_pages.Value![_index].Item2.IsAlive)
                {
                    GetThumbnail().ContinueWith(t =>
                    {
                        Thumbnail = t.Result;
                    });
                    return null;
                }
                return _pages.Value![_index].Item2.Target ?? null;
            }
            private set
            {
                if (value != null && _pages.Value![_index].Item2.Target != value)
                {
                    _pages.Value![_index].Item2.Target = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool IsLoading
        {
            get
            {
                return !_pages.Value![_index].Item2.IsAlive;
            }
        }

        public PageThumbnail(int index, StrongBox<List<Tuple<PdfSharpPage, WeakReference>>> pages)
        {
            Index = index;
            _pages = pages;
        }

        private async Task<BitmapImage?> GetThumbnail()
        {
            // Uncomment next line to see the loaders
            // await Task.Delay(5000);
            if (_pages.Value is null || _pages.Value.Count < _index)
            {
                //This should never happen, but guard against it anyway
                return null;
            }

            PdfSharpDocument tmp = new PdfSharpDocument();
            MemoryStream stream = new MemoryStream();
            tmp.AddPage(_pages.Value[_index].Item1);
            tmp.Save(stream);
            tmp.Dispose();

            PdfDocument pdf = await PdfDocument.LoadFromStreamAsync(stream.AsRandomAccessStream());
            PdfPage page = pdf.GetPage(0);
            await page.PreparePageAsync();

            using (var imras = new InMemoryRandomAccessStream())
            {
                PdfPageRenderOptions renderOptions = new PdfPageRenderOptions();
                renderOptions.DestinationHeight = (uint)(PdfPageReorderWindow.ITEM_MAX_SIZE * 0.75) * 2;
                renderOptions.DestinationWidth = (uint)(page.Size.Width / page.Size.Height * PdfPageReorderWindow.ITEM_MAX_SIZE * 0.75) * 2;
                renderOptions.BitmapEncoderId = Windows.Graphics.Imaging.BitmapEncoder.PngEncoderId;
                renderOptions.BackgroundColor = Windows.UI.Color.FromArgb(0xff, 0xff, 0xff, 0xff);

                await page.RenderToStreamAsync(imras, renderOptions);

                var bitmapImage = new BitmapImage();
                bitmapImage.BeginInit();
                bitmapImage.StreamSource = imras.AsStream();
                bitmapImage.CacheOption = BitmapCacheOption.OnLoad;
                bitmapImage.EndInit();
                bitmapImage.Freeze();
                stream.Dispose();
                return bitmapImage;
            }
        }

        private void OnPropertyChanged()
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(null));
        }
    }

    public partial class PdfPageReorderWindow : FluentWindow, IDropTarget, IDisposable
    {
        public const int ITEM_MIN_SIZE = 200;
        public const int ITEM_MAX_SIZE = 300;
        public ObservableCollection<PageThumbnail> Thumbnails { get; set; }
        public ObservablePrimitive<bool> Modified { get; set; }

        private List<Tuple<PdfSharpPage, WeakReference>> _initialPages = [];

        public PdfPageReorderWindow(List<PdfSharpPage> initialPages)
        {
            InitializeComponent();
            DataContext = this;
            initialPages.ToList().ForEach(page => _initialPages.Add(new Tuple<PdfSharpPage, WeakReference>(page, new WeakReference(null))));
            Thumbnails = new ObservableCollection<PageThumbnail>();

            Enumerable.Range(0, _initialPages.Count).ToList()
                .ForEach(idx => Thumbnails.Add(new PageThumbnail(idx, new StrongBox<List<Tuple<PdfSharpPage, WeakReference>>>(_initialPages))));

            Modified = new ObservablePrimitive<bool>(false);
        }

        void IDisposable.Dispose()
        {
            for (int i = 0; i < _initialPages.Count; ++i)
            {
                if (_initialPages[i].Item2.IsAlive)
                {
                    _initialPages[i].Item2.Target = null;
                }
            }
            _initialPages.Clear();
        }

        private void OkButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
            Close();
        }

        private void ResetButton_Click(object sender, RoutedEventArgs e)
        {
            Modified.Object = false;
            Thumbnails.Clear();
            Enumerable.Range(0, _initialPages.Count).ToList()
                .ForEach(idx => Thumbnails.Add(new PageThumbnail(idx, new StrongBox<List<Tuple<PdfSharpPage, WeakReference>>>(_initialPages))));
        }

        private void PageDelete_Click(object sender, RoutedEventArgs e)
        {
            Modified.Object = true;
            Wpf.Ui.Controls.MenuItem? menuItem = sender as Wpf.Ui.Controls.MenuItem;
            if (menuItem != null)
            {
                PageThumbnail? thumbnail = menuItem.DataContext as PageThumbnail;
                if (thumbnail != null)
                {
                    Thumbnails.Remove(thumbnail);
                }
            }
        }

        void IDropTarget.DragOver(IDropInfo dropInfo)
        {
            GongSolutions.Wpf.DragDrop.DragDrop.DefaultDropHandler.DragOver(dropInfo);
        }

        void IDropTarget.Drop(IDropInfo dropInfo)
        {
            if (dropInfo.Data is PageThumbnail[] || dropInfo.InsertIndex != dropInfo.DragInfo.SourceIndex)
            {
                Modified.Object = true;
            }
            GongSolutions.Wpf.DragDrop.DragDrop.DefaultDropHandler.Drop(dropInfo);
        }

        private void WrapPanel_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            int width = int.Clamp((int)(e.NewSize.Width / 5), ITEM_MIN_SIZE, ITEM_MAX_SIZE);
            WrapPanel? wrapPanel = sender as WrapPanel;
            if (wrapPanel != null)
            {
                wrapPanel.ItemWidth = width;
                wrapPanel.ItemHeight = width;
            }
        }
    }
}
