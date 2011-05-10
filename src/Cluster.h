
/**
 * @file Cluster.h
 * @brief Cluster definition
 */

#pragma once

#include <QObject>
#include <QMetaType>

class QScriptProgram;

class btVector3;

class Cluster : public QObject
{
    Q_OBJECT

public:
    Cluster (const btVector3& origin, QScriptProgram& shellProgram,
             QObject* parent = NULL);
    virtual ~Cluster ();

    void makeImage (int maxWidth);

    Q_INVOKABLE void emitStar (btVector3 initialVelocity);

public slots:
    void draw ();
    void update (qreal dt);

private:
    void setup ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

Q_DECLARE_METATYPE(Cluster*)
