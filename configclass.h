/*
 *
 *    kmidi - a midi to wav converter
 *
 *    configclass.h
 *
 *    Copyright 2003 Russell Miller <rmiller@duskglow.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qcolor.h>

class KmidiConfig {

public:

	bool toolTips;
	bool meterVisible;
	bool infoVisible;
	bool filterRequest;
	bool effectsRequest;
	bool interpolationRequest;

	int infoWindowHeight;

	int volume;
	int meterAdjust;
	int playlistNum;
	
	int currentVoices;
	int stereoState;
	int echoState;
	int detuneState;
	int chorusState;
	int reverbState;
	int verbosityState;
	int dryState;
	int eState;
	int vState;
	int sState;
	int evsState;
	int currentPatchSet;
	int meterFudge;

	int maxPatchMegs;

	KConfig *config;

	QColor backgroundColor;
	QColor ledColor;
	QString currentDirectory;

	KmidiConfig();
	~KmidiConfig();

	void saveConfig();
	void loadConfig();

};
