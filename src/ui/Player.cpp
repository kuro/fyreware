
/**
 * @file Player.cpp
 * @brief Player implementation
 */

#include "defs.h"

#include "Player.moc"

#include "../Playlist.h"
#include "Scene.h"
#include "PlaylistWidget.h"

#include <QtFMOD/Channel.h>
#include <QtFMOD/Sound.h>

#include <QSvgRenderer>
#include <QPainter>
#include <QImage>

using namespace QtFMOD;

struct Player::Private
{
    QIcon prevIcon;
    QIcon nextIcon;
    QIcon playIcon;
    QIcon pauseIcon;

    QSvgRenderer renderer;
    Private (Player* q) //:
    {
        Q_UNUSED(q);
    }

    QPixmap loadIcon (const QSize& size, const QString& id);
};

QPixmap Player::Private::loadIcon (const QSize& size, const QString& id)
{
    QImage image (size, QImage::Format_ARGB32);
    image.fill(0);
    QPainter painter;
    painter.begin(&image);
    renderer.render(&painter, id);
    painter.end();
    return QPixmap::fromImage(image);
}

Player::Player (QWidget* parent) :
    QWidget(parent),
    d(new Private(this))
{
    setupUi(this);
    d->renderer.load(QString(":ui/player.svg"));

    d->prevIcon  = d->loadIcon(prevButton->iconSize(), "Previous");
    d->nextIcon  = d->loadIcon(nextButton->iconSize(), "Next");
    d->playIcon  = d->loadIcon(playButton->iconSize(), "Play");
    d->pauseIcon = d->loadIcon(playButton->iconSize(), "Pause");

    prevButton->setIcon(d->prevIcon);
    nextButton->setIcon(d->nextIcon);
    playButton->setIcon(d->playIcon);

    startTimer(100);

    connect(prevButton, SIGNAL(pressed()),
            playlist, SLOT(prev()));
    connect(nextButton, SIGNAL(pressed()),
            playlist, SLOT(next()));
    connect(playButton, SIGNAL(pressed()),
            playlist, SLOT(playPause()));
}

Player::~Player ()
{
}

/**
 * @todo support hours
 */
static inline
QString timeToString (int msecs)
{
    qreal seconds = qreal(msecs) * 0.001;
    qreal minutes = seconds / 60.0;
    seconds = quint32(seconds) % 60;
    minutes = quint32(minutes);

    QString timeString;
    QTextStream textStream (&timeString);
    textStream
        << minutes << ":"
        << qSetPadChar('0') << qSetFieldWidth(2)
        << seconds;
    return timeString;
}

void Player::timerEvent (QTimerEvent* evt)
{
    Q_UNUSED(evt);

    QSharedPointer<Sound> stream (scene->stream());
    QSharedPointer<Channel> channel (scene->streamChannel());

    if (!channel || !stream) {
        timeSlider->setValue(0);
        //volumeSlider->setValue(0);
        playButton->setIcon(d->playIcon);
        timeLabel->setText("0:00");
        timeRemainingLabel->setText("-0:00");
        return;
    }

    // volume
    if (channel->isPlaying()) {
        volumeSlider->setValue(channel->volume() * 100);
    }

    // time
    unsigned int pos = channel->position(FMOD_TIMEUNIT_MS);
    unsigned int len = stream->length(FMOD_TIMEUNIT_MS);

    if (len == 0xffffffff) {
        // probably a radio stream
        timeSlider->setValue(0);
        timeLabel->setText(timeToString(pos));
        timeRemainingLabel->setText(QString());
    } else {
        qreal nt = qreal(pos) / qreal(len);  // the normalized position

        timeSlider->setRange(0, len);
        timeSlider->setValue(pos);

        timeLabel->setText(timeToString(nt * len));
        timeRemainingLabel->setText("-" + timeToString((1.0-nt)*len));
    }

    // icons
    if (!channel->isPlaying() || channel->paused()) {
        playButton->setIcon(d->playIcon);
    } else {
        playButton->setIcon(d->pauseIcon);
    }
}

void Player::on_timeSlider_sliderMoved (int value)
{
    QSharedPointer<Channel> channel (scene->streamChannel());
    if (channel) {
        channel->setPosition(value, FMOD_TIMEUNIT_MS);
    }
}

void Player::on_volumeSlider_sliderMoved (int value)
{
    QSharedPointer<Channel> channel (scene->streamChannel());
    if (channel) {
        channel->setVolume(value / 100.0);
    }
}

/**
 * Handle case where slider is clicked but not dragged.
 */
void Player::on_timeSlider_sliderPressed ()
{
    on_timeSlider_sliderMoved(timeSlider->value());
}

/**
 * Handle case where slider is clicked but not dragged.
 */
void Player::on_volumeSlider_sliderPressed ()
{
    on_volumeSlider_sliderMoved(volumeSlider->value());
}
