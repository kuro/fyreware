
/**
 * @file SoundEngine.cpp
 * @brief SoundEngine implementation
 */

#include "SoundEngine.moc"

#include "defs.h"
#include "scripting.h"
#include "Scene.h"
#include "Playlist.h"

#include <QtFMOD/System.h>
#include <QtFMOD/Channel.h>
#include <QtFMOD/Sound.h>

#include <QDebug>
#include <QScriptEngine>

#define fsysCheck()                                                         \
    do {                                                                    \
        if (d->fsys->error() != FMOD_OK) {                                  \
            qWarning() << Q_FUNC_INFO << __LINE__ << d->fsys->errorString();                           \
        }                                                                   \
    } while (0)

/**
 * How long before we actually go back.
 *
 * In milliseconds.
 */
#define BACK_CUTOFF 10000

#define SPECTRUM_GENERATIONS 8

QPointer<SoundEngine> soundEngine;

struct SoundEngine::Private
{
    QtFMOD::System* fsys;
    QHash<QString, QSharedPointer<QtFMOD::Sound> > sounds;

    QSharedPointer<QtFMOD::Channel> channel;
    QSharedPointer<QtFMOD::Sound> sound;

    int spectrumLength;
    FMOD_DSP_FFT_WINDOW spectrumWindowType;
    QVector<float> spectrumNew[2];      ///< values before smoothing
    QVector<float> spectrum[2];         ///< values after smoothing

    Playlist* playlist;
    int current;
    bool channelConnected;

    Private (SoundEngine* q) :
        fsys(new QtFMOD::System(q)),
        spectrumLength(256),
        spectrumWindowType(FMOD_DSP_FFT_WINDOW_RECT),

        playlist(new Playlist(q)),
        current(0),
        channelConnected(false)
    {
        fsys->setObjectName("fsys");

        spectrumNew[0].resize(spectrumLength);
        spectrumNew[1].resize(spectrumLength);
        spectrum[0].resize(spectrumLength);
        spectrum[1].resize(spectrumLength);

        QMetaObject::connectSlotsByName(q);
    }
};

SoundEngine::SoundEngine (QObject* parent) :
    QObject(parent),
    d(new Private(this))
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT(!soundEngine);
    soundEngine = this;
}

SoundEngine::~SoundEngine ()
{
}

Playlist* SoundEngine::playlist () const
{
    return d->playlist;
}

void SoundEngine::initialize ()
{
    qDebug() << Q_FUNC_INFO;
#ifdef Q_OS_LINUX
    d->fsys->setOutput(FMOD_OUTPUTTYPE_ALSA);
#endif

    fsysCheck();

    // init sound system
    d->fsys->init(32);
    fsysCheck();

    d->fsys->set3DNumListeners(1);
    d->fsys->set3DSettings(1.0f, 1.0f, 0.3f);

    // sound effects
    QSharedPointer<QtFMOD::Sound> sound (
        d->fsys->createSound(":media/sfx/explosion0.oga", FMOD_3D)
        );
    fsysCheck();
    d->sounds.insert("explosion", sound);
    sound->set3DMinMaxDistance(150, 600);

    startTimer(10);
}

QtFMOD::System* SoundEngine::soundSystem () const
{
    return d->fsys;
}

QWeakPointer<QtFMOD::Sound> SoundEngine::sound (const QString& name) const
{
    return d->sounds[name];
}

QWeakPointer<QtFMOD::Sound> SoundEngine::stream () const
{
    return d->sound;
}

QWeakPointer<QtFMOD::Channel> SoundEngine::streamChannel () const
{
    return d->channel;
}


const QVector<float>& SoundEngine::spectrum (int idx) const
{
    return d->spectrum[idx];
}

int SoundEngine::spectrumLength () const
{
    return d->spectrumLength;
}

static
QScriptValue launchFun (QScriptContext* ctx, QScriptEngine* eng)
{
    Q_UNUSED(ctx);
    Q_UNUSED(eng);
    scene->launch();
    return QScriptValue();
}

void SoundEngine::analyzeSound ()
{
    if (!d->channel || d->channel->paused()) {
        return;
    }

    // scripted analyzer
    QScriptEngine* scriptEngine = scene->scriptEngine();
    QScriptContext* ctx = scriptEngine->pushContext();
    QScriptValue ao = ctx->activationObject();
    prepGlobalObject(ao);
    ao.setProperty("launch", scriptEngine->newFunction(launchFun));
    scriptEngine->evaluate(scene->analyzerProgram());
    scriptEngine->popContext();
}

