#include "EspiAnalyzer.h"
#include "EspiCommand.h"
#include "EspiAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

namespace
{
	enum EspiFrameType : U8
	{
		kTransactionFrame = 1,
		kAlertFrame = 2
	};

	static constexpr U32 kPreviewCommandByteCount = EspiCommand::kPreviewByteCount;
	static constexpr U32 kPreviewResponseByteCount = 4;

	enum class EspiIoMode : U8
	{
		Single = 0,
		Dual = 1,
		Quad = 2
	};

	bool IsAcceptResponse( U8 response )
	{
		return ( response & 0x0f ) == 0x08 && ( response & 0x30 ) == 0x00;
	}
}

EspiAnalyzer::EspiAnalyzer()
:	Analyzer2(),  
	mSettings(),
	mClock( nullptr ),
	mChipSelect( nullptr ),
	mIo0( nullptr ),
	mIo1( nullptr ),
	mIo2( nullptr ),
	mIo3( nullptr ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( &mSettings );
}

EspiAnalyzer::~EspiAnalyzer()
{
	KillThread();
}

void EspiAnalyzer::SetupResults()
{
	// SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the results each time.
	mResults.reset(new EspiAnalyzerResults( this, &mSettings ));
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings.mChipSelectChannel );
	mResults->AddChannelBubblesWillAppearOn( mSettings.mIo1Channel );
}

