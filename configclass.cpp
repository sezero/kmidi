
#include <qcolor.h>
#include <kconfig.h>
#include <kapplication.h>

#include "config.h"
#include "configclass.h"

kmidiConfig::kmidiConfig() {

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
	
	stereoState = 0;
	echoState = 0;
	detuneState = 0;
	verbosityState = 0;
	dryState = 0;
	eState = 0;
	vState = 0;
	sState = 0;
	evsState = 0;
	currentPatchSet = 0;

	backgroundColor = QColor(0,0,0);
	ledColor = QColor(107,227,88);

	config = KApplication::kApplication()->config();
	config->setGroup("KMidi");

	loadConfig();

}

kmidiConfig::~kmidiConfig() {

}

void kmidiConfig::saveConfig() {

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
	 config->sync();
	 
}

void kmidiConfig::loadConfig() {

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

}
