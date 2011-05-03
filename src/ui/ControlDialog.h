
/**
 * @file ControlDialog.h
 * @brief ControlDialog definition
 */

#pragma once

#include <QDialog>

class ControlDialog : public QDialog
{
    Q_OBJECT

public:
    ControlDialog (QWidget* parent = NULL);
    virtual ~ControlDialog ();

private:
    struct Private;
    QScopedPointer<Private> d;
};
