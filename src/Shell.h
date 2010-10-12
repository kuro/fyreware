
/**
 * @file Shell.h
 * @brief Shell definition
 */

#pragma once

#include <QObject>

#include <LinearMath/btMotionState.h>

class Shell : public QObject, public btMotionState
{
    Q_OBJECT

public:
    Shell (QObject* parent = NULL);
    virtual ~Shell ();

    void getWorldTransform (btTransform& trx) const;
    void setWorldTransform (const btTransform& trx);

public slots:
    void draw ();
    void update (qreal dt);

private:
    struct Private;
    QScopedPointer<Private> d;
};
