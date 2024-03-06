# DoCombine

Utility to merge, reorder and remove pages from PDF Documents.

# TODOs:
- [x] ~Implement Shortcut Menu Handler Shell Extension that calls this utility~
- [x] ~Fix memory leak when closing reorder page window and when resetting pages after being reordered. Not sure if it's ever garbage collected. C# isn't my first language~
      Still not 100% sure why, but it seems like it was due to freezing `BitmapImage`s with a MemoryStream (as opposed to an `InMemoryRandomAccessStream` ¯\\\_(ツ)\_/¯). Also using
      the classes in `Windows.Data.Pdf` dropped the memory usage quite a bit (from manually running the app and inspecting the memory graph. Didn't capture the data).
- [x] ~Write installer~
- [x] ~Add logging to the GUI and the shell extension. (How do I change the log level at runtime for an explorer extension??)~ -> ***Added logging only to the Shell Extension, but no runtime log level modification yet.*** It's probably good enough. I'm not doing much error handling in the UI code. Will add a sentinel file for the log level soon(ish).
- [ ] Allow registering the shortcut menu handler system wide from the installer (currently other users will have to use the ellipsis button in the app to register it).
- [ ] Write build script so I don't have to open Visual Studio and the Inno Setup Compiler GUI.

# Dependencies:
- [PDFSharp](https://github.com/empira/PDFsharp)
- [WPFUI](https://github.com/lepoco/wpfui)
- [GongSolutions.WPF.DragDrop](https://github.com/punker76/gong-wpf-dragdrop)
- [Inno Setup](https://github.com/jrsoftware/issrc) - This is an absolutely beautifully simple system for building installers. Everything else from NSIS to WiX was a royal PITA.