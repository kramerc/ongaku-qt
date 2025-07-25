#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <QTableView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_viewStack(nullptr)
    , m_libraryView(nullptr)
    , m_flatView(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_searchEdit(nullptr)
    , m_searchTimer(new QTimer(this))
    , m_viewUpdateTimer(new QTimer(this))
    , m_sortCombo(nullptr)
    , m_viewCombo(nullptr)
    , m_scanButton(nullptr)
    , m_databaseManager(new DatabaseManager(this))
    , m_musicScanner(new MusicScanner(m_databaseManager, this))
    , m_libraryModel(new MusicLibraryModel(m_databaseManager, this))
    , m_flatModel(new MusicLibraryFlatModel(m_databaseManager, this))
    , m_musicPlayer(new MusicPlayer(this))
    , m_scanInProgress(false)
    , m_pendingViewUpdate(false)
{
    setupUI();
    connectSignals();

    // Set window title
    setWindowTitle("Ongaku - Music Library Manager");

    // Initialize the database
    if (!m_databaseManager->initialize()) {
        QMessageBox::critical(this, "Database Error", "Failed to initialize database.");
        return;
    }

    // Load existing library
    m_libraryModel->refreshData();
    m_flatModel->refreshData();

    m_statusLabel->setText("Ready");
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setupMenuBar(); // Initialize menu actions first
    setupStatusBar(); // Initialize status bar components

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Create toolbar section
    QHBoxLayout *toolbarLayout = new QHBoxLayout;

    // Search section
    QLabel *searchLabel = new QLabel("Search:");
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Search for tracks, artists, albums, or genres...");
    m_searchEdit->setMinimumWidth(300);

    // Sort section
    QLabel *sortLabel = new QLabel("Sort by:");
    m_sortCombo = new QComboBox;
    m_sortCombo->addItem("Artist > Album", MusicLibraryModel::SortByArtistAlbum);
    m_sortCombo->addItem("Album", MusicLibraryModel::SortByAlbum);
    m_sortCombo->addItem("Genre", MusicLibraryModel::SortByGenre);
    m_sortCombo->addItem("Year", MusicLibraryModel::SortByYear);

    // View mode section
    QLabel *viewLabel = new QLabel("View:");
    m_viewCombo = new QComboBox;
    m_viewCombo->addItem("Tree View", 0);
    m_viewCombo->addItem("Flat List", 1);

    // Action buttons
    m_scanButton = new QPushButton("Scan Library");
    m_refreshButton = new QPushButton("Refresh");

    toolbarLayout->addWidget(searchLabel);
    toolbarLayout->addWidget(m_searchEdit);
    toolbarLayout->addSpacing(20);
    toolbarLayout->addWidget(sortLabel);
    toolbarLayout->addWidget(m_sortCombo);
    toolbarLayout->addSpacing(20);
    toolbarLayout->addWidget(viewLabel);
    toolbarLayout->addWidget(m_viewCombo);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_refreshButton);
    toolbarLayout->addWidget(m_scanButton);

    mainLayout->addLayout(toolbarLayout);

    // Create splitter for resizable sections
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Create stacked widget to switch between tree and table views
    m_viewStack = new QStackedWidget;

    // Library tree view
    m_libraryView = new QTreeView;
    m_libraryView->setModel(m_libraryModel);
    m_libraryView->setAlternatingRowColors(true);
    m_libraryView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_libraryView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_libraryView->setSortingEnabled(false); // We handle sorting through the model
    m_libraryView->setRootIsDecorated(true);
    m_libraryView->setExpandsOnDoubleClick(false);
    m_libraryView->setItemsExpandable(true);
    m_libraryView->setUniformRowHeights(true); // Improves performance

    // Configure tree view header
    QHeaderView *treeHeader = m_libraryView->header();
    treeHeader->setStretchLastSection(false);
    treeHeader->resizeSection(MusicLibraryModel::TitleColumn, 250);
    treeHeader->resizeSection(MusicLibraryModel::ArtistColumn, 200);
    treeHeader->resizeSection(MusicLibraryModel::AlbumColumn, 200);
    treeHeader->resizeSection(MusicLibraryModel::GenreColumn, 120);
    treeHeader->resizeSection(MusicLibraryModel::YearColumn, 60);
    treeHeader->resizeSection(MusicLibraryModel::TrackColumn, 60);
    treeHeader->resizeSection(MusicLibraryModel::DurationColumn, 80);
    treeHeader->setSectionResizeMode(MusicLibraryModel::TitleColumn, QHeaderView::Stretch);

    // Flat table view
    m_flatView = new QTableView;
    m_flatView->setModel(m_flatModel);
    m_flatView->setAlternatingRowColors(true);
    m_flatView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_flatView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_flatView->setSortingEnabled(true); // Enable column sorting for flat view
    m_flatView->setShowGrid(false);
    m_flatView->verticalHeader()->setVisible(false);
    m_flatView->horizontalHeader()->setHighlightSections(false);

    // Configure table view header
    QHeaderView *tableHeader = m_flatView->horizontalHeader();
    tableHeader->setStretchLastSection(false);
    tableHeader->resizeSection(MusicLibraryFlatModel::TitleColumn, 250);
    tableHeader->resizeSection(MusicLibraryFlatModel::ArtistColumn, 200);
    tableHeader->resizeSection(MusicLibraryFlatModel::AlbumColumn, 200);
    tableHeader->resizeSection(MusicLibraryFlatModel::GenreColumn, 120);
    tableHeader->resizeSection(MusicLibraryFlatModel::YearColumn, 60);
    tableHeader->resizeSection(MusicLibraryFlatModel::TrackColumn, 60);
    tableHeader->resizeSection(MusicLibraryFlatModel::DurationColumn, 80);
    tableHeader->setSectionResizeMode(MusicLibraryFlatModel::TitleColumn, QHeaderView::Stretch);

    // Add views to stacked widget
    m_viewStack->addWidget(m_libraryView);  // Index 0
    m_viewStack->addWidget(m_flatView);     // Index 1

    splitter->addWidget(m_viewStack);
    splitter->setSizes({800, 200});

    // Create vertical splitter for library and player
    QSplitter *verticalSplitter = new QSplitter(Qt::Vertical);
    verticalSplitter->addWidget(splitter);
    verticalSplitter->addWidget(m_musicPlayer);
    verticalSplitter->setSizes({600, 200}); // Give more space to library, smaller to player

    mainLayout->addWidget(verticalSplitter);

    // Expand first level by default
    m_libraryView->expandToDepth(0);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();

    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");

    m_scanAction = new QAction("&Scan Library", this);
    m_scanAction->setShortcut(QKeySequence("Ctrl+S"));
    m_scanAction->setStatusTip("Scan music directory for new files");
    fileMenu->addAction(m_scanAction);

    m_refreshAction = new QAction("&Refresh Library", this);
    m_refreshAction->setShortcut(QKeySequence("F5"));
    m_refreshAction->setStatusTip("Refresh the library view");
    fileMenu->addAction(m_refreshAction);

    fileMenu->addSeparator();

    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    m_exitAction->setStatusTip("Exit the application");
    fileMenu->addAction(m_exitAction);

    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");

    m_aboutAction = new QAction("&About Ongaku", this);
    m_aboutAction->setStatusTip("Show information about Ongaku");
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *statusBar = this->statusBar();

    m_statusLabel = new QLabel("Ready");
    m_trackCountLabel = new QLabel;
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);

    statusBar->addWidget(m_statusLabel);
    statusBar->addPermanentWidget(m_progressBar);
    statusBar->addPermanentWidget(m_trackCountLabel);
}

