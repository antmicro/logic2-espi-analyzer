#ifndef ESPI_ANALYZER_SETTINGS
#define ESPI_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class EspiAnalyzerSettings : public AnalyzerSettings
{
public:
	EspiAnalyzerSettings();
	virtual ~EspiAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	Channel mClockChannel;
	Channel mChipSelectChannel;
	Channel mIo0Channel;
	Channel mIo1Channel;
	Channel mIo2Channel;
	Channel mIo3Channel;

protected:
	AnalyzerSettingInterfaceChannel	mClockChannelInterface;
	AnalyzerSettingInterfaceChannel	mChipSelectChannelInterface;
	AnalyzerSettingInterfaceChannel	mIo0ChannelInterface;
	AnalyzerSettingInterfaceChannel	mIo1ChannelInterface;
	AnalyzerSettingInterfaceChannel	mIo2ChannelInterface;
	AnalyzerSettingInterfaceChannel	mIo3ChannelInterface;
};

#endif //ESPI_ANALYZER_SETTINGS
