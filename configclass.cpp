/*
 *
 *    kmidi - a midi to wav converter
 *
 *    configclass.cpp
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
#include <kconfig.h>
#include <kapplication.h>

#include "config.h"
#include "configclass.h"

KmidiConfig::KmidiConfig() {

	toolTips = false;
	meterVisible = false;
	infoVisible = false;
	filterRequest = false;
	effectsRequest = false;
	interpolationRequest = false;

	infoWindowHeight = 0;

	volume = 40;
	meterAdjust = 0;
	playlistNum = 0;
	
	stereoState = 1;
	echoState = 1;
	detuneState = 1;
	verbosityState = 1;
	dryState = 0;
	eState = 0;
	vState = 0;
	sState = 0;
	evsState = 0;
	currentPatchSet = 0;

	currentVoices = DEFAULT_VOICES;

	backgroundColor = QColor(0,0,0);
	ledColor = QColor(107,227,88);

	config = KApplication::kApplication()->config();
	config->setGroup("KMidi");

	loadConfig();

}

KmidiConfig::~KmidiConfig() {

}

void KmidiConfig::saveConfig() {

	 config->writeEntry("ToolTips", toolTips);

	 config->writeEntry("Volume", volume);
	 config->writeEntry("Polyphony", currentVoices);
	 config->writeEntry("BackColor", backgroundColor);
	 config->writeEntry("LEDColor", ledColor);
	 config->writeEntry("MeterAdjust", meterFudge);
	 config->writeEntry("Playlist", playlistNum);
	 config->writeEntry("ShowMeter", meterVisible);
	 config->writeEntry("ShowInfo", infoVisible);
	 config->writeEntry("InfoWindowHeight", infoWindowHeight);
	 config->writeEntry("Filter", filterRequest);
	 config->writeEntry("Effects", effectsRequest);
	 config->writeEntry("Interpolation", interpolationRequest);
	 config->writeEntry("MegsOfPatches", maxPatchMegs);
	 config->writeEntry("StereoNotes", stereoState);
	 config->writeEntry("EchoNotes", echoState);
	 config->writeEntry("Reverb", reverbState);
	 config->writeEntry("DetuneNotes", detuneState);
	 config->writeEntry("Chorus", chorusState);
	 config->writeEntry("Dry", dryState);
	 eState = (evsState >> 8) & 0x0f;
	 config->writeEntry("ExpressionCurve", eState);
	 vState = (evsState >> 4) & 0x0f;
	 config->writeEntry("VolumeCurve", vState);
	 eState = evsState & 0x0f;
	 config->writeEntry("Surround", sState);
	 config->writeEntry("Verbosity", verbosityState);
	 config->writeEntry("Patchset", currentPatchSet);
	 config->writeEntry("Directory", currentDirectory);
	 config->sync();
	 
}

void KmidiConfig::loadConfig() {


	volume = config->readNumEntry("Volume", 40);
	currentVoices = config->readNumEntry("Polyphony", DEFAULT_VOICES);
	meterFudge = config->readNumEntry("MeterAdjust", 0);
	toolTips = config->readBoolEntry("ToolTips", TRUE);
	playlistNum = config->readNumEntry("PlayList", 0);
	meterVisible = config->readBoolEntry("ShowMeter", TRUE);
	infoVisible = config->readBoolEntry("ShowInfo", TRUE);
	infoWindowHeight = config->readNumEntry("InfoWindowHeight", 80);
	filterRequest = config->readBoolEntry("Filter", FALSE);
	effectsRequest = config->readBoolEntry("Effects", TRUE);
	interpolationRequest = config->readNumEntry("Interpolation", 2);
	maxPatchMegs = config->readNumEntry("MegsOfPatches", 60);
	stereoState = config->readNumEntry("StereoNotes", 1);
	echoState = config->readNumEntry("EchoNotes", 1);
	reverbState = config->readNumEntry("Reverb", 0);
	detuneState = config->readNumEntry("DetuneNotes", 1);
	chorusState = config->readNumEntry("Chorus", 1);
	dryState = config->readBoolEntry("Dry", FALSE);
	eState = config->readNumEntry("ExpressionCurve", 1);
	vState = config->readNumEntry("VolumeCurve", 1);
	sState = config->readNumEntry("Surround", 1);
	evsState = (eState << 8) + (vState << 4) + sState;
	verbosityState = config->readNumEntry("Verbosity", 1);
	currentPatchSet = config->readNumEntry("Patchset", 0);

	backgroundColor = config->readColorEntry("BackColor", &QColor(0,0,0));
	ledColor = config->readColorEntry("LEDColor", &QColor(107, 227, 98));

	currentDirectory = config->readPathEntry("Directory");

}
