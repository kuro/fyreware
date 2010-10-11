
/**
 * @file ShaderProgram.h
 * @brief ShaderProgram definition
 */

#pragma once

#include <QObject>

#include <Cg/cgGL.h>

class ShaderProgram : public QObject
{
    Q_OBJECT

public:
    ShaderProgram (QObject* parent = NULL);
    virtual ~ShaderProgram ();

    bool addShaderFromSourceCode (CGGLenum profileClass, const QString& code,
                                  const QString& entryPoint);
    bool addShaderFromSourceFile (CGGLenum profileClass, const QString& file,
                                  const QString& entryPoint);

    bool link ();
    bool bind ();
    void release ();

    CGerror error () const;
    QString errorString () const;

private:
    struct Private;
    QScopedPointer<Private> d;
};
