#include "pch.h"
#include "categorydialog.h"

#include "qtcolorpicker\qtcolorpicker.h"

CategoryDialog::CategoryDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	delete ui.m_colorPicker;
	ui.m_colorPicker = new QtColorPicker(this);
	ui.m_colorPicker->setFixedHeight(37);
	ui.m_colorPicker->setStandardColors();
	ui.gridLayout->addWidget(ui.m_colorPicker, 0, 1, Qt::AlignVCenter);
	ui.gridLayout->setRowMinimumHeight(0, 100);
	ui.gridLayout->setRowStretch(0, 2);
}

CategoryDialog::~CategoryDialog()
{
}

void CategoryDialog::setName(QString text)
{
	ui.m_name->setText(text);
}

void CategoryDialog::setFilterIn(QString text)
{
	ui.m_filterIn->setText(text);
}

void CategoryDialog::setFilterOut(QString text)
{
	ui.m_filterOut->setText(text);
}

void CategoryDialog::setColor(QColor color)
{
	ui.m_colorPicker->setCurrentColor(color);
}

QString CategoryDialog::getName()
{
	return ui.m_name->text();
}

QString CategoryDialog::getFilterIn()
{
	return ui.m_filterIn->text();
}

QString CategoryDialog::getFilterOut()
{
	return ui.m_filterOut->text();
}

QColor CategoryDialog::getColor()
{
	return ui.m_colorPicker->currentColor();
}