
/**
 * @file Shader.cpp
 * @brief Shader implementation
 */

#include "Shader.moc"

#include <QFile>

struct Shader::Private
{
    CGGLenum profileClass;

    CGprofile profile;
    CGprogram program;

    Private (CGGLenum profileClass, Shader* q) :
        profileClass(profileClass)
    {
        Q_UNUSED(q);
    }
};

Shader::Shader (CGGLenum profileClass, QObject* parent) :
    QObject(parent),
    d(new Private(profileClass, this))
{
}

Shader::~Shader ()
{
}

bool Shader::compileSourceCode (CGcontext context,
                                const QString& code, const QString& entryPoint)
{
    d->profile = cgGLGetLatestProfile(d->profileClass);

    qDebug("profile: %s", cgGetProfileString(d->profile));

    cgGLSetOptimalOptions(d->profile);
    d->program = cgCreateProgram(context, CG_SOURCE, code.toLocal8Bit(),
                                 d->profile, entryPoint.toLocal8Bit(), NULL);
    return true;
}

bool Shader::compileSourceFile (CGcontext context,
                                const QString& file, const QString& entryPoint)
{
    QFile dev (file);
    if (!dev.open(QIODevice::ReadOnly)) {
        qWarning("%s", qPrintable(dev.errorString()));
        return false;
    }
    return compileSourceCode(context, dev.readAll(), entryPoint);
}

CGprofile Shader::profile () const
{
    return d->profile;
}

CGprogram Shader::program () const
{
    return d->program;
}
