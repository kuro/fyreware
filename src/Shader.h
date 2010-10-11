
/**
 * @file Shader.h
 * @brief Shader definition
 */

#pragma once

#include <QObject>

#include <Cg/cgGL.h>

class Shader : public QObject
{
    Q_OBJECT

public:
    Shader (CGGLenum profileClass, QObject* parent = NULL);
    virtual ~Shader ();

    CGprofile profile () const;
    CGprogram program () const;

    bool compileSourceCode (CGcontext context,
                            const QString& code, const QString& entryPoint);
    bool compileSourceFile (CGcontext context,
                            const QString& file, const QString& entryPoint);

private:
    struct Private;
    QScopedPointer<Private> d;
};
