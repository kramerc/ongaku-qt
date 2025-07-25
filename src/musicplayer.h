#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTimer>
#include "databasemanager.h"

class MusicPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

    void playTrack(const MusicTrack &track);
    void addToQueue(const MusicTrack &track);
    void clearQueue();
    void setVolume(int volume);

public slots:
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void seek(int position);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onQueueItemDoubleClicked(int row);
    void removeFromQueue();
    void updateTimeDisplay();

signals:
    void trackChanged(const MusicTrack &track);

private:
    void setupUI();
    void connectSignals();
    void updatePlayButton();
    void updateTrackInfo(const MusicTrack &track);
    void playCurrentTrack();
    void loadNextTrack();
    QString formatTime(qint64 milliseconds);

    // Media components
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;

    // UI components
    QPushButton *m_playButton;
    QPushButton *m_stopButton;
    QPushButton *m_nextButton;
    QPushButton *m_previousButton;
    QPushButton *m_removeFromQueueButton;

    QSlider *m_positionSlider;
    QSlider *m_volumeSlider;

    QLabel *m_trackInfoLabel;
    QLabel *m_timeLabel;
    QLabel *m_volumeLabel;

    QListWidget *m_queueList;

    QTimer *m_updateTimer;

    // Queue management
    QList<MusicTrack> m_queue;
    int m_currentIndex;
    MusicTrack m_currentTrack;

    bool m_seeking;
};

#endif // MUSICPLAYER_H
