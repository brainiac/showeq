/*
 * main.h
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sourceforge.net/
 */

#ifndef _SHOWEQ_MAIN_H
#define _SHOWEQ_MAIN_H

#include "xmlpreferences.h"
extern class XMLPreferences *pSEQPrefs;

#ifdef VERSION
#undef VERSION
#endif

#define VERSION "6.0.0.0"

#ifdef PKGDATADIR
#undef PKGDATADIR
#endif

// TODO: Update file paths for cross platform compatibility
#ifdef Q_OS_WIN
#define PKGDATADIR "."
#else
#define PKGDATADIR "/usr/local/share/showeq"
#endif



// TODO: Look up ID for new races

//----------------------------------------------------------------------
// TODO: bogus structure that should die soon
struct ShowEQParams
{
	bool			retarded_coords; // Verant style YXZ instead of XYZ
	bool			fast_machine;
	bool			createUnknownSpawns;
	bool			keep_selected_visible;
	bool			pvp;
	bool			deitypvp;
	bool			walkpathrecord;
	uint32_t		walkpathlength;
	bool			systime_spawntime;
	bool			showRealName;

	bool			saveZoneState;
	bool			savePlayerState;
	bool			saveSpawns;
	uint32_t		saveSpawnsFrequency;
	bool			restorePlayerState;
	bool			restoreZoneState;
	bool			restoreSpawns;
	QString			saveRestoreBaseFilename;
	bool			useUpdateRadius;
	uint8_t			filterZoneDataLog;
};

extern struct ShowEQParams *showeq_params;

#endif
