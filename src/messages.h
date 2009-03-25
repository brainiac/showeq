/*
 * messages.h
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2002-2003 Zaphod (dohpaz@users.sourceforge.net)
 *
 */

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include "message.h"
#include "messagefilter.h"

#include <stdint.h>

#include <QObject>
#include <QString>
#include <QList>

//----------------------------------------------------------------------
// forward declarations
class DateTimeMgr;

//----------------------------------------------------------------------
// MessageList
typedef QList<MessageEntry> MessageList;

//----------------------------------------------------------------------
// Messages
class Messages : public QObject
{
  Q_OBJECT

public:
	Messages(DateTimeMgr* dateTimeMgr, MessageFilters* messageFilters,
			 QObject* parent = 0, const char* name = 0);
	~Messages();

	static Messages* messages() { return s_messages; }
	const MessageList messageList() const { return m_messages; }

public slots:
	void addMessage(MessageType type, const QString& text, uint32_t color = ME_InvalidColor);
	void clear();

protected slots:
	void removedFilter(uint32_t mask, uint8_t filter);
	void addedFilter(uint32_t mask, uint8_t filterid, const MessageFilter& filter);

signals:
	void newMessage(const MessageEntry& message);
	void cleared();

protected:
	DateTimeMgr* m_dateTimeMgr;
	MessageFilters*	m_messageFilters;
	MessageList	m_messages;

	static Messages*	s_messages;
};

#endif // _MESSAGES_H_
