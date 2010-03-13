/*
 * packet.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://www.sourceforge.net/projects/seq
 *
 *  Copyright 2000-2004 by the respective ShowEQ Developers
 *  Portions Copyright 2001-2004,2007 Zaphod (dohpaz@users.sourceforge.net).
 */

#include "pch.h"

/* Implementation of Packet class */

#ifdef __FreeBSD__
#include "packet.h"
#endif

#include "everquest.h"
#include "packet.h"
#ifndef _WINDOWS		/* No Support for Live Packet Capture in windows yet */
#include "packetcapture.h"
#include "packetformat.h"
#include "packetstream.h"
#include "vpacket.h"
#endif
#include "packetinfo.h"
#include "everquest.h"
#include "diagnosticmessages.h"
#include "remotepacket.h"

//----------------------------------------------------------------------
// Macros

//#define DEBUG_PACKET
//#undef DEBUG_PACKET

// The following defines are used to diagnose packet handling behavior
// this define is used to diagnose packet processing (in dispatchPacket mostly)
//#define PACKET_PROCESS_DIAG 3
// verbosity level 0-n

// this define is used to diagnose cache handling (in dispatchPacket mostly)
//#define PACKET_CACHE_DIAG 3
// verbosity level (0-n)

// diagnose opcode DB issues
//#define  PACKET_OPCODEDB_DIAG 1

// diagnose structure size changes
//#define PACKET_PAYLOAD_SIZE_DIAG 1

// Packet version is a unique number that should be bumped every time packet
// structure (ie. encryption) changes.  It is checked by the VPacket feature
// (currently the date of the last packet structure change)
#define PACKETVERSION  40101

//----------------------------------------------------------------------
// constants

const in_port_t WorldServerGeneralPort = 9000;
const in_port_t WorldServerChatPort = 9876;
const in_port_t WorldServerChat2Port = 9875; // xgame tells, mail
const in_port_t LoginServerMinPort = 15900;
const in_port_t LoginServerMaxPort = 15910;
const in_port_t ChatServerPort = 5998;

//----------------------------------------------------------------------
// Here begins the code


//----------------------------------------------------------------------
// EQPacket class methods

/* EQPacket Class - Sets up packet capturing */

////////////////////////////////////////////////////
// Constructor
EQPacket::EQPacket(const QString& worldopcodesxml,
		const QString& zoneopcodesxml,
		uint16_t arqSeqGiveUp,
		QString device,
		QString ip,
		QString mac_address,
		bool realtime,
		bool sessionTrackingFlag,
		bool recordPackets,
		int playbackPackets,
		int8_t playbackSpeed,
		bool useRemote,
		uint32_t portNum,
		QObject * parent, const char *name)
  : QObject (parent, name),
#ifndef _WINDOWS
	m_packetCapture(NULL),
	m_vPacket(NULL),
