
/**
 * @file ControlDialog.cpp
 * @brief ControlDialog implementation
 */

#include "ControlDialog.moc"

#include "ui/PlaylistWidget.h"
#include "ui/Player.h"

/**
 * @class ControlDialog
 *
 * @brief container for controls
 *
 * @bug How can I prevent this from being closed?  Or at least provide a way to
 * repoen it.
 */

struct ControlDialog::Private
{
    Player* player;
    PlaylistWidget* playlist;

    Private (ControlDialog* q) :
        player(new Player(q)),
        playlist(new PlaylistWidget(q))
    {
    }
};

ControlDialog::ControlDialog (QWidget* parent) :
    QDialog(parent, Qt::CustomizeWindowHint|Qt::WindowTitleHint),
    d(new Private(this))
{
    setSizeGripEnabled(true);
    setWindowTitle("Control");

    setWindowOpacity(0.8);

    setLayout(new QVBoxLayout);

    layout()->addWidget(d->player);
    layout()->addWidget(d->playlist);

    /// @todo not the best place to put this call
    QMetaObject::invokeMethod(d->playlist, "update", Qt::QueuedConnection);
}

ControlDialog::~ControlDialog ()
{
}
