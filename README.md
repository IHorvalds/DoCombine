# DoCombine

Utility to merge, reorder and remove pages from PDF Documents.

# TODOs:
- [ ] Implement Shortcut Handler Shell Extension that calls this utility
- [x] ~Fix memory leak when closing reorder page window and when resetting pages after being reordered. Not sure if it's ever garbage collected. C# isn't my first language~
      Still not 100% sure why, but it seems like it was due to freezing `BitmapImage`s with a MemoryStream (as opposed to an `InMemoryRandomAccessStream` ¯\\\_(ツ)\_/¯). Also using
      the classes in `Windows.Data.Pdf` dropped the memory usage quite a bit (from manually running the app and inspecting the memory graph. Didn't capture the data).

# Dependencies:
- [PDFSharp](https://github.com/empira/PDFsharp)
- [WPFUI](https://github.com/lepoco/wpfui)
- [GongSolutions.WPF.DragDrop](https://github.com/punker76/gong-wpf-dragdrop)