#endif
	m_timer(NULL),
	m_busy_decoding(false),
	m_useRemote(useRemote),
	m_remotePort(portNum),
	m_arqSeqGiveUp(arqSeqGiveUp),
	m_device(device),
	m_ip(ip),
	m_mac(mac_address),
	m_realtime(realtime),
	m_session_tracking(sessionTrackingFlag),
	m_recordPackets(recordPackets),
	m_playbackPackets(playbackPackets),
	m_playbackSpeed(playbackSpeed)
{
	if (m_useRemote)
	{
		m_playbackPackets = PLAYBACK_REMOTE;
	}

	// create the packet type db
	m_packetTypeDB = new EQPacketTypeDB();
#ifdef PACKET_OPCODEDB_DIAG
	m_packetTypeDB->list();
#endif

	// create the world opcode db (with hash size of 29)
	m_worldOPCodeDB = new EQPacketOPCodeDB(29);
	// load the world opcode db
	if (!m_worldOPCodeDB->load(*m_packetTypeDB, worldopcodesxml))
		seqFatal("Error loading '%s'!", (const char*)worldopcodesxml);
#ifdef PACKET_OPCODEDB_DIAG
	m_worldOPCodeDB->list();
#endif

	// create the zone opcode db (with hash size of 211)
	m_zoneOPCodeDB = new EQPacketOPCodeDB(211);
	// load the zone opcode db
	if (!m_zoneOPCodeDB->load(*m_packetTypeDB, zoneopcodesxml))
		seqFatal("Error loading '%s'!", (const char*)zoneopcodesxml);
#ifdef PACKET_OPCODEDB_DIAG
	m_zoneOPCodeDB->list();
#endif

	// ** Setup the data streams **
#ifndef _WINDOWS
	// Setup client -> world stream
	m_client2WorldStream = new EQPacketStream(client2world, DIR_Client, m_arqSeqGiveUp, *m_worldOPCodeDB, this, "client2world");
	connectStream(m_client2WorldStream);

	// Setup world -> client stream
	m_world2ClientStream = new EQPacketStream(world2client, DIR_Server, m_arqSeqGiveUp, *m_worldOPCodeDB, this, "world2client");
	connectStream(m_world2ClientStream);

	// Setup client -> zone stream
	m_client2ZoneStream = new EQPacketStream(client2zone, DIR_Client, m_arqSeqGiveUp, *m_zoneOPCodeDB, this, "client2zone");
	connectStream(m_client2ZoneStream);

	// Setup zone -> client stream
	m_zone2ClientStream = new EQPacketStream(zone2client, DIR_Server, m_arqSeqGiveUp, *m_zoneOPCodeDB, this, "zone2client");
	connectStream(m_zone2ClientStream);

	// Initialize convenient streams array
	m_streams[client2world] = m_client2WorldStream;
	m_streams[world2client] = m_world2ClientStream;
	m_streams[client2zone] = m_client2ZoneStream;
	m_streams[zone2client] = m_zone2ClientStream;
#endif

	// no client/server ports yet
	m_clientPort = 0;
	m_serverPort = 0;

	struct hostent *he;
	struct in_addr  ia;
	if (m_ip.isEmpty() && m_mac.isEmpty())
		seqFatal("No address specified");

	/* remote capture doesn't use the ip based packet capture system.
	 * so don't bother setting it up. */
#ifndef _WINDOWS
	if (!m_ip.isEmpty() && m_playbackPackets != PLAYBACK_REMOTE)
	{
		/* Substitute "special" IP which is interpreted
		 to set up a different filter for picking up new sessions */

		if (m_ip == "auto")
			inet_aton (AUTOMATIC_CLIENT_IP, &ia);
		else if (inet_aton (m_ip, &ia) == 0)
		{
			he = gethostbyname(m_ip);
			if (!he)
				seqFatal("Invalid address; %s", (const char*)m_ip);

			memcpy (&ia, he->h_addr_list[0], he->h_length);
		}
		m_client_addr = ia.s_addr;
		m_ip = inet_ntoa(ia);

		if (m_ip ==  AUTOMATIC_CLIENT_IP)
		{
			m_detectingClient = true;
			seqInfo("Listening for first client seen.");
		}
		else
		{
			m_detectingClient = false;
			seqInfo("Listening for client: %s", (const char*)m_ip);
		}
	}
#endif

	m_remoteServer = NULL;
	if (m_playbackPackets == PLAYBACK_REMOTE)
	{
		m_remoteServer = new RemotePacketServer(*m_zoneOPCodeDB, *m_worldOPCodeDB, this, "RemotePacketServer");
		m_remoteServer->start(m_remotePort);
		seqInfo("Remote capture mode enabled. ShowEQ will listen for a connection on port %i", m_remotePort);
	}
#ifndef _WINDOWS
	else if (m_playbackPackets == PLAYBACK_OFF)
	{
		// create the pcap object and initialize, either with MAC or IP
		m_packetCapture = new PacketCaptureThread();

		if (m_mac.length() == 17)
			m_packetCapture->start(m_device, m_mac, m_realtime, MAC_ADDRESS_TYPE);
		else
			m_packetCapture->start(m_device, m_ip, m_realtime, IP_ADDRESS_TYPE);

		emit filterChanged();
	}
	else if (m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
	{
		// Create the pcap object and initialize with the file input given
		m_packetCapture = new PacketCaptureThread();

		const char* filename = pSEQPrefs->getPrefString("Filename", "VPacket");

		m_packetCapture->startOffline(filename, m_playbackSpeed);
		seqInfo("Playing back packets from '%s' at speed '%d'", filename, m_playbackSpeed);
	}
#endif

	// Flag session tracking properly on streams
	session_tracking(sessionTrackingFlag);

#ifndef _WINDOWS
	// if running setuid root, then give up root access, since the PacketCapture
	// is the only thing that needed it.
	if ((geteuid() == 0) && (getuid() != geteuid()))
		setuid(getuid());
#endif

	/* Create timer object */
	m_timer = new QTimer(this);

	if (m_playbackPackets == PLAYBACK_OFF ||
		m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP ||
		m_playbackPackets == PLAYBACK_REMOTE)
	{
		// Normal pcap packet handler
		connect(m_timer, SIGNAL(timeout()), this, SLOT(processPackets()));
	}
#ifndef _WINDOWS
	else
	{
		// Special internal playback handler
		connect(m_timer, SIGNAL(timeout()), this, SLOT(processPlaybackPackets()));
	}

	/* setup VPacket */
	m_vPacket = NULL;

	// First param to VPacket is the filename
	// Second param is playback speed:  0 = fast as poss, 1 = 1X, 2 = 2X etc
	if (pSEQPrefs->isPreference("Filename", "VPacket"))
	{
		const char *filename = pSEQPrefs->getPrefString("Filename", "VPacket");

		if (m_recordPackets)
		{
			m_vPacket = new VPacket(filename, 1, true);
			// Must appear befire next call to getPrefString, which uses a static string
			seqInfo("Recording packets to '%s' for future playback", filename);

			if (!pSEQPrefs->getPrefString("FlushPackets", "VPacket").isEmpty())
				m_vPacket->setFlushPacket(true);
		}
		else if (m_playbackPackets == PLAYBACK_FORMAT_SEQ)
		{
			m_vPacket = new VPacket(filename, 1, false);
			m_vPacket->setCompressTime(pSEQPrefs->getPrefInt("CompressTime", "VPacket", 0));
			m_vPacket->setPlaybackSpeed(m_playbackSpeed);

			seqInfo("Playing back packets from '%s' at speed '%d'", filename, m_playbackSpeed);
		}
	}
	else if (m_playbackPackets != PLAYBACK_REMOTE)
	{
		m_recordPackets = 0;
		m_playbackPackets = PLAYBACK_OFF;
	}
#endif
}

////////////////////////////////////////////////////
// Destructor
EQPacket::~EQPacket()
{
#ifndef _WINDOWS
	if (m_packetCapture != NULL)
	{
		// stop any packet capture
		m_packetCapture->stop();

		// delete the object
		delete m_packetCapture;
	}
#endif

	if (m_remoteServer != NULL)
	{
		// stop the packet capture server
		m_remoteServer->stop();

		// delete the object
		delete m_remoteServer;
	}

#ifndef _WINDOWS
	// try to close down VPacket cleanly
	if (m_vPacket != NULL)
	{
		// make sure any data is flushed to the file
		m_vPacket->Flush();

		// delete VPacket
		delete m_vPacket;
	}
#endif

	if (m_timer != NULL)
	{
		// make sure the timer is stopped
		m_timer->stop();

		// delete the timer
		delete m_timer;
	}

	resetEQPacket();

#ifndef _WINDOWS
	delete m_client2WorldStream;
	delete m_world2ClientStream;
	delete m_client2ZoneStream;
	delete m_zone2ClientStream;
#endif

	if (m_packetTypeDB)
	{
		delete m_packetTypeDB;
	}

	if (m_zoneOPCodeDB)
	{
		delete m_zoneOPCodeDB;
	}

	if (m_worldOPCodeDB)
	{
		delete m_worldOPCodeDB;
	}
}

/* Start the timer to process packets */
void EQPacket::start(int delay)
{
#ifdef DEBUG_PACKET
	debug("start()");
#endif /* DEBUG_PACKET */
	m_timer->start(delay, false);
}

/* Stop the timer to process packets */
void EQPacket::stop()
{
#ifdef DEBUG_PACKET
	debug("stop()");
#endif /* DEBUG_PACKET */
	m_timer->stop();
}

/* Reads packets and processes waiting packets */
void EQPacket::processPackets()
{
	/* Make sure we are not called while already busy */
	if (m_busy_decoding)
		return;

	if (m_playbackPackets == PLAYBACK_REMOTE)
	{
		m_remoteServer->processPackets();
	}
#ifndef _WINDOWS
	else
	{
		/* Set flag that we are busy decoding */
		m_busy_decoding = true;

		unsigned char buffer[BUFSIZ];
		short size;

		/* fetch them from pcap */
		while ((size = m_packetCapture->getPacket(buffer)))
		{
			/* Now.. we know the rest is an IP udp packet concerning the
			 * host in question, because pcap takes care of that.
			 */

			/* Now we assume its an everquest packet */
			if (m_recordPackets)
			{
				time_t now = time(NULL);
				m_vPacket->Record((const char *) buffer, size, now, PACKETVERSION);
			}

			/* take the raw packet and dispatch it to the proper stream */
			dispatchPacket(size - sizeof(struct ether_header), (unsigned char *)buffer + sizeof(struct ether_header));
		}

	}
	/* Clear decoding flag */
	m_busy_decoding = false;
#endif
}

////////////////////////////////////////////////////
// Reads packets and processes waiting packets from playback file
void EQPacket::processPlaybackPackets()
{
#ifdef DEBUG_PACKET
	//   debug ("processPackets()");
#endif /* DEBUG_PACKET */
	/* Make sure we are not called while already busy */
	if (m_busy_decoding)
		return;

#ifndef _WINDOWS
	/* Set flag that we are busy decoding */
	m_busy_decoding = true;

	unsigned char  buffer[8192];
	int            size;

	/* in packet playback mode fetch packets from VPacket class */
	time_t now;
	int timein = mTime();
	int i = 0;

	long version = PACKETVERSION;

	// decode packets from the playback buffer
	do
	{
		size = m_vPacket->Playback((char *)buffer, sizeof(buffer), &now, &version);

		if (size)
		{
			i++;

			if (PACKETVERSION == version)
			{
				dispatchPacket(size - sizeof(ether_header), (unsigned char *)buffer + sizeof(ether_header));
			}
			else
			{
				seqWarn("Error:  The version of the packet stream has " \
						"changed since '%s' was recorded - disabling playback",
						m_vPacket->getFileName());

				// stop the timer, nothing more can be done...
				stop();
				break;
			}
		}
		else
			break;
	} while (mTime() - timein < 100);

	// check if we've reached the end of the recording
	if (m_vPacket->endOfData())
	{
		seqInfo("End of playback file '%s' reached. Playback Finished!", m_vPacket->getFileName());

		// stop the timer, nothing more can be done...
		stop();
	}

	/* Clear decoding flag */
	m_busy_decoding = false;
#endif
}

#ifndef _WINDOWS
/////////////////////////////////////////////////////////
// Connect the given stream's signals to the proper slots
void EQPacket::connectStream(EQPacketStream* stream)
{
	// Packet logging
	switch (stream->streamID())
	{
		case zone2client:
		case client2zone:
			// Zone server stream
			connect(stream, SIGNAL(rawPacket(const uint8_t*, size_t, uint8_t, uint16_t)),
					this,	SIGNAL(rawZonePacket(const uint8_t*, size_t, uint8_t, uint16_t)));

			connect(stream,	SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
					this,	SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));

			connect(stream,	SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
					this,	SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
			break;

		case world2client:
		case client2world:
			// World server stream
			connect(stream,	SIGNAL(rawPacket(const uint8_t*, size_t, uint8_t, uint16_t)),
					this,	SIGNAL(rawWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t)));

			connect(stream,	SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
					this,	SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));

			connect(stream,	SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
					this,	SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
			break
			;
		default:
			return;
	}

	// Debugging
	connect(stream,	SIGNAL(cacheSize(int, int)),
			this,	SIGNAL(cacheSize(int, int)));
	connect(stream,	SIGNAL(seqReceive(int, int)),
			this,	SIGNAL(seqReceive(int, int)));
	connect(stream,	SIGNAL(seqExpect(int, int)),
			this,	SIGNAL(seqExpect(int, int)));
	connect(stream,	SIGNAL(numPacket(int, int)),
			this,	SIGNAL(numPacket(int, int)));

	// Session handling
	connect(stream,	SIGNAL(sessionTrackingChanged(uint8_t)),
			this,	SIGNAL(sessionTrackingChanged(uint8_t)));
	connect(stream,	SIGNAL(lockOnClient(in_port_t, in_port_t)),
			this,	SLOT(lockOnClient(in_port_t, in_port_t)));
	connect(stream,	SIGNAL(closing(uint32_t, EQStreamID)),
			this,	SLOT(closeStream(uint32_t, EQStreamID)));
	connect(stream,	SIGNAL(sessionKey(uint32_t, EQStreamID, uint32_t)),
			this,	SLOT(dispatchSessionKey(uint32_t, EQStreamID, uint32_t)));
	connect(stream,	SIGNAL(maxLength(int, int)),
			this,	SIGNAL(maxLength(int, int)));
}

