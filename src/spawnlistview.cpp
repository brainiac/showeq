/*
 * spawnlistview.cpp
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2001,2007 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *
 * Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net)
 * for use under the terms of the GNU General Public License,
 * incorporated herein by reference.
 *
 */

#include "pch.h"
#include "spawnlistview.h"

SpawnListView::SpawnListView(QWidget *parent)
	: QTreeView(parent)
{
	setupUi(this);


}

SpawnListView::~SpawnListView()
{

}
