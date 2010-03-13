/*
 * compass.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sf.net/
 *
 *  Copyright 2003-2007 by the respective ShowEQ Developers
 */

//
// NOTE: Trying to keep this file ShowEQ/Everquest independent to allow it
// to be reused for other Show{} style projects.  Any existing ShowEQ/EQ
// dependencies will be migrated out.
//

#include "pch.h"

#include "compass.h"

using namespace Qt;

Compass::Compass(QWidget *parent, const char *name)
  : QWidget(parent, name)
{
	m_ang = 0;
	m_dSpawnAngle = -1;
}

const QRect compass_rect(-3, 0, 40, 6);

QSize Compass::sizeHint() const // preferred size
{
	return QSize(110, 110);
}

QSizePolicy Compass::sizePolicy() const // size policy
{
	return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void Compass::calcTargetHeading()
{
	int deltaX, deltaY;
	double quadRads, quadDegs;

	deltaX = m_cPlayer.x() - m_cTarget.x();
	deltaY = (m_cPlayer.y() - m_cTarget.y()) * -1;

	quadRads = atan2((double)deltaY, (double)deltaX);

	// Convert radians to degrees
	quadDegs = (quadRads/(2 * M_PI)) * 360;

	// Account for stuff in the lower half of the circle
	if (quadDegs < 0)
	{
		quadDegs += 360.0;
	}

	m_dSpawnAngle = quadDegs;

	repaint(compassRect(), FALSE);
}

QRect Compass::compassRect() const
{
	QRect r(0,0,width(),height());
	r.moveBottomLeft (rect().bottomLeft());
	return r;
}

void Compass::setHeading(int32_t degrees)
{
	if (m_ang == ((360-degrees)+90))
		return;
	m_ang = (360-degrees)+90;
	repaint(compassRect(), FALSE);
	emit angleChanged(m_ang);
}

void Compass::setPos(int16_t x, int16_t y, int16_t z)
{
	m_cPlayer.setPoint(x, y, z);

	if (m_dSpawnAngle > 0)
	{
		calcTargetHeading();
	}
}

void Compass::setTargetPos(int x,int y, int z)
{
	// Recalc the mob heading
	m_cTarget.setPoint(x,y,z);

	calcTargetHeading();
}

void Compass::clearTarget()
{
	m_dSpawnAngle = -1;
}

void Compass::paintCompass(QPainter *p)
{
	QRect cr = compassRect();
	cr.addCoords(5, 5, -5, -5);
	int pwd2 = cr.width() >> 1;
	int pwd4 = cr.width() >> 2;
	int phd2 = cr.height() >> 1;
	int phd4 = cr.height() >> 2;

	p->setRenderHint(QPainter::Antialiasing, true);

	// Draw Compass Background
	//p->setBrush(QColor(64, 64, 64));
	//p->setPen(darkGray);
	//p->drawEllipse(0,0, cr.width(), cr.height());
	QConicalGradient coneGradient(0, 0, -90.0);
	coneGradient.setColorAt(0.0, Qt::darkGray);
	coneGradient.setColorAt(0.2, QColor(150, 150, 200));
	coneGradient.setColorAt(0.5, Qt::white);
	coneGradient.setColorAt(1.0, Qt::darkGray);

	p->setBrush(coneGradient);
	p->drawEllipse(0, 0, cr.width(), cr.height());


	p->setPen(gray);
	p->drawLine(0,phd2, cr.width(), phd2);
	p->drawLine(pwd2,0, pwd2, cr.height());
	p->drawLine(pwd4, phd4, cr.width() - pwd4, cr.height() - phd4);
	p->drawLine(cr.width() - pwd4, phd4, pwd4, cr.height() - phd4);
	p->translate(pwd2, phd2);

	p->rotate(-m_ang);
	p->setBrush(blue);
	p->setPen(blue);
	p->drawLine(0 - pwd4, 0, pwd2, 0);
	p->drawLine(0,0 - phd4, 0, phd4);
	p->setBrush(red);
	p->setPen(red);
	p->drawLine(0, 1, pwd2, 1);
	p->drawLine(0, -1, pwd4, -1);
	p->setBrush(blue);
	p->setPen(red);
	p->drawEllipse(0-5, 0-5, 10, 10);
	p->setBrush(red);
	p->setPen(red);
	p->drawEllipse(pwd2 - 2, -2, 4, 4);

	if (m_dSpawnAngle > 0)
	{
		p->resetXForm();
		p->translate(pwd2, phd2);
		p->rotate(-m_dSpawnAngle);
		p->setPen(green);
		p->setBrush(green);
		p->drawEllipse(pwd2 -2, -2, 4, 4);
	}
}

void Compass::paintEvent(QPaintEvent *e)
{
	QRect updateR = e->rect();
	QPainter p(this);
	if (updateR.intersects(compassRect()))
		paintCompass(&p);
}