////////////////////////////////////////////////////
// This function decides the fate of the Everquest packet
// and dispatches it to the correct packet stream for handling function
void EQPacket::dispatchPacket(int size, unsigned char *buffer)
{
#ifdef DEBUG_PACKET
	debug("EQPacket::dispatchPacket()");
#endif /* DEBUG_PACKET */

	// Create an object to parse the packet
	EQUDPIPPacketFormat packet(buffer, size, false);

	dispatchPacket(packet);

	// signal a new packet. This has to be at the end so that the session is
	// filled in if possible, so that it can report on crc errors properly
	emit newPacket(packet);
}

void EQPacket::dispatchPacket(EQUDPIPPacketFormat& packet)
{
	// Detect client by world server port traffic...
	if (m_detectingClient && packet.getSourcePort() == WorldServerGeneralPort)
	{
		m_ip = packet.getIPv4DestA();
		m_client_addr = packet.getIPv4DestN();
		m_detectingClient = false;
		emit clientChanged(m_client_addr);
		seqInfo("Client Detected: %s", (const char*)m_ip);
	}
	else if (m_detectingClient && packet.getDestPort() == WorldServerGeneralPort)
	{
		m_ip = packet.getIPv4SourceA();
		m_client_addr = packet.getIPv4SourceN();
		m_detectingClient = false;
		emit clientChanged(m_client_addr);
		seqInfo("Client Detected: %s", (const char*)m_ip);
	}

	// Dispatch based on known streams
	if ((packet.getDestPort() == ChatServerPort) ||	(packet.getSourcePort() == ChatServerPort))
	{
		// Drop chat server traffic
		return;
	}
	else if ((packet.getDestPort() == WorldServerChatPort) || (packet.getSourcePort() == WorldServerChatPort))
	{
		// Drop cross-server chat traffic
		return;
	}
	else if ((packet.getDestPort() == WorldServerChat2Port) || (packet.getSourcePort() == WorldServerChat2Port))
	{
		// Drop email and cross-game chat traffic
		return;
	}
	else if (((packet.getDestPort() >= LoginServerMinPort) && (packet.getDestPort() <= LoginServerMaxPort)) ||
			 (packet.getSourcePort() >= LoginServerMinPort) && (packet.getSourcePort() <= LoginServerMaxPort))
	{
		// Drop login server traffic
		return;
	}
	else if (packet.getDestPort() == WorldServerGeneralPort || packet.getSourcePort() == WorldServerGeneralPort)
	{
		// World server traffic. Dispatch it.
		if (packet.getIPv4SourceN() == m_client_addr)
		{
			m_client2WorldStream->handlePacket(packet);
		}
		else
		{
			m_world2ClientStream->handlePacket(packet);
		}
	}
	else
	{
		// Anything else we assume is zone server traffic.
		if (packet.getIPv4SourceN() == m_client_addr)
		{
			m_client2ZoneStream->handlePacket(packet);
		}
		else
		{
			m_zone2ClientStream->handlePacket(packet);
		}
	}
} /* end dispatchPacket() */
#endif

