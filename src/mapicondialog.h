/*
 * mapicondialog.h
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sourceforge.net/
 *
 */

#ifndef _MAPICONDIALOG_H_
#define _MAPICONDIALOG_H_

#include "ui_mapicondialog.h"

class MapIconDialog : public QDialog, public Ui::MapIconDialog
{
	Q_OBJECT

public:
	MapIconDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0)
		: QDialog(parent, name, modal, fl) 
	{
		setupUi(this);
		init();
	}
	~MapIconDialog() {}

public slots:
	virtual void apply();
	virtual void revert();
	virtual void init();
	virtual void destroy();
	virtual void setMapIcons( MapIcons * mapIcons );
	virtual void mapIconCombo_activated( int id );
	virtual void imagePenColor_clicked();
	virtual void imageBrushColor_clicked();
	virtual void highlightPenColor_clicked();
	virtual void highlightBrushColor_clicked();
	virtual void line0PenColor_clicked();
	virtual void line1PenColor_clicked();
	virtual void line2PenColor_clicked();
	virtual void walkPathPenColor_clicked();
	virtual void setupMapIconDisplay();

protected:
	MapIcons* m_mapIcons;
	MapIcon m_currentMapIcon;
	MapIcon m_currentMapIconBackup;
	MapIconType m_currentMapIconType;
};

#endif // _MAPICONDIALOG_H_