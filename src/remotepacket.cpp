/*
 *  remotepacket.cpp
 *  showeq
 *
 *  Created by Stephen Raub on 2/28/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "pch.h"

#include "remotepacket.h"
#include "packet.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "packetinfo.h"
#include "diagnosticmessages.h"

// Packet structure
// uint32			- Total Length of packet
// uint32			- Message Id
// << DATA >>

//#define PACKET_INFO_DIAG 0

enum MessageId
{
	MSG_Invalid = -1,
	MSG_Hello = 0,
	MSG_SendPacket,
	MSG_ReceivePacket,
	MSG_TextOutput
};

struct TransferPacketStruct
{
	uint32_t opcode;
	uint32_t size;
	uint8_t data[0];
};

uint8_t RemotePacketServer::s_tempBuffer[TEMP_BUFFER_SIZE] = { 0 };

// RemotePacketServer implementation
RemotePacketServer::RemotePacketServer(EQPacketOPCodeDB& zoneOPCodeDB, EQPacketOPCodeDB& worldOPCodeDB,
									   QObject* parent, const char* name)
  : QObject(parent, name),
	m_zoneOPCodeDB(zoneOPCodeDB),
	m_worldOPCodeDB(worldOPCodeDB)
{
	m_socket = NULL;
	m_port = 0;
	m_nextMessageType = MSG_Invalid;
	m_nextBufferSize = 0;

	// create the tcp server with parent
	m_server = new QTcpServer(this);
	m_server->setMaxPendingConnections(1);
	connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

RemotePacketServer::~RemotePacketServer()
{
	if (m_socket)
		delete m_socket;

	if (!m_sendDispatchers.isEmpty())
	{
		Dispatcher::iterator it;
		for (it = m_sendDispatchers.begin(); it != m_sendDispatchers.end(); it++)
		{
			EQPacketDispatch* dispatch = it.value();
			if (dispatch != NULL)
				delete dispatch;
		}
		m_sendDispatchers.clear();
	}

	if (!m_recvDispatchers.isEmpty())
	{
		Dispatcher::iterator it;
		for (it = m_recvDispatchers.begin(); it != m_recvDispatchers.end(); it++)
		{
			EQPacketDispatch* dispatch = it.value();
			if (dispatch != NULL)
				delete dispatch;
		}
		m_recvDispatchers.clear();
	}
}

// Dispatcher framework, copied from packetstream.cpp.
// TODO: Factor out this code from here and packetstream.cpp into a new subclass.
bool RemotePacketServer::connect2(const QString& opcodeName, const char* payloadType,
		EQStreamPairs sp, uint8_t dir, EQSizeCheckType szt,
		const QObject* receiver, const char* member)
{
	const EQPacketOPCode* opcode = NULL;

	if (sp == SP_World)
		opcode = m_worldOPCodeDB.find(opcodeName);
	else
		opcode = m_zoneOPCodeDB.find(opcodeName);

	if (!opcode)
	{
		seqDebug("RemotePacketServer::connect2: Unknown opcode '%s' with payload type '%s'",
				 (const char*)opcodeName, payloadType);
		seqDebug("\tfor receiver '%s' of type '%s' to member '%s'", receiver->name(),
				 receiver->className(), member);
		return false;
	}

	// try to find a matching payload for this opcode
	EQPacketPayload* payload = NULL;
	EQPayloadListIterator pit(*opcode);
	while ((payload = pit.current()) != NULL)
	{
		// if all the parameters match, then use this payload
		if ((payload->typeName() == payloadType) &&	(payload->sizeCheckType() == szt))
			break;
		++pit;
	}

	// if no payload found, issue a warning
	if (!payload)
	{
		seqDebug("RemotePacketServer::connect2: Warning! opcode '%s' has no matching payload.", (const char*)opcodeName);
		seqDebug("\tdir 'any' payload '%s' szt '%d'", payloadType, szt);
		seqDebug("\tfor receiver '%s' of type '%s' to member '%s'", receiver->name(), receiver->className(), member);
		return false;
	}

	bool result = true;

	if (dir & DIR_Client)
	{
		result = assignDispatcher(&m_sendDispatchers, opcodeName, payload,
				receiver, member);
	}

	if (dir & DIR_Server)
	{
		result = result && assignDispatcher(&m_recvDispatchers, opcodeName, payload,
				receiver, member);
	}

	return result;
}

bool RemotePacketServer::assignDispatcher(Dispatcher* dispatcher, const QString& opcodeName, EQPacketPayload* payload,
			const QObject* receiver, const char* member)
{
	// attempt to find an existing dispatch
	EQPacketDispatch* dispatch = dispatcher->value((void*)payload);

	// if no existing dispatch was found, create one
	if (!dispatch)
	{
		QString dispatchName;
		dispatchName.sprintf("PacketDispatch:%s:%s:%d:%s:%d", (const char*)name(), (const char*)opcodeName,
							 payload->dir(), (const char*)payload->typeName(), payload->sizeCheckType());

		// create new dispatch object
		dispatch = new EQPacketDispatch(this, dispatchName);

		// insert dispatcher into dispatcher hash table
		dispatcher->insert((void*)payload, dispatch);
	}

	return dispatch->connect(receiver, member);
}

void RemotePacketServer::clientDisconnected()
{
	if (m_socket != NULL)
	{
		seqInfo("Disconnected from remote server at %s", (const char*)m_socket->peerAddress().toString());
		m_socket->close();
		m_socket->deleteLater();
		m_socket = NULL;
	}
}

void RemotePacketServer::newConnection()
{
	if (m_socket != NULL)
	{
	}
	else
	{
		m_socket = m_server->nextPendingConnection();
		connect(m_socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
		seqInfo("Connected to remote server at %s", (const char*)m_socket->peerAddress().toString());
	}
}

void RemotePacketServer::dispatchPacket(uint32_t type, const uint8_t* data, uint32_t length, EQDir dir)
{
	Dispatcher* dispatcher = (dir == DIR_Client) ? &m_sendDispatchers : &m_recvDispatchers;
	const EQPacketOPCode* opcode = NULL;
	bool unknown = true;


	opcode = m_zoneOPCodeDB.find(type);
	if (opcode == NULL)
	{
		opcode = m_worldOPCodeDB.find(type);
	}

	if (opcode != NULL)
	{
#ifdef PACKET_INFO_DIAG
		seqDebug("dispatchPacket: attempting to dispatch opcode [%s] %#04x '%s'",
				dir == DIR_Client ? "send" : "recv",
				opcode->opcode(), (const char*)opcode->name());
#endif

		EQPayloadListIterator pit(*opcode);
		EQPacketPayload* payload;
		bool found = false;
		while ((payload = pit.current()) != 0)
		{
			if (payload->match(data, length, dir))
			{
				found = true;
				unknown = false;

#if defined(PACKET_INFO_DIAG) && (PACKET_INFO_DIAG > 2)
				seqDebug("\tmatched payload, find dispatcher in dict (%d/%d)",
						 dispatcher->count(), dispatcher->size());
#endif
				EQPacketDispatch* dispatch = dispatcher->value((void*)payload);

				if (dispatch != NULL)
				{
#if defined(PACKET_INFO_DIAG) && (PACKET_INFO_DIAG > 2)
					seqDebug("\tactivating signal...");
#endif
					dispatch->activate(data, length, dir);
				}
				else
				{
#if defined(PACKET_INFO_DIAG)
					seqWarn("dispatchPacket: opcode [%s] %#04x '%s' has no dispatcher!",
							dir == DIR_Client ? "send" : "recv", opcode->opcode(),
							(const char*)opcode->name());
#endif
				}
			}
			++pit;
		}
#ifdef PACKET_PAYLOAD_SIZE_DIAG
		// TODO: Fill this in
#endif
	}
#if defined(PACKET_INFO_DIAG) && (PACKET_INFO_DIAG > 1)
	else
	{
		seqWarn("DispatchPacket(): buffer size %d opcode %04x not in opcodeDb",
				length, type);
	}
#endif
}


void RemotePacketServer::handlePacket(uint32_t type, const uint8_t* data, uint32_t length)
{
	// I suppose we should start first by selectively handling the individual
	// packet types. type represents an internal packet protocol type, not an
	// EQ Packet protocol type.

	// data contains the payload for the packet. For forwarded EQ packets, it
	// will contain a structure with opcode, size and data for the wrapped packet.
	switch (type)
	{
		case MSG_Hello:
		{
			// this message type is just a greeting from the client when the
			// connection is first established. Eventually it might provide
			// information but for now we won't do anything.
			break;
		}

		case MSG_SendPacket:
		{
			// this message type is for when the client sends a packet to the
			// server. The equivalent EQ_DIR would be DIR_Client.
			TransferPacketStruct* packet = (TransferPacketStruct*)data;
			//seqInfo("\e[0;32m" "SendPacket with Opcode %x, Length: %i" "\e[0m", packet->opcode, packet->size);

			dispatchPacket(packet->opcode, packet->data, packet->size, DIR_Client);
			break;
		}

		case MSG_ReceivePacket:
		{
			// this message is just like the one above, but occurs when the
			// client receives a packet from the server. This is equivalent
			// to Dir_Server
			TransferPacketStruct* packet = (TransferPacketStruct*)data;
			//seqInfo("\e[0;36m" "ReceivePacket with Opcode %x, Length: %i" "\e[0m", packet->opcode, packet->size);

			dispatchPacket(packet->opcode, packet->data, packet->size, DIR_Server);
			break;
		}

		case MSG_TextOutput:
		{
			// this message type contains a string that represents something we
			// should print out to the screen somewhere. It usually is MQ2
			// console messages but it doesn't have to be limited to that.
			break;
		}

		default:
			// other messages that haven't been implemented yet. Don't know what
			// we would put here for this, so just don't do anything!
			seqWarn("RemotePacketServer: Received an unrecognized message type: %i", (int)type);
			break;
	}
}

void RemotePacketServer::processPackets()
{
	if (!m_socket)
		return;

	while (m_socket->bytesAvailable() > 0)
	{
		QDataStream input(m_socket);
		input.setVersion(QDataStream::Qt_4_4);
		input.setByteOrder(QDataStream::LittleEndian);

		if (m_nextBufferSize == 0)
		{
			// make sure there is enough in the buffer to read the size and the
			// message id (2 ints)
			if (m_socket->bytesAvailable() < (2 * (int)sizeof(int)))
				return;

			input >> m_nextBufferSize;
			input >> m_nextMessageType;
		}

		if (m_socket->bytesAvailable() < m_nextBufferSize)
			return;

		uint8_t* buffer = &s_tempBuffer[0];

		// we should have a full buffer now. Lets process it.
		if (m_nextBufferSize >= TEMP_BUFFER_SIZE)
			buffer = new uint8_t[m_nextBufferSize];

		input.readRawData((char*)buffer, m_nextBufferSize);

		handlePacket(m_nextMessageType, buffer, m_nextBufferSize);

		if (m_nextBufferSize >= TEMP_BUFFER_SIZE)
			delete [] buffer;

		m_nextBufferSize = 0;
		m_nextMessageType = MSG_Invalid;
	}
}

void RemotePacketServer::start(uint16_t port)
{
	m_port = port;
	m_server->listen(QHostAddress::Any, port);
}

void RemotePacketServer::stop()
{
	// TODO: What should be done to stop?
}

void RemotePacketServer::reset()
{
	// TODO: What should be done to reset?
}
