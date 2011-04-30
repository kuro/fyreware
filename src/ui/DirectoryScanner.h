
/**
 * @file DirectoryScanner.h
 * @brief DirectoryScanner definition
 */

#pragma once

#include <QRunnable>
#include <QScopedPointer>

class PlaylistWidget;

class DirectoryScanner : public QRunnable
{
public:
    DirectoryScanner (const QString& path, PlaylistWidget* playlist);
    virtual ~DirectoryScanner ();

    virtual void run ();

protected:
    void scanFile (const QString& path);

private:
    struct Private;
    QScopedPointer<Private> d;
};
