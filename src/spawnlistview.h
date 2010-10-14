#ifndef SPAWNLISTVIEW_H
#define SPAWNLISTVIEW_H

#include <QTreeView>
#include "ui_spawnlistview.h"

using namespace Ui;

class SpawnListView : public QTreeView, public SpawnListViewClass
{
	Q_OBJECT

public:
	SpawnListView(QWidget *parent = 0);
	~SpawnListView();


	void setPreferenceName(const QString& preferenceName) { m_preferenceName = preferenceName; }
	QString getPreferenceName() const { return m_preferenceName; }

public slots:
	void saveColumnPreferences();
	void loadColumnPreferences();

private:
	QString		m_preferenceName;
};

#endif // SPAWNLISTVIEW_H
