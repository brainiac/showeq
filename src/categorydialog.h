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

	// Setters!
	void setName(QString text);
	void setFilterIn(QString text);
	void setFilterOut(QString text);
	void setColor(QColor color);

	// Getters!
	QString getName();
	QString getFilterIn();
	QString getFilterOut();
	QColor getColor();


private:
	Ui::CategoryDialogClass ui;
};

#endif // CATEGORYDIALOG_H
