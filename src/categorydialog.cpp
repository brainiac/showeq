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
	ui.gridLayout->addWidget(ui.m_colorPicker, 0, 1, Qt::AlignVCenter);
	ui.gridLayout->setRowMinimumHeight(0, 100);
	ui.gridLayout->setRowStretch(0, 2);
}

CategoryDialog::~CategoryDialog()
{

}
