#include "EspiAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


EspiAnalyzerSettings::EspiAnalyzerSettings()
:	mClockChannel( UNDEFINED_CHANNEL ),
	mChipSelectChannel( UNDEFINED_CHANNEL ),
	mResetChannel( UNDEFINED_CHANNEL ),
	mAlertChannel( UNDEFINED_CHANNEL ),
	mCsGlitchFilterNs( 0 ),
	mIo0Channel( UNDEFINED_CHANNEL ),
	mIo1Channel( UNDEFINED_CHANNEL ),
	mIo2Channel( UNDEFINED_CHANNEL ),
	mIo3Channel( UNDEFINED_CHANNEL ),
	mInitialIoMode( 0 ),
	mIgnoreAlert( false ),
	mIgnorePutIoReadShort( false ),
	mClockChannelInterface(),
	mChipSelectChannelInterface(),
	mResetChannelInterface(),
	mIo0ChannelInterface(),
	mIo1ChannelInterface(),
	mIo2ChannelInterface(),
	mIo3ChannelInterface(),
	mInitialIoModeInterface(),
	mIgnoreAlertInterface(),
	mIgnorePutIoReadShortInterface()
{
	mAlertChannelInterface.SetTitleAndTooltip( "ALERT# (optional)", "Dedicated active-low ALERT# input. Leave unselected to use shared IO1 alerts." );
	mAlertChannelInterface.SetSelectionOfNoneIsAllowed( true );
	mAlertChannelInterface.SetChannel( mAlertChannel );
	mCsGlitchFilterNsInterface.SetTitleAndTooltip( "CS# glitch filter (ns)", "Ignore high and low CS# pulses shorter than this duration. Zero disables filtering." );
	mCsGlitchFilterNsInterface.SetMin( 0 );
	mCsGlitchFilterNsInterface.SetMax( 1000000000 );
	mCsGlitchFilterNsInterface.SetInteger( mCsGlitchFilterNs );

	mClockChannelInterface.SetTitleAndTooltip( "Clock", "eSPI clock line." );
	mClockChannelInterface.SetChannel( mClockChannel );

	mChipSelectChannelInterface.SetTitleAndTooltip( "CS#", "Active-low eSPI chip select." );
	mChipSelectChannelInterface.SetChannel( mChipSelectChannel );

	mResetChannelInterface.SetTitleAndTooltip( "RESET#", "Optional active-low eSPI reset line. When asserted, the analyzer returns to Single I/O mode." );
	mResetChannelInterface.SetSelectionOfNoneIsAllowed( true );
	mResetChannelInterface.SetChannel( mResetChannel );

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

	mInitialIoModeInterface.SetTitleAndTooltip( "Initial I/O mode", "I/O width active at the beginning of the capture." );
	mInitialIoModeInterface.AddNumber( 0, "Single", "Start by sampling commands and responses in Single I/O mode." );
	mInitialIoModeInterface.AddNumber( 1, "Dual", "Start by sampling commands and responses in Dual I/O mode." );
	mInitialIoModeInterface.AddNumber( 2, "Quad", "Start by sampling commands and responses in Quad I/O mode." );
	mInitialIoModeInterface.SetNumber( mInitialIoMode );

	mIgnoreAlertInterface.SetCheckBoxText( "Ignore ALERT#" );
	mIgnoreAlertInterface.SetValue( mIgnoreAlert );

	mIgnorePutIoReadShortInterface.SetCheckBoxText( "Ignore PUT_IORD_SHORT" );
	mIgnorePutIoReadShortInterface.SetValue( mIgnorePutIoReadShort );

	AddInterface( &mClockChannelInterface );
	AddInterface( &mChipSelectChannelInterface );
	AddInterface( &mResetChannelInterface );
	AddInterface( &mAlertChannelInterface );
	AddInterface( &mCsGlitchFilterNsInterface );
	AddInterface( &mIo0ChannelInterface );
	AddInterface( &mIo1ChannelInterface );
	AddInterface( &mIo2ChannelInterface );
	AddInterface( &mIo3ChannelInterface );
	AddInterface( &mInitialIoModeInterface );
	AddInterface( &mIgnoreAlertInterface );
	AddInterface( &mIgnorePutIoReadShortInterface );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mClockChannel, "CLK", false );
	AddChannel( mChipSelectChannel, "CS#", false );
	AddChannel( mResetChannel, "RESET#", false );
	AddChannel( mAlertChannel, "ALERT#", false );
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
	mAlertChannel = mAlertChannelInterface.GetChannel();
	const int filter_ns = mCsGlitchFilterNsInterface.GetInteger();
	if( filter_ns < 0 || filter_ns > 1000000000 )
	{
		SetErrorText( "CS# glitch filter must be between 0 and 1000000000 ns." );
		return false;
	}
	mCsGlitchFilterNs = U32( filter_ns );
	mClockChannel = mClockChannelInterface.GetChannel();
	mChipSelectChannel = mChipSelectChannelInterface.GetChannel();
	mResetChannel = mResetChannelInterface.GetChannel();
	mIo0Channel = mIo0ChannelInterface.GetChannel();
	mIo1Channel = mIo1ChannelInterface.GetChannel();
	mIo2Channel = mIo2ChannelInterface.GetChannel();
	mIo3Channel = mIo3ChannelInterface.GetChannel();
	mInitialIoMode = U32( mInitialIoModeInterface.GetNumber() );
	mIgnoreAlert = mIgnoreAlertInterface.GetValue();
	mIgnorePutIoReadShort = mIgnorePutIoReadShortInterface.GetValue();

	if( mInitialIoMode > 2 )
	{
		SetErrorText( "Initial I/O mode is invalid." );
		return false;
	}
	if( mInitialIoMode == 2 && ( mIo2Channel == UNDEFINED_CHANNEL || mIo3Channel == UNDEFINED_CHANNEL ) )
	{
		SetErrorText( "Quad I/O mode requires both IO2 and IO3 channels." );
		return false;
	}

	ClearChannels();
	AddChannel( mClockChannel, "CLK", true );
	AddChannel( mChipSelectChannel, "CS#", true );
	AddChannel( mResetChannel, "RESET#", mResetChannel != UNDEFINED_CHANNEL );
	AddChannel( mAlertChannel, "ALERT#", mAlertChannel != UNDEFINED_CHANNEL );
	AddChannel( mIo0Channel, "IO0", true );
	AddChannel( mIo1Channel, "IO1", true );
	AddChannel( mIo2Channel, "IO2", mIo2Channel != UNDEFINED_CHANNEL );
	AddChannel( mIo3Channel, "IO3", mIo3Channel != UNDEFINED_CHANNEL );

	return true;
}

void EspiAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mAlertChannelInterface.SetChannel( mAlertChannel );
	mCsGlitchFilterNsInterface.SetInteger( mCsGlitchFilterNs );
	mClockChannelInterface.SetChannel( mClockChannel );
	mChipSelectChannelInterface.SetChannel( mChipSelectChannel );
	mResetChannelInterface.SetChannel( mResetChannel );
	mIo0ChannelInterface.SetChannel( mIo0Channel );
	mIo1ChannelInterface.SetChannel( mIo1Channel );
	mIo2ChannelInterface.SetChannel( mIo2Channel );
	mIo3ChannelInterface.SetChannel( mIo3Channel );
	mInitialIoModeInterface.SetNumber( mInitialIoMode );
	mIgnoreAlertInterface.SetValue( mIgnoreAlert );
	mIgnorePutIoReadShortInterface.SetValue( mIgnorePutIoReadShort );
}

void EspiAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mClockChannel;
	text_archive >> mChipSelectChannel;
	Channel reset_channel = UNDEFINED_CHANNEL;
	if( text_archive >> reset_channel )
		mResetChannel = reset_channel;
	text_archive >> mIo0Channel;
	text_archive >> mIo1Channel;
	text_archive >> mIo2Channel;
	text_archive >> mIo3Channel;
	U32 initial_io_mode = 0;
	if( text_archive >> initial_io_mode )
		mInitialIoMode = initial_io_mode <= 2 ? initial_io_mode : 0;
	bool ignore_alert = false;
	if( text_archive >> ignore_alert )
		mIgnoreAlert = ignore_alert;
	bool ignore_put_io_read_short = false;
	if( text_archive >> ignore_put_io_read_short )
		mIgnorePutIoReadShort = ignore_put_io_read_short;

	// Append new settings so older saved configurations keep their defaults.
	mAlertChannel = UNDEFINED_CHANNEL;
	mCsGlitchFilterNs = 0;
	Channel alert_channel = UNDEFINED_CHANNEL;
	if( text_archive >> alert_channel )
		mAlertChannel = alert_channel;
	U32 filter_ns = 0;
	if( text_archive >> filter_ns )
		mCsGlitchFilterNs = filter_ns <= 1000000000 ? filter_ns : 0;

	ClearChannels();
	AddChannel( mClockChannel, "CLK", true );
	AddChannel( mChipSelectChannel, "CS#", true );
	AddChannel( mResetChannel, "RESET#", mResetChannel != UNDEFINED_CHANNEL );
	AddChannel( mAlertChannel, "ALERT#", mAlertChannel != UNDEFINED_CHANNEL );
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
	text_archive << mResetChannel;
	text_archive << mIo0Channel;
	text_archive << mIo1Channel;
	text_archive << mIo2Channel;
	text_archive << mIo3Channel;
	text_archive << mInitialIoMode;
	text_archive << mIgnoreAlert;
	text_archive << mIgnorePutIoReadShort;
	text_archive << mAlertChannel;
	text_archive << mCsGlitchFilterNs;

	return SetReturnString( text_archive.GetString() );
}
