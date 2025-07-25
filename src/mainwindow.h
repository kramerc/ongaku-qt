#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QTableView>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <QTimer>
#include <QStackedWidget>

#include "databasemanager.h"
#include "musicscanner.h"
#include "musiclibrarymodel.h"
#include "musiclibraryflat.h"
#include "musicplayer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSearchTextChanged();
    void onSortModeChanged();
    void onViewModeChanged();
    void onScanLibrary();
    void onScanStarted();
    void onScanProgress(int current, int total);
    void onTrackScanned(const QString &filePath);
    void onTrackAdded(const MusicTrack &track);
    void onTrackUpdated(const MusicTrack &track);
    void onScanCompleted(int tracksFound, int tracksAdded, int tracksUpdated);
    void onScanError(const QString &error);
    void onLibraryDoubleClicked(const QModelIndex &index);
    void onRefreshLibrary();
    void onAbout();
    void onUpdateViewDuringScanning();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void connectSignals();
    void updateStatusBar();

    // Core components
    DatabaseManager *m_databaseManager;
    MusicScanner *m_musicScanner;
    MusicLibraryModel *m_libraryModel;
    MusicLibraryFlatModel *m_flatModel;
    MusicPlayer *m_musicPlayer;

    // UI components
    QStackedWidget *m_viewStack;
    QTreeView *m_libraryView;
    QTableView *m_flatView;
    QLineEdit *m_searchEdit;
    QComboBox *m_sortCombo;
    QComboBox *m_viewCombo;
    QPushButton *m_scanButton;
    QPushButton *m_refreshButton;

    // Status bar components
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_trackCountLabel;

    // Menu actions
    QAction *m_scanAction;
    QAction *m_refreshAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;

    QTimer *m_searchTimer;
    QTimer *m_viewUpdateTimer;
    bool m_scanInProgress;
    bool m_pendingViewUpdate;
};

#endif // MAINWINDOW_H