void MainWindow::connectSignals()
{
    // Search functionality
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);

    // Set search timer to single shot and give enough delay for user to finish typing
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(500); // 500ms delay after user stops typing

    connect(m_searchTimer, &QTimer::timeout, [this]() {
        QString searchText = m_searchEdit->text().trimmed();
        qDebug() << "Search timer triggered with text:" << searchText;
        if (searchText.isEmpty()) {
            m_libraryModel->showAllTracks();
            m_flatModel->showAllTracks();
        } else {
            m_libraryModel->searchTracks(searchText);
            m_flatModel->searchTracks(searchText);
        }
        m_libraryView->expandToDepth(0);
    });

    // View update timer for scanning (set interval and single shot)
    m_viewUpdateTimer->setSingleShot(false);
    m_viewUpdateTimer->setInterval(2000); // Update every 2 seconds during scanning
    connect(m_viewUpdateTimer, &QTimer::timeout, this, &MainWindow::onUpdateViewDuringScanning);

    // Sort functionality
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSortModeChanged);

    // View mode functionality
    connect(m_viewCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onViewModeChanged);

    // Button actions
    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::onScanLibrary);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshLibrary);

    // Menu actions
    connect(m_scanAction, &QAction::triggered, this, [this]() { onScanLibrary(); });
    connect(m_refreshAction, &QAction::triggered, this, [this]() { onRefreshLibrary(); });
    connect(m_exitAction, &QAction::triggered, this, [this]() { close(); });
    connect(m_aboutAction, &QAction::triggered, this, [this]() { onAbout(); });

    // Scanner signals
    connect(m_musicScanner, &MusicScanner::scanStarted, this, &MainWindow::onScanStarted);
    connect(m_musicScanner, &MusicScanner::scanProgress, this, &MainWindow::onScanProgress);
    connect(m_musicScanner, &MusicScanner::trackScanned, this, &MainWindow::onTrackScanned);
    connect(m_musicScanner, &MusicScanner::trackAdded, this, &MainWindow::onTrackAdded);
    connect(m_musicScanner, &MusicScanner::trackUpdated, this, &MainWindow::onTrackUpdated);
    connect(m_musicScanner, &MusicScanner::scanCompleted, this, &MainWindow::onScanCompleted);
    connect(m_musicScanner, &MusicScanner::scanError, this, &MainWindow::onScanError);

    // Library view signals
    connect(m_libraryView, &QTreeView::doubleClicked, this, &MainWindow::onLibraryDoubleClicked);
    connect(m_flatView, &QTableView::doubleClicked, this, &MainWindow::onLibraryDoubleClicked);
}

