#include "EspiAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


EspiAnalyzerSettings::EspiAnalyzerSettings()
:	mClockChannel( UNDEFINED_CHANNEL ),
	mChipSelectChannel( UNDEFINED_CHANNEL ),
	mIo0Channel( UNDEFINED_CHANNEL ),
	mIo1Channel( UNDEFINED_CHANNEL ),
	mIo2Channel( UNDEFINED_CHANNEL ),
	mIo3Channel( UNDEFINED_CHANNEL ),
	mClockChannelInterface(),
	mChipSelectChannelInterface(),
	mIo0ChannelInterface(),
	mIo1ChannelInterface(),
	mIo2ChannelInterface(),
	mIo3ChannelInterface()
{
	mClockChannelInterface.SetTitleAndTooltip( "Clock", "eSPI clock line." );
	mClockChannelInterface.SetChannel( mClockChannel );

	mChipSelectChannelInterface.SetTitleAndTooltip( "CS#", "Active-low eSPI chip select." );
	mChipSelectChannelInterface.SetChannel( mChipSelectChannel );

	mIo0ChannelInterface.SetTitleAndTooltip( "IO0", "eSPI IO0 data line." );
	mIo0ChannelInterface.SetChannel( mIo0Channel );

	mIo1ChannelInterface.SetTitleAndTooltip( "IO1", "eSPI IO1 data line." );
	mIo1ChannelInterface.SetChannel( mIo1Channel );

	mIo2ChannelInterface.SetTitleAndTooltip( "IO2", "eSPI IO2 data line." );
	mIo2ChannelInterface.SetSelectionOfNoneIsAllowed( true );
	mIo2ChannelInterface.SetChannel( mIo2Channel );

	mIo3ChannelInterface.SetTitleAndTooltip( "IO3", "eSPI IO3 data line." );
	mIo3ChannelInterface.SetSelectionOfNoneIsAllowed( true );
	mIo3ChannelInterface.SetChannel( mIo3Channel );

	AddInterface( &mClockChannelInterface );
	AddInterface( &mChipSelectChannelInterface );
	AddInterface( &mIo0ChannelInterface );
	AddInterface( &mIo1ChannelInterface );
	AddInterface( &mIo2ChannelInterface );
	AddInterface( &mIo3ChannelInterface );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mClockChannel, "CLK", false );
	AddChannel( mChipSelectChannel, "CS#", false );
	AddChannel( mIo0Channel, "IO0", false );
	AddChannel( mIo1Channel, "IO1", false );
	AddChannel( mIo2Channel, "IO2", false );
	AddChannel( mIo3Channel, "IO3", false );
}

EspiAnalyzerSettings::~EspiAnalyzerSettings()
{
}

bool EspiAnalyzerSettings::SetSettingsFromInterfaces()
{
	mClockChannel = mClockChannelInterface.GetChannel();
	mChipSelectChannel = mChipSelectChannelInterface.GetChannel();
	mIo0Channel = mIo0ChannelInterface.GetChannel();
	mIo1Channel = mIo1ChannelInterface.GetChannel();
	mIo2Channel = mIo2ChannelInterface.GetChannel();
	mIo3Channel = mIo3ChannelInterface.GetChannel();

	ClearChannels();
	AddChannel( mClockChannel, "CLK", true );
	AddChannel( mChipSelectChannel, "CS#", true );
	AddChannel( mIo0Channel, "IO0", true );
	AddChannel( mIo1Channel, "IO1", true );
	AddChannel( mIo2Channel, "IO2", mIo2Channel != UNDEFINED_CHANNEL );
	AddChannel( mIo3Channel, "IO3", mIo3Channel != UNDEFINED_CHANNEL );

	return true;
}

void EspiAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mClockChannelInterface.SetChannel( mClockChannel );
	mChipSelectChannelInterface.SetChannel( mChipSelectChannel );
	mIo0ChannelInterface.SetChannel( mIo0Channel );
	mIo1ChannelInterface.SetChannel( mIo1Channel );
	mIo2ChannelInterface.SetChannel( mIo2Channel );
	mIo3ChannelInterface.SetChannel( mIo3Channel );
}

void EspiAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mClockChannel;
	text_archive >> mChipSelectChannel;
	text_archive >> mIo0Channel;
	text_archive >> mIo1Channel;
	text_archive >> mIo2Channel;
	text_archive >> mIo3Channel;

	ClearChannels();
	AddChannel( mClockChannel, "CLK", true );
	AddChannel( mChipSelectChannel, "CS#", true );
	AddChannel( mIo0Channel, "IO0", true );
	AddChannel( mIo1Channel, "IO1", true );
	AddChannel( mIo2Channel, "IO2", mIo2Channel != UNDEFINED_CHANNEL );
	AddChannel( mIo3Channel, "IO3", mIo3Channel != UNDEFINED_CHANNEL );

	UpdateInterfacesFromSettings();
}

const char* EspiAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mClockChannel;
	text_archive << mChipSelectChannel;
	text_archive << mIo0Channel;
	text_archive << mIo1Channel;
	text_archive << mIo2Channel;
	text_archive << mIo3Channel;

	return SetReturnString( text_archive.GetString() );
}
