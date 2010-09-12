/*
 *  datasource.cpp
 *  showeq
 *
 *  Created by Stephen Raub on 4/26/09.
 *  Copyright 2009 Vexislogic. All rights reserved.
 *
 */

#include "pch.h"
#include "datasource.h"


DataSource::DataSource(QObject* parent) : QObject(parent)
{
}

DataSource::~DataSource()
{
}

void DataSource::reset()
{
	stop();
	start();
}