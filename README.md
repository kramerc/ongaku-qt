# Ongaku - Music Library Manager

A modern Qt6-based music library manager that automatically scans and catalogs your music collection with metadata extraction using TagLib.

## Features

- **Automatic Music Scanning**: Recursively scans directories for supported audio formats
- **Metadata Extraction**: Uses TagLib to extract title, artist, album, genre, year, track number, and duration
- **SQLite Database**: Fast, local database storage with indexing for quick searches
- **Multiple View Modes**: 
  - Tree View: Artist > Album (hierarchical)
  - Flat List: Sortable table view with column headers
  - Supports Album, Genre, and Year grouping in tree view
- **Real-time Search**: Search across all metadata fields as you type
- **Live Updates**: View updates in real-time while scanning
- **Batch Processing**: Optimized scanning with database transactions

## Supported Audio Formats

- MP3, FLAC, OGG, M4A, MP4, AAC
- WMA, WAV, AIFF, APE, OPUS

## Prerequisites

- Qt6 (Core, Widgets, Multimedia, and Sql modules)
- CMake 3.16 or higher
- C++17 compatible compiler
- TagLib (for audio metadata reading)
- SQLite (usually included with Qt6)

### Installing TagLib

#### Ubuntu/Debian:
```bash
sudo apt-get install libtag1-dev
```

#### Fedora/CentOS:
```bash
sudo dnf install taglib-devel
```

#### macOS (with Homebrew):
```bash
brew install taglib
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./Ongaku
```

## Project Structure

```
Ongaku/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp
    ├── mainwindow.h
    └── mainwindow.cpp
```

## Features

- **Dual View Modes**:
  - **Folder Tree View**: Browse music folders in a hierarchical tree structure
  - **Library View**: Scan and display all tracks from the entire music collection (Ctrl+L to toggle)
- **File Details Table**: Display detailed information about music files in columns:
  - **Title**: Extracted from file metadata (or filename as fallback)
  - **Artist**: Read from audio file tags
  - **Album**: Read from audio file tags
  - **Publisher/Label**: Extracted from advanced metadata fields
  - **Catalog Number**: Extracted from CATALOGNUMBER or BARCODE fields
  - **Date Modified**: File system modification date
  - **Format**: File extension (MP3, FLAC, WAV, etc.)
  - **Bitrate**: Audio bitrate in kbps
  - **Sample Rate**: Audio sample rate in Hz
  - **File Size**: In MB
- **Music Library Scanner**: Recursively scans the entire music directory to build a complete library database
- **SQLite Caching**: Fast library access with persistent SQLite database cache that automatically updates
- **Smart Cache Management**: 
  - **Automatic loading**: Cached library loads instantly on startup
  - **Cache invalidation**: Automatically refreshes when cache is older than 24 hours
  - **Manual refresh**: F5 key or View menu to force library refresh
  - **Persistent storage**: Cache survives application restarts
- **Built-in Music Player**: 
  - **Double-click to play**: Double-click any track in either view to start playback
  - **Playback controls**: Play/pause, stop buttons
  - **Seek control**: Position slider to navigate within tracks
  - **Volume control**: Adjustable volume slider
  - **Track information**: Current playing track display
  - **Time display**: Current time and total duration
- **Metadata Reading**: Full support for audio metadata using TagLib
- **Supported Formats**: MP3, FLAC, WAV, M4A, OGG, AAC
- **Menu Bar**: File operations, View toggle, and Help menu
- **Keyboard Shortcuts**: 
  - **Ctrl+L**: Toggle between folder and library view
  - **F5**: Refresh library cache (force rescan)
- **Status Bar**: Real-time feedback, file counts, and scan progress
- **Responsive Layout**: Resizable splitter between folder tree and file details