void SoundEngine::checkTags ()
{
    if (!d->sound) {
        return;
    }
    int nbUpdated;
    QHash<QString, QtFMOD::Tag> tags (d->sound->tags(&nbUpdated));
    if (nbUpdated > 0) {
        if (tags.contains("TITLE")) {
            QString title (tr("%0 / %1 - FyreWare"));
            title = title.arg(tags["ARTIST"].value().toString());
            title = title.arg(tags["TITLE"].value().toString());
            //setWindowTitle(title);
        }
        qDebug();
    }
    QHashIterator<QString, QtFMOD::Tag> tagIter (tags);
    while (tagIter.hasNext()) {
        tagIter.next();
        QtFMOD::Tag tag (tagIter.value());
        if (tag.updated()) {
            qDebug().nospace()
                << qPrintable(tagIter.key()) << ": "
                << qPrintable(tag.value().toString());
            if (tag.name() == "APIC") {
                //setWindowIcon(QPixmap::fromImage(tag.toImage()));
            }
        }
    }
}

void SoundEngine::timerEvent (QTimerEvent* evt)
{
    Q_UNUSED(evt);

    Q_ASSERT(d->fsys);

    d->fsys->update();

    analyzeSound();
    checkTags();

    // spectrum
    if (isPlaying()) {
        d->channel->spectrum(d->spectrumNew[0], 0, d->spectrumWindowType);
        d->channel->spectrum(d->spectrumNew[1], 1, d->spectrumWindowType);

        for (int i = 0; i < d->spectrumLength; i++) {
            expMovAvg(d->spectrum[0][i],
                      d->spectrumNew[0][i],
                      d->spectrumNew[0][i] > d->spectrum[0][i]
                      ? 1.5 : SPECTRUM_GENERATIONS);
            expMovAvg(d->spectrum[1][i],
                      d->spectrumNew[1][i],
                      d->spectrumNew[1][i] > d->spectrum[1][i]
                      ? 1.5 : SPECTRUM_GENERATIONS);
        }
    }
}

void SoundEngine::prev ()
{
    if (isPlaying() && d->channel->position(FMOD_TIMEUNIT_MS) > BACK_CUTOFF) {
        // back up
        d->channel->setPosition(0, FMOD_TIMEUNIT_MS);
        return;
    }

    advance(-1);

    if (isPlaying()) {
        playSong();
    }
}

void SoundEngine::next ()
{
    advance(1);

    if (isPlaying()) {
        playSong();
    }
}

/**
 * Play/Pause.
 */
void SoundEngine::play ()
{
    if (d->channel) {
        d->channel->setPaused(!d->channel->paused());
    } else {
        playSong();
    }
}

void SoundEngine::advance (int offset)
{
    // check
    if (d->playlist->isEmpty()) {
        d->current = 0;
        return;
    }

    // advance
    d->current += offset;

    // check bounds
    if (d->current < 0) {
        d->current = d->playlist->size() - 1;
    }
    if (d->current >= d->playlist->size()) {
        d->current = 0;
    }

    showit(d->playlist->at(d->current));
}

bool SoundEngine::isPlaying () const
{
    if (!d->channel) {
        return false;
    }
    Q_ASSERT(d->channel);
    return d->channel->isPlaying();
}

void SoundEngine::playSong (QUrl url)
{
    if (!url.isValid()) {
        url = d->playlist->at(d->current);
    }
    showit(url);

    d->sound = d->fsys->createStream(url.toString());
    fsysCheck();
    Q_ASSERT(d->sound);

    d->fsys->playSound(FMOD_CHANNEL_REUSE, d->sound, false, d->channel);
    fsysCheck();
    Q_ASSERT(d->channel);

    showit(d->channel->isPlaying());

    if (!d->channelConnected) {
        d->channelConnected = true;
        connect(d->channel.data(), SIGNAL(soundEnded()), SLOT(autoAdvance()));
    }
}

void SoundEngine::autoAdvance ()
{
    //showit(d->channel->isPlaying());
    //advance(1);
    //playSong();
}