////////////////////////////////////////////////////
// Handle zone2client stream closing
void EQPacket::closeStream(uint32_t sessionId, EQStreamID streamId)
{
#ifndef _WINDOWS
	// If this is the zone server session closing, reset the pcap filter to
	// a non-exclusive form
	if ((streamId == zone2client || streamId == client2zone) &&
		(m_playbackPackets == PLAYBACK_OFF || m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP))
	{
		m_packetCapture->setFilter(m_device, m_ip, m_realtime, IP_ADDRESS_TYPE, 0, 0);
		emit filterChanged();
	}

	// Pass the close onto the streams
	m_client2WorldStream->close(sessionId, streamId, m_session_tracking);
	m_world2ClientStream->close(sessionId, streamId, m_session_tracking);
	m_client2ZoneStream->close(sessionId, streamId, m_session_tracking);
	m_zone2ClientStream->close(sessionId, streamId, m_session_tracking);

	// If we just closed the zone server session, unlatch the client port
	if (streamId == zone2client || streamId == client2zone)
	{
		m_clientPort = 0;
		m_serverPort = 0;

		emit clientPortLatched(m_clientPort);

		seqInfo("EQPacket: SessionDisconnect detected, awaiting next zone session,  pcap filter: EQ Client %s",
				(const char*)m_ip);
	}
#endif
}

