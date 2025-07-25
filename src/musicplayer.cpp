#include "musicplayer.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QStyle>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

MusicPlayer::MusicPlayer(QWidget *parent)
    : QWidget(parent)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_currentIndex(-1)
    , m_seeking(false)
{
    // Initialize media player first
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // Initialize update timer
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(100); // Update every 100ms

    // Setup UI after media player is ready
    setupUI();
    connectSignals();

    // Set initial volume and button state
    setVolume(50);
    updatePlayButton();
}

MusicPlayer::~MusicPlayer()
{
}

void MusicPlayer::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Create splitter for player controls and queue
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // Player controls section
    QWidget *controlsWidget = new QWidget;
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsWidget);

    // Track info
    m_trackInfoLabel = new QLabel("No track loaded");
    m_trackInfoLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    m_trackInfoLabel->setWordWrap(true);
    controlsLayout->addWidget(m_trackInfoLabel);

    // Position slider
    m_positionSlider = new QSlider(Qt::Horizontal);
    m_positionSlider->setEnabled(false);
    controlsLayout->addWidget(m_positionSlider);

    // Time display
    m_timeLabel = new QLabel("00:00 / 00:00");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    controlsLayout->addWidget(m_timeLabel);

    // Control buttons
    QHBoxLayout *buttonsLayout = new QHBoxLayout;

    m_previousButton = new QPushButton();
    m_previousButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    m_previousButton->setEnabled(false);

    m_playButton = new QPushButton();
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playButton->setEnabled(false);

    m_stopButton = new QPushButton();
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setEnabled(false);

    m_nextButton = new QPushButton();
    m_nextButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    m_nextButton->setEnabled(false);

    buttonsLayout->addWidget(m_previousButton);
    buttonsLayout->addWidget(m_playButton);
    buttonsLayout->addWidget(m_stopButton);
    buttonsLayout->addWidget(m_nextButton);
    buttonsLayout->addStretch();

    // Volume control
    m_volumeLabel = new QLabel("Volume:");
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    m_volumeSlider->setMaximumWidth(100);

    buttonsLayout->addWidget(m_volumeLabel);
    buttonsLayout->addWidget(m_volumeSlider);

    controlsLayout->addLayout(buttonsLayout);
    controlsLayout->addStretch();

    // Queue section
    QWidget *queueWidget = new QWidget;
    QVBoxLayout *queueLayout = new QVBoxLayout(queueWidget);

    QGroupBox *queueGroup = new QGroupBox("Play Queue");
    QVBoxLayout *queueGroupLayout = new QVBoxLayout(queueGroup);

    m_queueList = new QListWidget;
    m_queueList->setAlternatingRowColors(true);
    queueGroupLayout->addWidget(m_queueList);

    // Queue control buttons
    QHBoxLayout *queueButtonsLayout = new QHBoxLayout;
    m_removeFromQueueButton = new QPushButton("Remove Selected");
    m_removeFromQueueButton->setEnabled(false);
    QPushButton *clearQueueButton = new QPushButton("Clear Queue");

    queueButtonsLayout->addWidget(m_removeFromQueueButton);
    queueButtonsLayout->addWidget(clearQueueButton);
    queueButtonsLayout->addStretch();

    queueGroupLayout->addLayout(queueButtonsLayout);
    queueLayout->addWidget(queueGroup);

    // Add to splitter
    splitter->addWidget(controlsWidget);
    splitter->addWidget(queueWidget);
    splitter->setSizes({400, 300});

    mainLayout->addWidget(splitter);

    // Connect queue buttons
    connect(m_removeFromQueueButton, &QPushButton::clicked, this, &MusicPlayer::removeFromQueue);
    connect(clearQueueButton, &QPushButton::clicked, this, &MusicPlayer::clearQueue);
    connect(m_queueList, &QListWidget::itemSelectionChanged, [this]() {
        m_removeFromQueueButton->setEnabled(m_queueList->currentRow() >= 0);
    });
}

