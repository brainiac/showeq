/*
 *  datasource.cpp
 *  showeq
 *
 *  Created by Stephen Raub on 4/26/09.
 *  Copyright 2009 Vexislogic. All rights reserved.
 *
 */

#include "datasource.h"


DataSource::DataSource()
{
}

DataSource::~DataSource()
{
}

bool DataSource::connectReceiver(const QString& messageName, const QString& payloadName,
		const QObject* receiver, const char* member)
{
	return false;
}

bool DataSource::connectReceiver(const QString& messageName, const QString& payloadName,
		EQStreamPairs sp, uint8_t dir, EQSizeCheckType szt, const QObject* receiver, const char* member)
{
	return false;
}

void DataSource::start()
{
}

void DataSource::stop()
{
}

void DataSource::reset()
{
}