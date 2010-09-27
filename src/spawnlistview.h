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
};

#endif // SPAWNLISTVIEW_H