void MusicPlayer::connectSignals()
{
    // Media player signals
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &MusicPlayer::onPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &MusicPlayer::onDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::onMediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MusicPlayer::onPlaybackStateChanged);

    // Control buttons (play button is handled dynamically in updatePlayButton)
    connect(m_stopButton, &QPushButton::clicked, this, &MusicPlayer::stop);
    connect(m_nextButton, &QPushButton::clicked, this, &MusicPlayer::next);
    connect(m_previousButton, &QPushButton::clicked, this, &MusicPlayer::previous);

    // Sliders
    connect(m_positionSlider, &QSlider::sliderPressed, [this]() { m_seeking = true; });
    connect(m_positionSlider, &QSlider::sliderReleased, [this]() {
        m_seeking = false;
        seek(m_positionSlider->value());
    });
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MusicPlayer::setVolume);

    // Queue list
    connect(m_queueList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        onQueueItemDoubleClicked(m_queueList->row(item));
    });

    // Update timer
    connect(m_updateTimer, &QTimer::timeout, this, &MusicPlayer::updateTimeDisplay);
}

void MusicPlayer::playTrack(const MusicTrack &track)
{
    // Clear queue and add this track
    clearQueue();
    addToQueue(track);

    // Play the track
    m_currentIndex = 0;
    playCurrentTrack();
}

void MusicPlayer::addToQueue(const MusicTrack &track)
{
    m_queue.append(track);

    // Add to queue list widget
    QString displayText = QString("%1 - %2").arg(track.artist, track.title);
    if (!track.album.isEmpty()) {
        displayText += QString(" (%1)").arg(track.album);
    }

    QListWidgetItem *item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, QVariant::fromValue(track));
    m_queueList->addItem(item);

    // Enable controls if this is the first track
    if (m_queue.size() == 1 && m_currentIndex < 0) {
        m_currentIndex = 0;
        m_playButton->setEnabled(true);
        m_nextButton->setEnabled(m_queue.size() > 1);
        m_previousButton->setEnabled(false);
    } else {
        m_nextButton->setEnabled(m_queue.size() > 1);
        m_previousButton->setEnabled(m_currentIndex > 0);
    }
}

void MusicPlayer::clearQueue()
{
    stop();
    m_queue.clear();
    m_queueList->clear();
    m_currentIndex = -1;

    m_playButton->setEnabled(false);
    m_stopButton->setEnabled(false);
    m_nextButton->setEnabled(false);
    m_previousButton->setEnabled(false);

    m_trackInfoLabel->setText("No track loaded");
    m_timeLabel->setText("00:00 / 00:00");
    m_positionSlider->setValue(0);
    m_positionSlider->setEnabled(false);
}

void MusicPlayer::play()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        m_mediaPlayer->play();
    } else if (m_currentIndex >= 0 && m_currentIndex < m_queue.size()) {
        playCurrentTrack();
    }
}

void MusicPlayer::pause()
{
    m_mediaPlayer->pause();
}

void MusicPlayer::stop()
{
    m_mediaPlayer->stop();
    m_updateTimer->stop();
}

void MusicPlayer::next()
{
    if (m_currentIndex < m_queue.size() - 1) {
        m_currentIndex++;
        playCurrentTrack();
    }
}

void MusicPlayer::previous()
{
    if (m_currentIndex > 0) {
        m_currentIndex--;
        playCurrentTrack();
    }
}

void MusicPlayer::seek(int position)
{
    if (m_mediaPlayer) {
        m_mediaPlayer->setPosition(position);
    }
}

void MusicPlayer::setVolume(int volume)
{
    if (m_audioOutput) {
        m_audioOutput->setVolume(volume / 100.0f);
    }
}

void MusicPlayer::onPositionChanged(qint64 position)
{
    if (!m_seeking) {
        m_positionSlider->setValue(static_cast<int>(position));
    }
}

void MusicPlayer::onDurationChanged(qint64 duration)
{
    m_positionSlider->setRange(0, static_cast<int>(duration));
    m_positionSlider->setEnabled(duration > 0);
}

void MusicPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        loadNextTrack();
    }
}

void MusicPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    updatePlayButton();

    if (state == QMediaPlayer::PlayingState) {
        m_stopButton->setEnabled(true);
        m_updateTimer->start();
    } else {
        if (state == QMediaPlayer::StoppedState) {
            m_stopButton->setEnabled(false);
        }
        m_updateTimer->stop();
    }
}

void MusicPlayer::onQueueItemDoubleClicked(int row)
{
    if (row >= 0 && row < m_queue.size()) {
        m_currentIndex = row;
        playCurrentTrack();
    }
}

void MusicPlayer::removeFromQueue()
{
    int row = m_queueList->currentRow();
    if (row >= 0 && row < m_queue.size()) {
        // If removing currently playing track, stop playback
        if (row == m_currentIndex) {
            stop();
        }

        // Remove from queue and list
        m_queue.removeAt(row);
        delete m_queueList->takeItem(row);

        // Adjust current index
        if (row < m_currentIndex) {
            m_currentIndex--;
        } else if (row == m_currentIndex && m_currentIndex >= m_queue.size()) {
            m_currentIndex = m_queue.size() - 1;
        }

        // Update button states
        if (m_queue.isEmpty()) {
            clearQueue();
        } else {
            m_nextButton->setEnabled(m_queue.size() > 1);
            m_previousButton->setEnabled(m_currentIndex > 0);
        }
    }
}

void MusicPlayer::updateTimeDisplay()
{
    qint64 position = m_mediaPlayer->position();
    qint64 duration = m_mediaPlayer->duration();

    QString timeText = QString("%1 / %2")
                       .arg(formatTime(position))
                       .arg(formatTime(duration));
    m_timeLabel->setText(timeText);
}

void MusicPlayer::updatePlayButton()
{
    if (!m_mediaPlayer || !m_playButton) {
        return; // Safety check
    }

    // Disconnect only our specific clicked signal connections
    disconnect(m_playButton, &QPushButton::clicked, this, &MusicPlayer::play);
    disconnect(m_playButton, &QPushButton::clicked, this, &MusicPlayer::pause);

    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        connect(m_playButton, &QPushButton::clicked, this, &MusicPlayer::pause);
    } else {
        m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        connect(m_playButton, &QPushButton::clicked, this, &MusicPlayer::play);
    }
}

void MusicPlayer::updateTrackInfo(const MusicTrack &track)
{
    QString info = QString("<b>%1</b><br>by %2").arg(track.title, track.artist);
    if (!track.album.isEmpty()) {
        info += QString("<br>from <i>%1</i>").arg(track.album);
    }
    m_trackInfoLabel->setText(info);

    // Highlight current track in queue
    for (int i = 0; i < m_queueList->count(); ++i) {
        QListWidgetItem *item = m_queueList->item(i);
        QFont font = item->font();
        font.setBold(i == m_currentIndex);
        item->setFont(font);
    }

    emit trackChanged(track);
}

void MusicPlayer::playCurrentTrack()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_queue.size()) {
        m_currentTrack = m_queue[m_currentIndex];

        QFileInfo fileInfo(m_currentTrack.filePath);
        if (fileInfo.exists()) {
            QUrl fileUrl = QUrl::fromLocalFile(m_currentTrack.filePath);
            m_mediaPlayer->setSource(fileUrl);
            m_mediaPlayer->play();

            updateTrackInfo(m_currentTrack);

            // Update button states
            m_nextButton->setEnabled(m_currentIndex < m_queue.size() - 1);
            m_previousButton->setEnabled(m_currentIndex > 0);
        } else {
            qWarning() << "File not found:" << m_currentTrack.filePath;
            // Try next track
            loadNextTrack();
        }
    }
}

void MusicPlayer::loadNextTrack()
{
    if (m_currentIndex < m_queue.size() - 1) {
        next();
    } else {
        // End of queue
        stop();
    }
}

QString MusicPlayer::formatTime(qint64 milliseconds)
{
    int seconds = static_cast<int>(milliseconds / 1000);
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}
