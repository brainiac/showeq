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
#include "diagnosticmessages.h"

using namespace Qt;

SEQWindow::SEQWindow(const QString prefName, const QString caption,
					 QWidget* parent, const char* name, Qt::WFlags f)
  : QDockWidget(name, parent, f),
	m_preferenceName(prefName)
{
	//seqDebug("Creating SEQWindow named %s with caption %s", name, (const char*)caption);
	setName(name);

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

void SEQWindow::setDockable(bool enabled)
{
	if (enabled)
	{
		setAllowedAreas(Qt::AllDockWidgetAreas);
	}
	else
	{
		setAllowedAreas(Qt::NoDockWidgetArea);
		undock();
	}
	restoreSize();
}

void SEQWindow::setCaption(const QString& text)
{
	// set the caption
	//Q3DockWindow::setCaption(text);
	//setName((const char*)caption());
	setWindowTitle(text);

	// set the preference
	pSEQPrefs->setPrefString("Caption", preferenceName(), text);
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
		setFont(font);
	}
}

void SEQWindow::restoreSize()
{
	// TODO: FIXME
	if (!isFloating())
	{
		QSize s = pSEQPrefs->getPrefSize("DockFixedExtent", preferenceName(), size());
		resize(s);
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
	if (isFloating())
	{
		// retrieve the saved position information
		QPoint p = pSEQPrefs->getPrefPoint("WindowPos", preferenceName(), pos());

		// Move window to new position
		move(p);
	}
}

void SEQWindow::savePrefs()
{
	if (pSEQPrefs->getPrefBool("SavePosition", "Interface", true))
	{
		// TODO: FIXME
		if (!isFloating())
		{
			pSEQPrefs->setPrefSize("DockFixedExtent", preferenceName(), size());
		}
		else
		{
			// save the windows size and position information
			pSEQPrefs->setPrefSize("WindowSize", preferenceName(), size());
			pSEQPrefs->setPrefPoint("WindowPos", preferenceName(), pos());
		}
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
