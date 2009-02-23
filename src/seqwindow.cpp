/*
 * seqwindow.cpp
 * 
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2001-2003,2007 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *
 * Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net) 
 * for use under the terms of the GNU General Public License, 
 * incorporated herein by reference.
 *
 */

#include "seqwindow.h"
#include "main.h"

using namespace Qt;

SEQWindow::SEQWindow(const QString prefName, const QString caption,
					 QWidget* parent, const char* name, Qt::WFlags f)
  : QDockWidget(name, parent, f),
	m_preferenceName(prefName)
{
	// set the windows caption
	setWindowTitle(pSEQPrefs->getPrefString("Caption", preferenceName(), caption));

	// windows default to resizable
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	
	// windows default to be closable when not docked
	setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
	
	// restore the font
	restoreFont();
}

SEQWindow::~SEQWindow()
{
}

QMenu* SEQWindow::menu()
{
	return 0;
}

void SEQWindow::dock()
{
	setFloating(false);
}

void SEQWindow::undock()
{
	setFloating(true);
}

void SEQWindow::setDockEnabled(bool enabled)
{	
	if (enabled)
		setAllowedAreas(Qt::AllDockWidgetAreas);
	else
		setAllowedAreas(Qt::NoDockWidgetArea);
}

void SEQWindow::removeDockingFeatures()
{
//	setFeatures(QDockWidget::NoDockWidgetFeatures);
//	setTitleBarWidget(new QWidget());
}

void SEQWindow::setCaption(const QString& text)
{
	// set the caption
	//Q3DockWindow::setCaption(text);
	setName((const char*)caption());
	
	// set the preference
	pSEQPrefs->setPrefString("Caption", preferenceName(), caption());
}

void SEQWindow::setWindowFont(const QFont& font)
{
	// set the font preference
	pSEQPrefs->setPrefFont("Font", preferenceName(), font);
	
	// restore the font to the preference
	restoreFont();
}

void SEQWindow::restoreFont()
{
	// set the applications default font
	if (pSEQPrefs->isPreference("Font", preferenceName()))
	{
		// use the font specified in the preferences
		QFont font = pSEQPrefs->getPrefFont("Font", preferenceName());
		setFont( font);
	}
}

void SEQWindow::restoreSize()
{
	// TODO: FIXME
	if (!isFloating())
	{
		//QSize s = pSEQPrefs->getPrefSize("DockFixedExtent", preferenceName(), fixedExtent());
		////setFixedExtentWidth(s.width());
		//setFixedExtentHeight(s.height());
	}
	else
	{
		// retrieve the saved size information
		QSize s = pSEQPrefs->getPrefSize("WindowSize", preferenceName(), size());
		
		resize(s);
	}
	
	if (pSEQPrefs->getPrefBool("DockVisible", preferenceName(), !isHidden()))
		show();
	else
		hide();
}

void SEQWindow::restorePosition()
{
	// TODO: FIXME
	if (!isFloating())
	{
		//setNewLine(pSEQPrefs->getPrefBool("DockNewLine", preferenceName(), newLine()));
		//setOffset(pSEQPrefs->getPrefInt("DockOffset", preferenceName(), offset()));
	}
	else
	{
		// retrieve the saved position information
		QPoint p = pSEQPrefs->getPrefPoint("WindowPos", preferenceName(), pos());
		
		// Move window to new position
		move(p);
	}
}

void SEQWindow::savePrefs(void)
{
	if (pSEQPrefs->getPrefBool("SavePosition", "Interface", true))
	{
		// TODO: FIXME
#if 0
		if (place() == InDock)
		{
			pSEQPrefs->setPrefBool("DockNewLine", preferenceName(), newLine());
			pSEQPrefs->setPrefInt("DockOffset", preferenceName(), offset());
			pSEQPrefs->setPrefSize("DockFixedExtent", preferenceName(), fixedExtent());
		}
		else
		{
			// save the windows size and position information
			pSEQPrefs->setPrefSize("WindowSize", preferenceName(), size());
			pSEQPrefs->setPrefPoint("WindowPos", preferenceName(), pos());
		}
#endif
		pSEQPrefs->setPrefBool("DockVisible", preferenceName(), !isHidden());
	}
}

void SEQWindow::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == RightButton)
	{
		QMenu* popupMenu = menu();
		if (popupMenu)
			popupMenu->popup(mapToGlobal(e->pos()));
		else
			/*Q3DockWindow::*/mousePressEvent(e);
	}
	else
		/*Q3DockWindow::*/mousePressEvent(e);
}

#ifndef QMAKEBUILD
#include "seqwindow.moc"
#endif