////////////////////////////////////////////////////
// Locks onto a specific client port (for session tracking)
void EQPacket::lockOnClient(in_port_t serverPort, in_port_t clientPort)
{
#ifndef _WINDOWS
	m_serverPort = serverPort;
	m_clientPort = clientPort;

	if (m_playbackPackets == PLAYBACK_OFF ||
		m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
	{
		if (m_mac.length() == 17)
		{
			m_packetCapture->setFilter(m_device, m_mac, m_realtime, MAC_ADDRESS_TYPE, 0, m_clientPort);
			emit filterChanged();
		}
		else
		{
			m_packetCapture->setFilter(m_device, m_ip, m_realtime, IP_ADDRESS_TYPE, 0, m_clientPort);
			emit filterChanged();
		}
	}

	// Wanted this message even if we're running on playback...
	if (m_mac.length() == 17)
	{
		seqInfo("EQPacket: SessionRequest detected, pcap filter: EQ Client %s, Client port %d. Server port %d",
				(const char*)m_mac, m_clientPort, m_serverPort);
	}
	else
	{
		seqInfo("EQPacket: SessionRequest detected, pcap filter: EQ Client %s, Client port %d. Server port %d",
				(const char*)m_ip, m_clientPort, m_serverPort);
	}

	emit clientPortLatched(m_clientPort);
#endif
}

void EQPacket::dispatchSessionKey(uint32_t sessionId, EQStreamID streamid, uint32_t sessionKey)
{
#ifndef _WINDOWS
	m_client2WorldStream->receiveSessionKey(sessionId, streamid, sessionKey);
	m_world2ClientStream->receiveSessionKey(sessionId, streamid, sessionKey);
	m_client2ZoneStream->receiveSessionKey(sessionId, streamid, sessionKey);
	m_zone2ClientStream->receiveSessionKey(sessionId, streamid, sessionKey);
#endif
}

///////////////////////////////////////////
//EQPacket::dispatchWorldChatData
// note this dispatch gets just the payload
void EQPacket::dispatchWorldChatData(size_t len, uint8_t *data, uint8_t dir)
{
#ifdef DEBUG_PACKET
	debug("dispatchWorldChatData()");
#endif /* DEBUG_PACKET */
	if (len < 10)
		return;

	uint16_t opCode = eqntohuint16(data);

	switch (opCode)
	{
		default:
			seqDebug("%04x - %d (%s)", opCode, len,
					 (dir == DIR_Server) ? "WorldChatServer --> Client" : "Client --> WorldChatServer");
	}
}

///////////////////////////////////////////
// Returns the current playback speed
int EQPacket::playbackSpeed()
{
#ifndef _WINDOWS
	if (m_vPacket)
		return m_vPacket->playbackSpeed();
	if (m_packetCapture)
		return m_packetCapture->getPlaybackSpeed();
#endif
	return 0;
}

///////////////////////////////////////////
// Set the packet playback speed
void EQPacket::setPlayback(int speed)
{
#ifndef _WINDOWS
	if (m_vPacket)
	{
		m_vPacket->setPlaybackSpeed(speed);
	}
	else if (m_packetCapture)
	{
		m_packetCapture->setPlaybackSpeed(speed);
	}
	else
		return;

	QString string("");

	if (speed == 0)
		string.sprintf("Playback speed set Fast as possible");
	else if (speed < 0)
		string.sprintf("Playback paused (']' to resume)");
	else
		string.sprintf("Playback speed set to %d", speed);

	emit stsMessage(string, 5000);

	emit resetPacket(m_client2WorldStream->packetCount(), client2world);
	emit resetPacket(m_world2ClientStream->packetCount(), world2client);
	emit resetPacket(m_client2ZoneStream->packetCount(), client2zone);
	emit resetPacket(m_zone2ClientStream->packetCount(), zone2client);

	emit playbackSpeedChanged(speed);
#endif
}

///////////////////////////////////////////
// Increment the packet playback speed
void EQPacket::incPlayback()
{
#ifndef _WINDOWS
	int x;

	if (m_vPacket)
	{
		x = m_vPacket->playbackSpeed();
	}
	else
	{
		x = m_packetCapture->getPlaybackSpeed();
	}

	switch(x)
	{
			// if we were paused go to 1X not full speed
		case -1:
			x = 1;
			break;

			// can't go faster than full speed
		case 0:
			return;

		case 9:
			x = 0;
			break;

		default:
			x += 1;
			break;
	}

	setPlayback(x);
#endif
}

///////////////////////////////////////////
// Decrement the packet playback speed
void EQPacket::decPlayback()
{
#ifndef _WINDOWS
	int x;

	if (m_vPacket)
	{
		x = m_vPacket->playbackSpeed();
	}
	else
	{
		x = m_packetCapture->getPlaybackSpeed();
	}

	switch(x)
	{
			// paused
		case -1:
			return;
			break;

			// slower than 1 is paused
		case 1:
			x = -1;
			break;

			// if we were full speed goto 9
		case 0:
			x = 9;
			break;

		default:
			x -= 1;
			break;
	}

	setPlayback(x);
#endif
}

///////////////////////////////////////////
// Set the IP address of the client to monitor
void EQPacket::monitorIPClient(const QString& ip)
{
	m_ip = ip;
#ifndef _WINDOWS
	struct in_addr  ia;
	inet_aton(m_ip, &ia);
	m_client_addr = ia.s_addr;
	emit clientChanged(m_client_addr);
#endif

	resetEQPacket();

	seqInfo("Listening for IP client: %s", (const char*)m_ip);
#ifndef _WINDOWS
	if (m_playbackPackets == PLAYBACK_OFF || m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
	{
		m_packetCapture->setFilter(m_device, m_ip, m_realtime, IP_ADDRESS_TYPE, 0, 0);
		emit filterChanged();
	}
#endif
}

///////////////////////////////////////////
// Set the MAC address of the client to monitor
void EQPacket::monitorMACClient(const QString& mac)
{
	m_mac = mac;
	m_detectingClient = true;
#ifndef _WINDOWS
	struct in_addr  ia;
	inet_aton(AUTOMATIC_CLIENT_IP, &ia);
	m_client_addr = ia.s_addr;
	emit clientChanged(m_client_addr);
#endif

	resetEQPacket();

	seqInfo("Listening for MAC client: %s", (const char*)m_mac);
#ifndef _WINDOWS
	if (m_playbackPackets == PLAYBACK_OFF || m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
	{
		m_packetCapture->setFilter(m_device, m_ip, m_realtime, IP_ADDRESS_TYPE, 0, 0);
		emit filterChanged();
	}
#endif
}

///////////////////////////////////////////
// Monitor the next client seen
void EQPacket::monitorNextClient()
{
	m_detectingClient = true;
	m_ip = AUTOMATIC_CLIENT_IP;
#ifndef _WINDOWS
	struct in_addr  ia;
	inet_aton(m_ip, &ia);
	m_client_addr = ia.s_addr;

	emit clientChanged(m_client_addr);
#endif

	resetEQPacket();

	seqInfo("Listening for next client seen. (you must zone for this to work!)");
#ifndef _WINDOWS
	if (m_playbackPackets == PLAYBACK_OFF || m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
	{
		m_packetCapture->setFilter(m_device, NULL, m_realtime, DEFAULT_ADDRESS_TYPE, 0, 0);
		emit filterChanged();
	}
#endif
}

///////////////////////////////////////////
// Monitor for packets on the specified device
void EQPacket::monitorDevice(const QString& dev)
{
	// set the device to use
	m_device = dev;

	// make sure we aren't playing back packets
	if (m_playbackPackets != PLAYBACK_OFF)
		return;
	if (m_useRemote)
		return;

#ifndef _WINDOWS
	// stop the current packet capture
	m_packetCapture->stop();

	// setup for capture on new device
	if (!m_ip.isEmpty())
	{
		struct hostent *he;
		struct in_addr  ia;

		/* Substitute "special" IP which is interpreted
		 to set up a different filter for picking up new sessions */

		if (m_ip == "auto")
			inet_aton(AUTOMATIC_CLIENT_IP, &ia);
		else if (inet_aton(m_ip, &ia) == 0)
		{
			he = gethostbyname(m_ip);
			if (!he)
				seqFatal("Invalid address; %s", (const char*)m_ip);

			memcpy(&ia, he->h_addr_list[0], he->h_length);
		}
		m_client_addr = ia.s_addr;
		m_ip = inet_ntoa(ia);

		if (m_ip ==  AUTOMATIC_CLIENT_IP)
		{
			m_detectingClient = true;
			seqInfo("Listening for first client seen.");
		}
		else
		{
			m_detectingClient = false;
			seqInfo("Listening for client: %s", (const char*)m_ip);
		}
	}
#endif

	resetEQPacket();

#ifndef _WINDOWS
	// restart packet capture
	if (m_mac.length() == 17)
		m_packetCapture->start(m_device, m_mac, m_realtime, MAC_ADDRESS_TYPE);
	else
		m_packetCapture->start(m_device, m_ip, m_realtime, IP_ADDRESS_TYPE);
	emit filterChanged();
#endif
}

///////////////////////////////////////////
// Set the session tracking state
void EQPacket::session_tracking(bool enable)
{
	m_session_tracking = enable;
#ifndef _WINDOWS
	m_client2WorldStream->setSessionTracking(m_session_tracking);
	m_world2ClientStream->setSessionTracking(m_session_tracking);
	m_client2ZoneStream->setSessionTracking(m_session_tracking);
	m_zone2ClientStream->setSessionTracking(m_session_tracking);
#endif
	emit sessionTrackingChanged(m_session_tracking);
}

///////////////////////////////////////////
// Set the current ArqSeqGiveUp
void EQPacket::setArqSeqGiveUp(uint16_t giveUp)
{
	// a sanity check, if the user set it to below 32, they're prolly nuts
	if (giveUp >= 32)
		m_arqSeqGiveUp = giveUp;

	// a sanity check, if the user set it to below 32, they're prolly nuts
	if (m_arqSeqGiveUp >= 32)
		giveUp = m_arqSeqGiveUp;
	else
		giveUp = 32;

#ifndef _WINDOWS
	m_client2WorldStream->setArqSeqGiveUp(giveUp);
	m_world2ClientStream->setArqSeqGiveUp(giveUp);
	m_client2ZoneStream->setArqSeqGiveUp(giveUp);
	m_zone2ClientStream->setArqSeqGiveUp(giveUp);
#endif
}

void EQPacket::setRealtime(bool val)
{
	m_realtime = val;
}

bool EQPacket::connect2(const QString& opcodeName, EQStreamPairs sp,
						uint8_t dir, const char* payload,  EQSizeCheckType szt,
						const QObject* receiver, const char* member)
{
	bool res = false;

	if (m_playbackPackets == PLAYBACK_REMOTE)
	{
		return m_remoteServer->connect2(opcodeName, payload, sp, dir, szt, receiver, member);
	}

#ifndef _WINDOWS
	if (sp & SP_World)
	{
		if (dir & DIR_Client)
			res = m_client2WorldStream->connect2(opcodeName, payload, szt, receiver, member);
		if (dir & DIR_Server)
			res = m_world2ClientStream->connect2(opcodeName, payload, szt, receiver, member);
	}
	if (sp & SP_Zone)
	{
		if (dir & DIR_Client)
			res = m_client2ZoneStream->connect2(opcodeName, payload, szt, receiver, member);
		if (dir & DIR_Server)
			res = m_zone2ClientStream->connect2(opcodeName, payload, szt, receiver, member);
	}
#endif
	return res;
}

///////////////////////////////////////////
// Reset EQPacket's state
void EQPacket::resetEQPacket()
{
	if (m_playbackPackets == PLAYBACK_REMOTE)
	{
		m_remoteServer->reset();
	}

#ifndef _WINDOWS
	m_client2WorldStream->reset();
	m_client2WorldStream->setSessionTracking(m_session_tracking);
	m_world2ClientStream->reset();
	m_world2ClientStream->setSessionTracking(m_session_tracking);
	m_client2ZoneStream->reset();
	m_client2ZoneStream->setSessionTracking(m_session_tracking);
	m_zone2ClientStream->reset();
	m_zone2ClientStream->setSessionTracking(m_session_tracking);
#endif
	m_clientPort = 0;
	m_serverPort = 0;

	emit clientPortLatched(m_clientPort);
}

///////////////////////////////////////////
// Return the current pcap filter
const QString EQPacket::pcapFilter()
{
	// make sure we aren't playing back packets
	if (m_playbackPackets == PLAYBACK_REMOTE)
		return QString("Remote");
	if (m_playbackPackets != PLAYBACK_OFF)
		return QString("Playback");
#ifndef _WINDOWS
	return m_packetCapture->getFilter();
#endif
}


int EQPacket::packetCount(int stream)
{
#ifndef _WINDOWS
	return m_streams[stream]->packetCount();
#else
	return 0;
#endif
}

uint8_t EQPacket::session_tracking_enabled()
{
#ifndef _WINDOWS
	return m_zone2ClientStream->sessionTracking();
#else
	return 0;
#endif
}

size_t EQPacket::currentCacheSize(int stream)
{
#ifndef _WINDOWS
	return m_streams[stream]->currentCacheSize();
#else
	return 0;
#endif
}

uint32_t EQPacket::currentMaxLength(int streamId)
{
#ifndef _WINDOWS
    return m_streams[streamId]->getMaxLength();
#else
	return 0;
#endif
}

uint16_t EQPacket::serverSeqExp(int stream)
{
#ifndef _WINDOWS
	return m_streams[stream]->arqSeqExp();
#else
	return 0;
#endif
}
