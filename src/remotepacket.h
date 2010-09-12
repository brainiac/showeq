/*
 *  remotepacket.h
 *  showeq
 *
 *  Created by Stephen Raub on 2/28/09.
 *
 */

#ifndef __REMOTEPACKET_H__
#define __REMOTEPACKET_H__

#include <QtNetwork/QTcpServer>
#include <QThread>
#include <QTimer>

#include "packetcommon.h"
#include "packetinfo.h"
#include "packet.h"

#include "datasource.h"

class EQPacketOPCode;
class EQPacketOPCodeDB;

class QTcpSocket;

#define TEMP_BUFFER_SIZE 512

class RemotePacketServer : public DataSource
{
	Q_OBJECT

public:
	RemotePacketServer(EQPacketOPCodeDB& zoneOPCodeDB, EQPacketOPCodeDB& worldOPCodeDB,
					   QObject* parent = NULL);
	virtual ~RemotePacketServer();

	// connect a opcode handler to a received opcode
	virtual bool connectReceiver(const QString& opcodeName, const QString& payloadType,
		EQStreamPairs sp, uint8_t dir, EQSizeCheckType szt,
		const QObject* receiver, const char* member);

	//void start(uint16_t portNum);
	virtual void start();
	virtual void stop();

	virtual void update();

	uint16_t getPort() { return m_port; }

	// receive a packet of data from a socket for processing
	void handlePacket(uint32_t type, const uint8_t* data, uint32_t length);
	void dispatchPacket(uint32_t type, const uint8_t* data, uint32_t length, EQDir dir);

signals:
	void connected(const QString& peer);

public slots:
	void newConnection();
	void clientDisconnected();

protected:
	typedef QHash<void*, EQPacketDispatch*> Dispatcher;

	bool assignDispatcher(Dispatcher* dispatcher, const QString& opcodeName, EQPacketPayload* payload,
			const QObject* receiver, const char* member);

	uint16_t m_port;
	QTcpSocket* m_socket;
	QTcpServer* m_server;

	Dispatcher m_sendDispatchers;
	Dispatcher m_recvDispatchers;
	EQPacketOPCodeDB& m_zoneOPCodeDB;
	EQPacketOPCodeDB& m_worldOPCodeDB;

	uint32_t m_nextBufferSize;
	int32_t m_nextMessageType;

	static uint8_t s_tempBuffer[TEMP_BUFFER_SIZE];
};

#endif // __REMOTEPACKET_H__