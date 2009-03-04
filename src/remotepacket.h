/*
 *  remotepacket.h
 *  showeq
 *
 *  Created by Stephen Raub on 2/28/09.
 *
 */

#include <QString>
#include <QObject>
#include <QByteArray>

#include <QThread>
#include <QtNetwork/QTcpServer>
#include <QDataStream>

#include "packetcommon.h"
#include "packetinfo.h"
#include "packet.h"

class EQPacketOPCode;
class EQPacketOPCodeDB;

class QTcpSocket;

#define TEMP_BUFFER_SIZE 512

class RemotePacketServer : public QObject
{
	Q_OBJECT
	
public:
	RemotePacketServer(EQPacketOPCodeDB& zoneOPCodeDB, EQPacketOPCodeDB& worldOPCodeDB,
					   QObject* parent = NULL, const char *name = NULL);
	~RemotePacketServer();

	// connect a opcode handler to a received opcode
	bool connect2(const QString& opcodeName, const char* payloadType,
				  EQStreamPairs sp, uint8_t dir, EQSizeCheckType szt,
				  const QObject* receiver, const char* member);

	void start(uint16_t portNum);
	void stop();
	void reset();
	
	uint16_t getPort() { return m_port; }
	
	// receive a packet of data from a socket for processing
	void handlePacket(uint32_t type, const uint8_t* data, uint32_t length);
	void dispatchPacket(uint32_t type, const uint8_t* data, uint32_t length, EQDir dir);
	
signals:
	void connected(const QString& peer);

public slots:
	void processPackets();
	void newConnection();
	void clientDisconnected();
	
protected:
	uint16_t m_port;
	QTcpSocket* m_socket;
	QTcpServer* m_server;
	
	typedef QHash<void*, EQPacketDispatch*> Dispatcher;
	
	Dispatcher m_sendDispatchers;
	Dispatcher m_recvDispatchers;
	EQPacketOPCodeDB& m_zoneOPCodeDB;
	EQPacketOPCodeDB& m_worldOPCodeDB;
	
	uint32_t m_nextBufferSize;
	int32_t m_nextMessageType;
	
	static uint8_t s_tempBuffer[TEMP_BUFFER_SIZE];
};