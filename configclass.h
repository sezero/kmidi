
#include <qcolor.h>

class kmidiConfig {

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

	kmidiConfig();
	~kmidiConfig();

	void saveConfig();
	void loadConfig();

};
