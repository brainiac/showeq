/*
 * group.h
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 * 
 * Copyright 2001 Fee (fee@users.sourceforge.net). All Rights Reserved.
 *
 * Contributed to ShowEQ by fee (fee@users.sourceforge.net)
 * for use under the terms of the GNU General Public License,
 * incorporated herein by reference.
 *
 */

#ifndef _GUILD_H_
#define _GUILD_H_

#include <QObject>
#include <QString>
#include <vector>

#include "everquest.h"

//------------------------------
// GuildMgr
class GuildMgr : public QObject
{
	Q_OBJECT
	
public:
	GuildMgr(QString, QObject* parent = 0, const char* name = 0);
	~GuildMgr();
	
	QString guildIdToName(uint16_t);
	
public slots:
	void worldGuildList(const uint8_t*, size_t);
	void readGuildList();
	void guildList2text(QString);
	void listGuildInfo();
	
private:	
	void writeGuildList(const worldGuildListStruct*, size_t);

	std::vector<QString> m_guildMap;
	QString guildsFileName;
};

#endif // _GUILD_H_
