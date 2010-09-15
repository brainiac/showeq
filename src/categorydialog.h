#ifndef CATEGORYDIALOG_H
#define CATEGORYDIALOG_H

#include <QDialog>
#include "ui_categorydialog.h"

class CategoryDialog : public QDialog
{
	Q_OBJECT

public:
	CategoryDialog(QWidget *parent = 0);
	~CategoryDialog();

private:
	Ui::CategoryDialogClass ui;
};

#endif // CATEGORYDIALOG_H
