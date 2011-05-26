
/**
 * @file SoundEngine.h
 * @brief SoundEngine definition
 */

#pragma once

#include <QObject>

#include <QPointer>
#include <QWeakPointer>
#include <QUrl>

namespace QtFMOD
{
class System;
class Sound;
class Channel;
}

class Playlist;

class SoundEngine : public QObject
{
    Q_OBJECT

public:
    SoundEngine (QObject* parent = NULL);
    virtual ~SoundEngine ();

    Playlist* playlist () const;

    const QVector<float>& spectrum (int idx) const;
    int spectrumLength () const;

    QtFMOD::System* soundSystem () const;

    QWeakPointer<QtFMOD::Sound> sound (const QString& name) const;

    QWeakPointer<QtFMOD::Sound> stream () const;
    QWeakPointer<QtFMOD::Channel> streamChannel () const;

    void initialize ();

    void playSong (QUrl url = QUrl());

    bool isPlaying () const;

public slots:
    void prev ();
    void play ();
    void next ();

protected:
    void analyzeSound ();
    void checkTags ();

    void advance (int offset);

private slots:
    void autoAdvance ();

private:
    void timerEvent (QTimerEvent* evt);

private:
    struct Private;
    QScopedPointer<Private> d;
};

extern QPointer<SoundEngine> soundEngine;