void EspiAnalyzer::WorkerThread()
{
	mClock = GetAnalyzerChannelData( mSettings.mClockChannel );
	mChipSelect = GetAnalyzerChannelData( mSettings.mChipSelectChannel );
	mIo0 = GetAnalyzerChannelData( mSettings.mIo0Channel );
	mIo1 = GetAnalyzerChannelData( mSettings.mIo1Channel );
	mIo2 = GetAnalyzerChannelData( mSettings.mIo2Channel );
	mIo3 = GetAnalyzerChannelData( mSettings.mIo3Channel );
	EspiIoMode active_io_mode = EspiIoMode::Single;
	bool alert_armed = mIo1->GetBitState() == BIT_HIGH;
	bool alert_asserted = false;
	U64 alert_start = 0;
	auto EmitAlertFrame = [&]( U64 start, U64 end ) {
		Frame frame;
		frame.mType = kAlertFrame;
		frame.mFlags = 0;
		frame.mData1 = 0;
		frame.mData2 = 0;
		frame.mStartingSampleInclusive = start;
		frame.mEndingSampleInclusive = end;
		mResults->AddTransactionDetails( EspiAnalyzerResults::TransactionDetails() );
		mResults->AddFrame( frame );
		mResults->CommitResults();
		mResults->CommitPacketAndStartNewPacket();
		ReportProgress( end );
	};

	for( ; ; )
	{
		if( mChipSelect->GetBitState() == BIT_HIGH )
		{
			mIo1->AdvanceToAbsPosition( mChipSelect->GetSampleNumber() );
			if( mIo1->GetBitState() == BIT_HIGH )
				alert_armed = true;

			while( mChipSelect->GetBitState() == BIT_HIGH )
			{
				const U64 next_cs_edge = mChipSelect->GetSampleOfNextEdge();
				const U64 next_alert_edge = mIo1->GetSampleOfNextEdge();
				if( next_alert_edge < next_cs_edge )
				{
					mIo1->AdvanceToNextEdge();
					mChipSelect->AdvanceToAbsPosition( mIo1->GetSampleNumber() );
					if( mIo1->GetBitState() == BIT_LOW && alert_armed )
					{
						alert_asserted = true;
						alert_armed = false;
						alert_start = mIo1->GetSampleNumber();
					}
					else if( mIo1->GetBitState() == BIT_HIGH )
					{
						if( alert_asserted )
							EmitAlertFrame( alert_start, mIo1->GetSampleNumber() );
						alert_asserted = false;
						alert_armed = true;
					}
					continue;
				}

				mChipSelect->AdvanceToNextEdge();
				mIo1->AdvanceToAbsPosition( mChipSelect->GetSampleNumber() );
				if( alert_asserted )
					EmitAlertFrame( alert_start, mChipSelect->GetSampleNumber() );
				alert_asserted = false;
				alert_armed = false;
			}
		}

		if( mChipSelect->GetBitState() != BIT_LOW )
			continue;

		const U64 transaction_start = mChipSelect->GetSampleNumber();
		mClock->AdvanceToAbsPosition( transaction_start );
		mIo0->AdvanceToAbsPosition( transaction_start );
		mIo1->AdvanceToAbsPosition( transaction_start );
		mIo2->AdvanceToAbsPosition( transaction_start );
		mIo3->AdvanceToAbsPosition( transaction_start );

		U64 clock_edge_count = 0;
		U64 preview_bytes = 0;
		U8 current_command_byte = 0;
		U8 current_response_byte = 0;
		U32 current_command_bit_count = 0;
		U32 current_response_bit_count = 0;
		U32 captured_command_bytes = 0;
		U32 captured_response_bytes = 0;
		U32 observed_wait_state_bytes = 0;
		U32 expected_command_bytes = kPreviewCommandByteCount;
		U32 turnaround_edge_count = 0;
		U8 command_opcode = 0;
		U8 command_byte1 = 0;
		U8 command_byte2 = 0;
		U8 command_byte3 = 0;
		U16 configuration_address = 0;
		U32 configuration_value = 0;
		U8 first_response_byte = 0xff;
		U8 response_tail0 = 0;
		U8 response_tail1 = 0;
		U8 response_tail2 = 0;
		U8 pending_virtual_wire_index = 0;
		EspiAnalyzerResults::TransactionDetails transaction_details;
		const U32 bits_per_clock = active_io_mode == EspiIoMode::Quad ? 4 : ( active_io_mode == EspiIoMode::Dual ? 2 : 1 );

		enum class Phase
		{
			Command,
			Turnaround,
			ResponseWaitState,
			ResponseData
		};

		Phase phase = Phase::Command;
		auto RecordResponseByte = [&]( U8 value ) {
			response_tail0 = response_tail1;
			response_tail1 = response_tail2;
			response_tail2 = value;
		};
		auto SampleSymbol = [&]( bool response_phase ) -> U8 {
			const U64 sample = mClock->GetSampleNumber();
			if( active_io_mode == EspiIoMode::Single )
			{
				AnalyzerChannelData* data = response_phase ? mIo1 : mIo0;
				data->AdvanceToAbsPosition( sample );
				return data->GetBitState() == BIT_HIGH ? 1 : 0;
			}

			mIo0->AdvanceToAbsPosition( sample );
			mIo1->AdvanceToAbsPosition( sample );
			U8 symbol = 0;
			if( mIo0->GetBitState() == BIT_HIGH )
				symbol |= 0x1;
			if( mIo1->GetBitState() == BIT_HIGH )
				symbol |= 0x2;

			if( active_io_mode == EspiIoMode::Quad )
			{
				mIo2->AdvanceToAbsPosition( sample );
				mIo3->AdvanceToAbsPosition( sample );
				if( mIo2->GetBitState() == BIT_HIGH )
					symbol |= 0x4;
				if( mIo3->GetBitState() == BIT_HIGH )
					symbol |= 0x8;
			}

			return symbol;
		};

		while( mChipSelect->GetBitState() == BIT_LOW )
		{
			const U64 next_cs_edge = mChipSelect->GetSampleOfNextEdge();
			const U64 next_clock_edge = mClock->GetSampleOfNextEdge();

			if( next_clock_edge < next_cs_edge )
			{
				mClock->AdvanceToNextEdge();
				mChipSelect->AdvanceToAbsPosition( mClock->GetSampleNumber() );

				if( mChipSelect->GetBitState() == BIT_LOW )
				{
					++clock_edge_count;

					if( mClock->GetBitState() != BIT_HIGH )
						continue;

					if( phase == Phase::Command )
					{
						current_command_byte = U8( ( current_command_byte << bits_per_clock ) | SampleSymbol( false ) );

						current_command_bit_count += bits_per_clock;
						if( current_command_bit_count == 8 )
						{
							const U32 command_byte_index = captured_command_bytes;
							if( captured_command_bytes < kPreviewCommandByteCount )
								preview_bytes |= ( U64( current_command_byte ) << ( captured_command_bytes * 8 ) );

							++captured_command_bytes;

							if( command_byte_index == 0 )
							{
								command_opcode = current_command_byte;
							}
							else if( command_byte_index == 1 )
							{
								command_byte1 = current_command_byte;
							}
							else if( command_byte_index == 2 )
							{
								command_byte2 = current_command_byte;
							}
							else if( command_byte_index == 3 )
							{
								command_byte3 = current_command_byte;
							}

							expected_command_bytes = EspiCommand::GetExpectedByteCount(
								command_opcode, command_byte1, command_byte2, command_byte3 );

							if( command_byte_index > 0 && ( command_opcode == 0x21 || command_opcode == 0x22 ) )
							{
								if( command_byte_index == 1 )
									configuration_address = U16( current_command_byte ) << 8;
								else if( command_byte_index == 2 )
									configuration_address |= current_command_byte;
								else if( command_opcode == 0x22 && command_byte_index >= 3 && command_byte_index <= 6 )
									configuration_value |= U32( current_command_byte ) << ( ( command_byte_index - 3 ) * 8 );
							}
							else if( command_opcode == 0x04 && command_byte_index == 1 )
							{
								transaction_details.virtual_wire_group_count = ( current_command_byte & 0x3f ) + 1;
							}
							else if( command_opcode == 0x04 && command_byte_index >= 2 &&
								command_byte_index < 2 + ( 2 * transaction_details.virtual_wire_group_count ) )
							{
								if( ( command_byte_index & 1 ) == 0 )
								{
									pending_virtual_wire_index = current_command_byte;
								}
								else
								{
									EspiAnalyzerResults::VirtualWireGroup group;
									group.index = pending_virtual_wire_index;
									group.data = current_command_byte;
									transaction_details.virtual_wire_groups.push_back( group );
								}
							}

							current_command_bit_count = 0;
							current_command_byte = 0;

							if( captured_command_bytes >= expected_command_bytes )
								phase = Phase::Turnaround;
						}
					}
					else if( phase == Phase::Turnaround )
					{
						++turnaround_edge_count;
						if( turnaround_edge_count >= 2 )
							phase = Phase::ResponseWaitState;
					}
					else
					{
						current_response_byte = U8( ( current_response_byte << bits_per_clock ) | SampleSymbol( true ) );

						current_response_bit_count += bits_per_clock;
						if( current_response_bit_count == 8 )
						{
							if( phase == Phase::ResponseWaitState )
							{
								if( current_response_byte == 0x0f )
								{
									++observed_wait_state_bytes;
								}
								else
								{
									first_response_byte = current_response_byte;
									RecordResponseByte( current_response_byte );
									if( captured_response_bytes < kPreviewResponseByteCount )
										preview_bytes |= ( U64( current_response_byte ) << ( ( kPreviewCommandByteCount + captured_response_bytes ) * 8 ) );

									++captured_response_bytes;
									phase = Phase::ResponseData;
								}
							}
							else
							{
								const U32 response_byte_index = captured_response_bytes;
								if( command_opcode == 0x21 && response_byte_index >= 1 && response_byte_index <= 4 )
								{
									configuration_value |= U32( current_response_byte ) << ( ( response_byte_index - 1 ) * 8 );
								}
								else if( command_opcode == 0x05 && response_byte_index == 1 )
								{
									transaction_details.virtual_wire_group_count = ( current_response_byte & 0x3f ) + 1;
								}
								else if( command_opcode == 0x05 && response_byte_index >= 2 &&
									response_byte_index < 2 + ( 2 * transaction_details.virtual_wire_group_count ) )
								{
									if( ( response_byte_index & 1 ) == 0 )
									{
										pending_virtual_wire_index = current_response_byte;
									}
									else
									{
										EspiAnalyzerResults::VirtualWireGroup group;
										group.index = pending_virtual_wire_index;
										group.data = current_response_byte;
										transaction_details.virtual_wire_groups.push_back( group );
									}
								}

								if( captured_response_bytes < kPreviewResponseByteCount )
									preview_bytes |= ( U64( current_response_byte ) << ( ( kPreviewCommandByteCount + captured_response_bytes ) * 8 ) );

								RecordResponseByte( current_response_byte );
								++captured_response_bytes;
							}

							current_response_bit_count = 0;
							current_response_byte = 0;
						}
					}
				}

				continue;
			}

			mChipSelect->AdvanceToNextEdge();
			mClock->AdvanceToAbsPosition( mChipSelect->GetSampleNumber() );
		}

		if( ( clock_edge_count == 0 ) && ( captured_command_bytes == 0 ) && ( captured_response_bytes == 0 ) &&
			( current_command_bit_count == 0 ) && ( current_response_bit_count == 0 ) && ( observed_wait_state_bytes == 0 ) )
		{
			continue;
		}

		const U64 transaction_end = mChipSelect->GetSampleNumber();
		EspiIoMode next_io_mode = active_io_mode;
		if( command_opcode == 0xff )
		{
			next_io_mode = EspiIoMode::Single;
		}
		else if( command_opcode == 0x22 && configuration_address == 0x0008 && IsAcceptResponse( first_response_byte ) )
		{
			const U8 requested_mode = U8( ( configuration_value >> 26 ) & 0x03 );
			if( requested_mode <= U8( EspiIoMode::Quad ) )
				next_io_mode = EspiIoMode( requested_mode );
		}

		const bool complete_configuration =
			( command_opcode == 0x21 && captured_command_bytes >= 3 && captured_response_bytes >= 5 ) ||
			( command_opcode == 0x22 && captured_command_bytes >= 7 && captured_response_bytes >= 1 );
		if( complete_configuration && IsAcceptResponse( first_response_byte ) )
		{
			transaction_details.has_configuration = true;
			transaction_details.configuration_is_write = command_opcode == 0x22;
			transaction_details.configuration_address = configuration_address;
			transaction_details.configuration_value = configuration_value;
		}

		// Every driven GET_STATUS response ends with Status[7:0], Status[15:8],
		// and CRC. Keeping a rolling response tail also covers appended channel data.
		if( command_opcode == 0x25 && captured_response_bytes >= 4 && first_response_byte != 0xff )
		{
			transaction_details.has_status = true;
			transaction_details.status = U16( response_tail0 ) | ( U16( response_tail1 ) << 8 );
			transaction_details.response_modifier = U8( ( first_response_byte >> 6 ) & 0x03 );
		}

		Frame frame;
		frame.mType = kTransactionFrame;
		frame.mFlags = 0;
		frame.mData1 = preview_bytes;
		// mData2: mode[63:62], next mode[61:60], rsp partial[59:57],
		// cmd partial[56:54], wait states[53:48],
		// response bytes[47:40], command bytes[39:32], clock edges[31:0].
		frame.mData2 =
			( U64( U8( active_io_mode ) & 0x03 ) << 62 ) |
			( U64( U8( next_io_mode ) & 0x03 ) << 60 ) |
			( U64( current_response_bit_count & 0x07 ) << 57 ) |
			( U64( current_command_bit_count & 0x07 ) << 54 ) |
			( U64( observed_wait_state_bytes & 0x3f ) << 48 ) |
			( U64( captured_response_bytes & 0xff ) << 40 ) |
			( U64( captured_command_bytes & 0xff ) << 32 ) |
			U64( clock_edge_count & 0xffffffffULL );
		frame.mStartingSampleInclusive = transaction_start;
		frame.mEndingSampleInclusive = transaction_end;
		mResults->AddTransactionDetails( transaction_details );
		mResults->AddFrame( frame );
		mResults->CommitResults();
		mResults->CommitPacketAndStartNewPacket();
		ReportProgress( transaction_end );
		active_io_mode = next_io_mode;
		alert_armed = mIo1->GetBitState() == BIT_HIGH;
	}
}

bool EspiAnalyzer::NeedsRerun()
{
	return false;
}

U32 EspiAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 EspiAnalyzer::GetMinimumSampleRateHz()
{
	return 25000;
}

const char* EspiAnalyzer::GetAnalyzerName() const
{
	return "Intel eSPI";
}

const char* GetAnalyzerName()
{
	return "Intel eSPI";
}

Analyzer* CreateAnalyzer()
{
	return new EspiAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
