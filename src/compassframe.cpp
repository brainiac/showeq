/*
 * compassframe.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sf.net/
 *
 *  Copyright 2003-2007 by the respective ShowEQ Developers
 */

#include <QVBoxLayout>
#include <QLabel>

#include "main.h"
#include "compassframe.h"

using namespace Qt;

CompassFrame::CompassFrame(Player* player, QWidget* parent, const char* name)
  : SEQWindow("Compass", "ShowEQ - Compass", parent, name)
{
	// create compass widget
	m_compass = new Compass (this, "compass");
	m_compass->setFixedWidth(120);
	m_compass->setFixedHeight(120);

	// Create the x: label
	QLabel *labelx = new QLabel(showeq_params->retarded_coords ? "E/W:" : "X:");
	labelx->setFixedHeight(labelx->sizeHint().height());
	labelx->setAlignment(AlignLeft | AlignVCenter);
      
	// Create the xpos label
	m_x = new QLabel("----");
	m_x->setFixedHeight(m_x->sizeHint().height());
	m_x->setAlignment(AlignRight | AlignVCenter);
		
	// Create the y: label
	QLabel *labely = new QLabel(showeq_params->retarded_coords ? "N/S:" : "Y:");
	labely->setFixedHeight(labely->sizeHint().height());
	labely->setAlignment(AlignLeft | AlignVCenter);
			
	// Create the ypos label
	m_y = new QLabel("----");
	m_y->setFixedHeight(m_y->sizeHint().height());
	m_y->setAlignment(AlignRight | AlignVCenter);
		
	// Create the z: label
	QLabel *labelz = new QLabel("Z:");
	labelz->setFixedHeight(labelz->sizeHint().height());
	labelz->setAlignment(AlignLeft | AlignVCenter);
	
	// Create the zpos label
	m_z = new QLabel("----");
	m_z->setFixedHeight(m_z->sizeHint().height());      
	m_z->setAlignment(AlignRight | AlignVCenter);
	
	QHBoxLayout* coordsBoxLayout = new QHBoxLayout();
	coordsBoxLayout->setSpacing(1);
	coordsBoxLayout->setMargin(0);
	if (showeq_params->retarded_coords)
	{
		coordsBoxLayout->addWidget(labely);
		coordsBoxLayout->addWidget(m_y);
		coordsBoxLayout->addWidget(labelx);
		coordsBoxLayout->addWidget(m_x);
	}
	else
	{
		coordsBoxLayout->addWidget(labelx);
		coordsBoxLayout->addWidget(m_x);
		coordsBoxLayout->addWidget(labely);
		coordsBoxLayout->addWidget(m_y);
	}
	coordsBoxLayout->addWidget(labelz);
	coordsBoxLayout->addWidget(m_z);
	
	QWidget* coordsBox = new QWidget();
	coordsBox->setLayout(coordsBoxLayout);
	
	QVBoxLayout* pLayout = new QVBoxLayout();
	pLayout->setMargin(0);
	pLayout->setSpacing(0);
	pLayout->setAlignment(AlignVCenter | AlignCenter);
	pLayout->addWidget(m_compass);
	pLayout->addWidget(coordsBox);
	
	QWidget* thisWidget = new QWidget();
	thisWidget->setLayout(pLayout);
	setWidget(thisWidget);
	
	// connect signals
	connect(player, SIGNAL(posChanged(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int32_t)), 
			this, SLOT(posChanged(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int32_t)));
	
	// initialize compass
	m_compass->setPos(player->x(), player->y(), player->z());
	m_compass->setHeading(player->headingDegrees());
}

CompassFrame::~CompassFrame()
{
}

void CompassFrame::selectSpawn(const Item* item)
{
	if (item)
		m_compass->setTargetPos(item->x(), item->y(), item->z());
	else
		m_compass->clearTarget();
}

void CompassFrame::posChanged(int16_t x, int16_t y, int16_t z,
							  int16_t deltaX, int16_t deltaY, int16_t deltaZ,
							  int32_t heading)
{
	// set compass info
	m_compass->setPos(x, y, z);
	m_compass->setHeading(heading);
	
	// set position labels
	m_x->setText(QString::number(x));
	m_y->setText(QString::number(y));
	m_z->setText(QString::number(z));
}

#ifndef QMAKEBUILD
#include "compassframe.moc"
#endif