void MainWindow::onSearchTextChanged()
{
    qDebug() << "Search text changed, restarting timer";
    // Stop the timer and restart it - this gives user time to finish typing
    m_searchTimer->stop();
    m_searchTimer->start();
}

void MainWindow::onSortModeChanged()
{
    MusicLibraryModel::SortMode mode = static_cast<MusicLibraryModel::SortMode>(
        m_sortCombo->currentData().toInt());
    m_libraryModel->setSortMode(mode);

    // For now, we'll keep sorting disabled in the tree view
    // Column sorting can be added later with a more efficient implementation
    m_libraryView->setSortingEnabled(false);
    m_libraryView->header()->setSectionsClickable(false);
    m_libraryView->header()->setSortIndicatorShown(false);

    m_libraryView->expandToDepth(0);

    // Note: Flat view uses column-based sorting, so no need to apply sort mode there
}

void MainWindow::onViewModeChanged()
{
    int viewIndex = m_viewCombo->currentData().toInt();
    m_viewStack->setCurrentIndex(viewIndex);

    if (viewIndex == 0) {
        // Tree view - enable/disable sort combo based on tree view
        m_sortCombo->setEnabled(true);
        // Ensure tree view is expanded
        m_libraryView->expandToDepth(0);
    } else {
        // Flat view - disable tree-specific sort combo since flat view has column sorting
        m_sortCombo->setEnabled(false);
    }
}

void MainWindow::onScanLibrary()
{
    if (m_scanInProgress) {
        m_musicScanner->stopScanning();
        return;
    }

    QDir musicDir("/mnt/shucked/Music");
    if (!musicDir.exists()) {
        QMessageBox::warning(this, "Directory Not Found",
                            "Music directory '/mnt/shucked/Music' does not exist.\n"
                            "Please make sure the directory is mounted and accessible.");
        return;
    }

    m_musicScanner->scanLibrary();
}

void MainWindow::onScanStarted()
{
    m_scanInProgress = true;
    m_pendingViewUpdate = false;
    m_scanButton->setText("Stop Scan");
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Scanning music library...");

    // Disable other actions during scan
    m_refreshButton->setEnabled(false);
    m_scanAction->setText("Stop Scan");
}

void MainWindow::onScanProgress(int current, int total)
{
    if (total > 0) {
        int percentage = (current * 100) / total;
        m_progressBar->setValue(percentage);
        m_statusLabel->setText(QString("Scanning... %1 of %2 files (%3%)")
                               .arg(current).arg(total).arg(percentage));
    }
}

void MainWindow::onTrackScanned(const QString &filePath)
{
    Q_UNUSED(filePath);
    // Could show current file being scanned if needed
}

void MainWindow::onTrackAdded(const MusicTrack &track)
{
    Q_UNUSED(track);
    // Mark that we have pending updates
    m_pendingViewUpdate = true;

    // Start the update timer if it's not already running
    if (!m_viewUpdateTimer->isActive()) {
        m_viewUpdateTimer->start();
    }

    // Update track count immediately for responsive feedback
    updateStatusBar();
}

void MainWindow::onTrackUpdated(const MusicTrack &track)
{
    Q_UNUSED(track);
    // Mark that we have pending updates
    m_pendingViewUpdate = true;

    // Start the update timer if it's not already running
    if (!m_viewUpdateTimer->isActive()) {
        m_viewUpdateTimer->start();
    }
}

