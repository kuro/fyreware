
/**
 * @file PlaylistModel.h
 * @brief PlaylistModel definition
 */

#pragma once

#include <QAbstractTableModel>

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    PlaylistModel (QList<QUrl>& urls, QObject* parent = NULL);
    virtual ~PlaylistModel ();

    /**
     * @name table model
     */
    //@{
    int rowCount (const QModelIndex& parent = QModelIndex()) const;
    int columnCount (const QModelIndex& parent = QModelIndex()) const;
    QVariant headerData (int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole) const;
    QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const;
    //@}

private:
    struct Private;
    QScopedPointer<Private> d;
};