void MainWindow::onScanCompleted(int tracksFound, int tracksAdded, int tracksUpdated)
{
    m_scanInProgress = false;
    m_pendingViewUpdate = false;
    m_viewUpdateTimer->stop();
    m_scanButton->setText("Scan Library");
    m_progressBar->setVisible(false);

    QString message = QString("Scan completed. Found %1 files, added %2 tracks, updated %3 tracks.")
                     .arg(tracksFound).arg(tracksAdded).arg(tracksUpdated);
    m_statusLabel->setText(message);

    // Re-enable controls
    m_refreshButton->setEnabled(true);
    m_scanAction->setText("Scan Library");

    // Final refresh of both models
    m_libraryModel->refreshData();
    m_flatModel->refreshData();
    m_libraryView->expandToDepth(0);
    updateStatusBar();

    // Show completion message if significant changes
    if (tracksAdded > 0 || tracksUpdated > 0) {
        QMessageBox::information(this, "Scan Complete", message);
    }
}

void MainWindow::onScanError(const QString &error)
{
    m_scanInProgress = false;
    m_pendingViewUpdate = false;
    m_viewUpdateTimer->stop();
    m_scanButton->setText("Scan Library");
    m_progressBar->setVisible(false);
    m_refreshButton->setEnabled(true);
    m_scanAction->setText("Scan Library");

    m_statusLabel->setText("Scan failed");
    QMessageBox::critical(this, "Scan Error", "Failed to scan music library:\n" + error);
}

void MainWindow::onLibraryDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        qDebug() << "Invalid index in double-click";
        return;
    }

    // Don't process double-clicks if search timer is active (user is still typing)
    if (m_searchTimer->isActive()) {
        qDebug() << "Search timer active, ignoring double-click";
        return;
    }

    MusicTrack track;

    // Determine which view sent the signal and get the track accordingly
    if (m_viewStack->currentIndex() == 0) {
        // Tree view
        track = m_libraryModel->getTrack(index);
    } else {
        // Flat view
        track = m_flatModel->getTrack(index);
    }

    qDebug() << "Double-clicked item, track path:" << track.filePath;

    if (!track.filePath.isEmpty()) {
        QFileInfo fileInfo(track.filePath);
        if (fileInfo.exists()) {
            qDebug() << "Playing track:" << track.title << "by" << track.artist;
            // Add track to music player queue and play it
            m_musicPlayer->playTrack(track);
        } else {
            qDebug() << "File not found:" << track.filePath;
            QMessageBox::warning(this, "File Not Found",
                                QString("The file '%1' could not be found.").arg(track.filePath));
        }
    } else {
        qDebug() << "Double-clicked on non-track item (probably artist/album folder)";
        // If it's not a track (i.e., artist or album), try to expand/collapse (only for tree view)
        if (m_viewStack->currentIndex() == 0) {
            if (m_libraryView->isExpanded(index)) {
                m_libraryView->collapse(index);
            } else {
                m_libraryView->expand(index);
            }
        }
    }
}

void MainWindow::onRefreshLibrary()
{
    m_libraryModel->refreshData();
    m_flatModel->refreshData();
    m_libraryView->expandToDepth(0);
    updateStatusBar();
    m_statusLabel->setText("Library refreshed");
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About Ongaku",
        "<h3>Ongaku</h3>"
        "<p>A modern music library manager built with Qt6 and TagLib.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Automatic music file scanning and metadata extraction</li>"
        "<li>SQLite database for fast searching and sorting</li>"
        "<li>Multiple view modes (Tree and Flat List)</li>"
        "<li>Sortable columns with real-time search functionality</li>"
        "</ul>"
        "<p><b>Version:</b> 1.0.0</p>"
        "<p><b>Built with:</b> Qt6, TagLib, SQLite</p>");
}

void MainWindow::updateStatusBar()
{
    int trackCount = m_databaseManager->getTrackCount();
    qDebug() << "Track count in database:" << trackCount;
    m_trackCountLabel->setText(QString("%1 tracks in library").arg(trackCount));
}

void MainWindow::onUpdateViewDuringScanning()
{
    if (m_scanInProgress && m_pendingViewUpdate) {
        // Refresh both models to show new tracks
        m_libraryModel->refreshData();
        m_flatModel->refreshData();
        m_libraryView->expandToDepth(0);
        m_pendingViewUpdate = false;

        // Update the status bar to show current track count
        updateStatusBar();
    }
}
