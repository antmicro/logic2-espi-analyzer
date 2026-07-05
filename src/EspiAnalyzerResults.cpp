#include "EspiAnalyzerResults.h"
#include "EspiCommand.h"
#include <AnalyzerHelpers.h>
#include "EspiAnalyzer.h"
#include "EspiAnalyzerSettings.h"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace
{
	static constexpr U32 kPreviewCommandByteCount = EspiCommand::kPreviewByteCount;
	static constexpr U32 kPreviewResponseByteCount = 4;

	std::string FormatOpcodeName( U8 opcode )
	{
		const std::string name = EspiCommand::GetOpcodeName( opcode );
		if( name != "UNKNOWN_CMD" )
			return name;

		std::ostringstream result;
		result << name << " 0x" << std::hex << std::uppercase << std::setw( 2 )
			   << std::setfill( '0' ) << unsigned( opcode );
		return result.str();
	}

	const char* GetResponseName( U8 response )
	{
		if( response == 0xff )
			return "NO_RESPONSE";

		if( ( response & 0x0f ) == 0x08 && ( response & 0x30 ) == 0x00 )
			return "ACCEPT";
		if( response == 0x01 )
			return "DEFER";
		if( response == 0x02 )
			return "NON_FATAL_ERROR";
		if( response == 0x03 )
			return "FATAL_ERROR";
		if( response == 0x0f )
			return "WAIT_STATE";

		return "UNKNOWN_RSP";
	}

	const char* GetIoModeName( U32 mode )
	{
		switch( mode )
		{
		case 0:
			return "SINGLE";
		case 1:
			return "DUAL";
		case 2:
			return "QUAD";
		default:
			return "RESERVED";
		}
	}

	const char* GetIoModeDisplayName( U32 mode )
	{
		switch( mode )
		{
		case 0:
			return "Single";
		case 1:
			return "Dual";
		case 2:
			return "Quad";
		default:
			return "Reserved";
		}
	}

	const char* GetResponseModifierName( U8 modifier )
	{
		switch( modifier )
		{
		case 0:
			return "none";
		case 1:
			return "peripheral completion";
		case 2:
			return "virtual wire";
		case 3:
			return "flash completion";
		default:
			return "unknown";
		}
	}

	std::string FormatStatusFlags( U16 status )
	{
		static const char* names[16] = {
			"PC_FREE", "NP_FREE", "VWIRE_FREE", "OOB_FREE",
			"PC_AVAIL", "NP_AVAIL", "VWIRE_AVAIL", "OOB_AVAIL",
			"FLASH_C_FREE", "FLASH_NP_FREE", nullptr, nullptr,
			"FLASH_C_AVAIL", "FLASH_NP_AVAIL", nullptr, nullptr
		};

		std::ostringstream result;
		bool first = true;
		for( U32 bit = 0; bit < 16; ++bit )
		{
			if( names[bit] == nullptr || ( status & ( U16( 1 ) << bit ) ) == 0 )
				continue;
			if( !first )
				result << '|';
			result << names[bit];
			first = false;
		}
		if( first )
			result << "none";

		const U16 reserved = status & 0xcc00;
		if( reserved != 0 )
			result << "|RESERVED_SET";
		return result.str();
	}

	std::string FormatPendingService( U16 status )
	{
		static const char* commands[16] = {
			nullptr, nullptr, nullptr, nullptr,
			"GET_PC", "GET_NP", "GET_VWIRE", "GET_OOB",
			nullptr, nullptr, nullptr, nullptr,
			"GET_FLASH_C", "GET_FLASH_NP", nullptr, nullptr
		};

		std::ostringstream result;
		for( U32 bit = 0; bit < 16; ++bit )
		{
			if( commands[bit] == nullptr || ( status & ( U16( 1 ) << bit ) ) == 0 )
				continue;
			if( result.tellp() > 0 )
				result << ", ";
			result << commands[bit];
		}
		return result.str();
	}

	std::string FormatFreeQueues( U16 status )
	{
		static const U32 bits[] = { 0, 1, 2, 3, 8, 9 };
		static const char* names[] = { "PC", "NP", "VWIRE", "OOB", "FLASH_C", "FLASH_NP" };
		std::ostringstream result;
		for( U32 i = 0; i < sizeof( bits ) / sizeof( bits[0] ); ++i )
		{
			if( ( status & ( U16( 1 ) << bits[i] ) ) == 0 )
				continue;
			if( result.tellp() > 0 )
				result << ", ";
			result << names[i];
		}
		return result.str();
	}

	std::string FormatStatusDetailsMultiline( const EspiAnalyzerResults::TransactionDetails& details )
	{
		if( !details.has_status )
			return "";

		std::ostringstream result;
		result << "  Status 0x" << std::hex << std::uppercase << std::setw( 4 ) << std::setfill( '0' ) << details.status;
		const std::string free_queues = FormatFreeQueues( details.status );
		if( !free_queues.empty() )
			result << " | Free: " << free_queues;
		if( details.response_modifier != 0 )
			result << " | Append: " << GetResponseModifierName( details.response_modifier );
		if( ( details.status & 0xcc00 ) != 0 )
			result << " | Reserved bits set";
		return result.str();
	}

	const char* GetConfigurationRegisterName( U16 address )
	{
		switch( address )
		{
		case 0x0004:
			return "Device Identification";
		case 0x0008:
			return "General";
		case 0x0010:
			return "Peripheral";
		case 0x0020:
			return "Virtual Wire";
		case 0x0030:
			return "OOB";
		case 0x0040:
			return "Flash";
		case 0x0044:
			return "Flash 2";
		case 0x0048:
			return "Flash 3";
		case 0x004c:
			return "Flash 4";
		default:
			return address >= 0x0800 ? "Platform-Specific Configuration" : "Unknown Configuration Register";
		}
	}

	const char* GetTransferSizeName( U32 encoding )
	{
		switch( encoding )
		{
		case 1:
			return "64 bytes";
		case 2:
			return "128 bytes";
		case 3:
			return "256 bytes";
		case 4:
			return "512 bytes";
		case 5:
			return "1024 bytes";
		case 6:
			return "2048 bytes";
		case 7:
			return "4096 bytes";
		default:
			return "Reserved";
		}
	}

	const char* GetFrequencyName( U32 encoding )
	{
		switch( encoding )
		{
		case 0:
			return "20 MHz";
		case 1:
			return "25 MHz";
		case 2:
			return "33 MHz";
		case 3:
			return "50 MHz";
		case 4:
			return "66 MHz";
		default:
			return "Reserved";
		}
	}

	std::string FormatConfigurationDetailsMultiline( const EspiAnalyzerResults::TransactionDetails& details )
	{
		if( !details.has_configuration )
			return "";

		const U16 address = details.configuration_address;
		const U32 value = details.configuration_value;
		std::ostringstream result;
		result << GetConfigurationRegisterName( address ) << " [0x" << std::hex << std::uppercase
			   << std::setw( 4 ) << std::setfill( '0' ) << address << "] = 0x"
			   << std::setw( 8 ) << std::setfill( '0' ) << value;

		switch( address )
		{
		case 0x0004:
			result << "\n  Version ID 0x" << std::setw( 2 ) << ( value & 0xff );
			break;

		case 0x0008:
			result << "\n  " << GetIoModeDisplayName( ( value >> 26 ) & 0x03 ) << " I/O @ " << GetFrequencyName( ( value >> 20 ) & 0x07 );
			result << " | CRC " << ( ( value & ( 1U << 31 ) ) != 0 ? "on" : "off" );
			if( ( value & ( 1U << 30 ) ) != 0 )
				result << " | Response modifier on";
			result << " | Alert " << ( ( value & ( 1U << 28 ) ) != 0 ? "dedicated" : "I/O[1]" );
			result << " | Max wait " << std::dec << ( ( ( value >> 12 ) & 0x0f ) == 0 ? 16 : ( ( value >> 12 ) & 0x0f ) );
			break;

		case 0x0010:
			result << "\n  " << ( ( value & 0x01 ) != 0 ? "Enabled" : "Disabled" );
			result << " | " << ( ( value & 0x02 ) != 0 ? "Ready" : "Not ready" );
			result << " | Payload " << GetTransferSizeName( ( value >> 8 ) & 0x07 );
			result << " | Read request " << GetTransferSizeName( ( value >> 12 ) & 0x07 );
			if( ( value & 0x04 ) != 0 )
				result << " | Bus master on";
			break;

		case 0x0020:
			result << "\n  " << ( ( value & 0x01 ) != 0 ? "Enabled" : "Disabled" );
			result << " | " << ( ( value & 0x02 ) != 0 ? "Ready" : "Not ready" );
			result << " | Selected " << std::dec << ( ( ( value >> 16 ) & 0x3f ) + 1 );
			result << " / Supported " << ( ( ( value >> 8 ) & 0x3f ) + 1 );
			break;

		case 0x0030:
			result << "\n  " << ( ( value & 0x01 ) != 0 ? "Enabled" : "Disabled" );
			result << " | " << ( ( value & 0x02 ) != 0 ? "Ready" : "Not ready" );
			result << " | Payload " << GetTransferSizeName( ( value >> 8 ) & 0x07 );
			result << " / Supported " << GetTransferSizeName( ( value >> 4 ) & 0x07 );
			break;

		case 0x0040:
			result << "\n  " << ( ( value & 0x01 ) != 0 ? "Enabled" : "Disabled" );
			result << " | " << ( ( value & 0x02 ) != 0 ? "Ready" : "Not ready" );
			result << " | " << ( ( value & ( 1U << 11 ) ) != 0 ? "Target-attached" : "Controller-attached" );
			result << " | Payload " << GetTransferSizeName( ( value >> 8 ) & 0x07 );
			result << " | Read request " << GetTransferSizeName( ( value >> 12 ) & 0x07 );
			break;
		}

		return result.str();
	}

	std::string FormatShortIoDetailsMultiline( const EspiAnalyzerResults::TransactionDetails& details )
	{
		if( !details.has_short_io )
			return "";

		std::ostringstream result;
		result << "  " << ( details.short_io_is_write ? "Write" : "Read" ) << ' '
			   << std::dec << unsigned( details.short_io_size ) << ( details.short_io_size == 1 ? " byte" : " bytes" )
			   << " @ 0x" << std::hex << std::uppercase << std::setw( 4 ) << std::setfill( '0' )
			   << details.short_io_address;
		if( details.short_io_has_data )
		{
			result << " | Data 0x" << std::setw( details.short_io_size * 2 ) << std::setfill( '0' )
				   << details.short_io_data;
		}
		if( details.short_io_has_status )
		{
			result << "\n  Status 0x" << std::setw( 4 ) << std::setfill( '0' ) << details.short_io_status;
			const std::string status_flags = FormatStatusFlags( details.short_io_status );
			if( !status_flags.empty() )
				result << " | " << status_flags;
		}
		return result.str();
	}

	const char* GetSystemEventName( U8 index, U32 slot )
	{
		static const char* names[6][4] = {
			{ "SLP_S3#", "SLP_S4#", "SLP_S5#", nullptr },
			{ "SUS_STAT#", "PLTRST#", "OOB_RST_WARN", nullptr },
			{ "OOB_RST_ACK", nullptr, "WAKE#", "PME#" },
			{ "TARGET_BOOT_LOAD_DONE", "ERROR_FATAL", "ERROR_NONFATAL", "TARGET_BOOT_LOAD_STATUS" },
			{ "SCI#", "SMI#", "RCIN#", "HOST_RST_ACK" },
			{ "HOST_RST_WARN", "SMIOUT#", "NMIOUT#", nullptr }
		};

		if( index < 2 || index > 7 || slot >= 4 )
			return nullptr;
		return names[index - 2][slot];
	}

	std::string FormatVirtualWireGroupBrief( const EspiAnalyzerResults::VirtualWireGroup& group )
	{
		std::ostringstream result;
		result << "VW[" << unsigned( group.index ) << "] ";

		if( group.index <= 1 )
		{
			const U32 irq = ( U32( group.index ) * 128 ) + ( group.data & 0x7f );
			result << "IRQ" << irq << '=' << ( ( group.data & 0x80 ) != 0 ? "HIGH" : "LOW" );
			return result.str();
		}

		bool first_signal = true;
		if( group.index >= 2 && group.index <= 7 )
		{
			for( U32 slot = 0; slot < 4; ++slot )
			{
				if( ( group.data & ( 1U << ( slot + 4 ) ) ) == 0 )
					continue;

				const char* name = GetSystemEventName( group.index, slot );
				if( name != nullptr )
				{
					if( !first_signal )
						result << ' ';
					result << name << '=' << ( ( group.data & ( 1U << slot ) ) != 0 ? "HIGH" : "LOW" );
					first_signal = false;
				}
			}
		}

		if( first_signal )
			result << "data=" << unsigned( group.data );

		return result.str();
	}

	std::string FormatVirtualWireDetails( const EspiAnalyzerResults::TransactionDetails& details )
	{
		if( details.virtual_wire_group_count == 0 )
			return "";

		std::ostringstream result;
		result << "VW ";
		for( size_t i = 0; i < details.virtual_wire_groups.size(); ++i )
		{
			if( i != 0 )
				result << " | ";
			result << FormatVirtualWireGroupBrief( details.virtual_wire_groups[i] );
		}
		if( details.virtual_wire_groups.empty() )
			result << "groups=" << details.virtual_wire_group_count;
		return result.str();
	}

	std::string FormatVirtualWireDetailsMultiline( const EspiAnalyzerResults::TransactionDetails& details )
	{
		if( details.virtual_wire_group_count == 0 )
			return "";

		std::ostringstream result;
		for( size_t i = 0; i < details.virtual_wire_groups.size(); ++i )
		{
			const auto& group = details.virtual_wire_groups[i];
			if( i != 0 )
				result << '\n';
			result << "  idx=" << std::setw( 2 ) << std::setfill( '0' ) << std::dec << unsigned( group.index );

			if( group.index <= 1 )
			{
				const U32 irq = ( U32( group.index ) * 128 ) + ( group.data & 0x7f );
				result << "\n    IRQ" << irq << '=' << ( ( group.data & 0x80 ) != 0 ? "HIGH" : "LOW" );
				continue;
			}

			bool any_signal = false;
			if( group.index >= 2 && group.index <= 7 )
			{
				for( U32 slot = 0; slot < 4; ++slot )
				{
					if( ( group.data & ( 1U << ( slot + 4 ) ) ) == 0 )
						continue;

					const char* name = GetSystemEventName( group.index, slot );
					if( name != nullptr )
					{
						result << "\n    " << name << '=' << ( ( group.data & ( 1U << slot ) ) != 0 ? "HIGH" : "LOW" );
						any_signal = true;
					}
				}
			}

			if( !any_signal )
				result << "\n    data=" << std::hex << std::uppercase << std::setw( 2 ) << std::setfill( '0' ) << unsigned( group.data );
		}
		return result.str();
	}

#ifdef ESPI_DEBUG_TRANSACTION_DETAILS
	std::string FormatVerboseTransactionDetails( const EspiAnalyzerResults::TransactionDetails& details )
	{
		if( details.virtual_wire_group_count == 0 )
			return "";

		std::ostringstream result;
		result << "vw_groups=" << details.virtual_wire_group_count;
		if( details.virtual_wire_groups.size() != details.virtual_wire_group_count )
			result << " decoded=" << details.virtual_wire_groups.size();

		for( size_t i = 0; i < details.virtual_wire_groups.size(); ++i )
		{
			const auto& group = details.virtual_wire_groups[i];
			char group_str[32];
			snprintf( group_str, sizeof( group_str ), "%sidx=%02X data=%02X", i == 0 ? " [" : "; ", group.index, group.data );
			result << group_str;
		}
		if( !details.virtual_wire_groups.empty() )
			result << ']';
		return result.str();
	}
#endif
}

EspiAnalyzerResults::EspiAnalyzerResults( EspiAnalyzer* analyzer, EspiAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

EspiAnalyzerResults::~EspiAnalyzerResults()
{
}

void EspiAnalyzerResults::AddTransactionDetails( const TransactionDetails& details )
{
	std::lock_guard<std::mutex> lock( mTransactionDetailsMutex );
	mTransactionDetails.push_back( details );
}

bool EspiAnalyzerResults::GetTransactionDetails( U64 frame_index, TransactionDetails& details ) const
{
	std::lock_guard<std::mutex> lock( mTransactionDetailsMutex );
	if( frame_index >= mTransactionDetails.size() )
		return false;
	details = mTransactionDetails[size_t( frame_index )];
	return true;
}

void EspiAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
	if( ( frame.mType == TransactionFrame && channel != mSettings->mChipSelectChannel ) ||
		( frame.mType == AlertFrame && channel != mSettings->mIo1Channel ) )
		return;
	TransactionDetails transaction_details;
	GetTransactionDetails( frame_index, transaction_details );

	switch( frame.mType )
	{
	case TransactionFrame:
		{
			const U32 edge_count = U32( frame.mData2 & 0xffffffffULL );
			const U32 cmd_byte_count = U32( ( frame.mData2 >> 32 ) & 0xffULL );
			const U32 rsp_byte_count = U32( ( frame.mData2 >> 40 ) & 0xffULL );
			const U32 wait_state_count = U32( ( frame.mData2 >> 48 ) & 0x3fULL );
			const U32 cmd_partial_bits = U32( ( frame.mData2 >> 54 ) & 0x07ULL );
			const U32 rsp_partial_bits = U32( ( frame.mData2 >> 57 ) & 0x07ULL );
			const U32 next_io_mode = U32( ( frame.mData2 >> 60 ) & 0x03ULL );
			const U32 io_mode = U32( ( frame.mData2 >> 62 ) & 0x03ULL );
			const U32 cmd_preview_count = cmd_byte_count < kPreviewCommandByteCount ? cmd_byte_count : kPreviewCommandByteCount;
			const U32 rsp_preview_count = rsp_byte_count < kPreviewResponseByteCount ? rsp_byte_count : kPreviewResponseByteCount;
			const U32 cmd_b0 = cmd_preview_count >= 1 ? U32( frame.mData1 & 0xffULL ) : 0;
			const U32 cmd_b1 = cmd_preview_count >= 2 ? U32( ( frame.mData1 >> 8 ) & 0xffULL ) : 0;
			const U32 cmd_b2 = cmd_preview_count >= 3 ? U32( ( frame.mData1 >> 16 ) & 0xffULL ) : 0;
			const U32 cmd_b3 = cmd_preview_count >= 4 ? U32( ( frame.mData1 >> 24 ) & 0xffULL ) : 0;
			const U32 cmd_prefix32 = cmd_preview_count >= 4 ? U32( frame.mData1 & 0xffffffffULL ) : 0;
			const U32 expected_byte_count = cmd_preview_count >= 1 ? EspiCommand::GetExpectedByteCount( U8( cmd_b0 ), U8( cmd_b1 ), U8( cmd_b2 ), U8( cmd_b3 ) ) : kPreviewCommandByteCount;
			const U32 rsp_b0 = rsp_preview_count >= 1 ? U32( ( frame.mData1 >> 32 ) & 0xffULL ) : 0;
			const U32 rsp_b1 = rsp_preview_count >= 2 ? U32( ( frame.mData1 >> 40 ) & 0xffULL ) : 0;
			const U32 rsp_b2 = rsp_preview_count >= 3 ? U32( ( frame.mData1 >> 48 ) & 0xffULL ) : 0;
			const U32 rsp_b3 = rsp_preview_count >= 4 ? U32( ( frame.mData1 >> 56 ) & 0xffULL ) : 0;
			const std::string opcode_name = cmd_preview_count >= 1 ? FormatOpcodeName( U8( cmd_b0 ) ) : "UNKNOWN_CMD";
			const char* response_name = rsp_preview_count >= 1 ? GetResponseName( U8( rsp_b0 ) ) : "NO_RESPONSE";
			const std::string virtual_wire_details = FormatVirtualWireDetails( transaction_details );
			const std::string virtual_wire_details_multiline = FormatVirtualWireDetailsMultiline( transaction_details );
			const std::string configuration_details_multiline = FormatConfigurationDetailsMultiline( transaction_details );
			const std::string status_details_multiline = FormatStatusDetailsMultiline( transaction_details );
			const std::string short_io_details_multiline = FormatShortIoDetailsMultiline( transaction_details );

#ifdef ESPI_DEBUG_TRANSACTION_DETAILS
			char mode_str[48];
			if( io_mode == next_io_mode )
				snprintf( mode_str, sizeof( mode_str ), "mode %s", GetIoModeName( io_mode ) );
			else
				snprintf( mode_str, sizeof( mode_str ), "mode %s -> %s", GetIoModeName( io_mode ), GetIoModeName( next_io_mode ) );
			AddResultString( mode_str );

			if( cmd_preview_count >= 1 )
				AddResultString( opcode_name.c_str() );
			if( !configuration_details_multiline.empty() )
				AddResultString( configuration_details_multiline.c_str() );
			if( !status_details_multiline.empty() )
				AddResultString( status_details_multiline.c_str() );
			if( !short_io_details_multiline.empty() )
				AddResultString( short_io_details_multiline.c_str() );
			if( wait_state_count > 0 )
			{
				char wait_state_str[48];
				snprintf( wait_state_str, sizeof( wait_state_str ), "WAIT_STATE x%u", wait_state_count );
				AddResultString( wait_state_str );
			}
			if( rsp_preview_count >= 1 )
				AddResultString( response_name );
			const std::string verbose_details = FormatVerboseTransactionDetails( transaction_details );
			if( !verbose_details.empty() )
				AddResultString( verbose_details.c_str() );

			if( cmd_preview_count >= 4 )
			{
				char sig4_str[32];
				snprintf( sig4_str, sizeof( sig4_str ), "%02X %02X %02X %02X", cmd_b0, cmd_b1, cmd_b2, cmd_b3 );
				AddResultString( sig4_str );

				char sig4_labeled_str[48];
				snprintf( sig4_labeled_str, sizeof( sig4_labeled_str ), "cmd4: %02X %02X %02X %02X", cmd_b0, cmd_b1, cmd_b2, cmd_b3 );
				AddResultString( sig4_labeled_str );

				char prefix32_str[32];
				snprintf( prefix32_str, sizeof( prefix32_str ), "cmd32: %08X", cmd_prefix32 );
				AddResultString( prefix32_str );
			}

			if( cmd_preview_count >= 2 )
			{
				char first_two_bytes_str[32];
				snprintf( first_two_bytes_str, sizeof( first_two_bytes_str ), "cmd0/1: %02X %02X", cmd_b0, cmd_b1 );
				AddResultString( first_two_bytes_str );
			}

			if( rsp_preview_count >= 1 )
			{
				char rsp_bytes_str[64];
				if( rsp_preview_count >= 4 )
					snprintf( rsp_bytes_str, sizeof( rsp_bytes_str ), "rsp: %02X %02X %02X %02X", rsp_b0, rsp_b1, rsp_b2, rsp_b3 );
				else if( rsp_preview_count >= 2 )
					snprintf( rsp_bytes_str, sizeof( rsp_bytes_str ), "rsp: %02X %02X", rsp_b0, rsp_b1 );
				else
					snprintf( rsp_bytes_str, sizeof( rsp_bytes_str ), "rsp: %02X", rsp_b0 );
				AddResultString( rsp_bytes_str );
			}

			std::ostringstream cmd_preview;
			cmd_preview << "cmd:";
			for( U32 i = 0; i < cmd_preview_count; ++i )
			{
				const U32 byte_value = U32( ( frame.mData1 >> ( i * 8 ) ) & 0xff );
				char byte_str[16];
				snprintf( byte_str, sizeof( byte_str ), "%02X", byte_value );
				cmd_preview << ' ' << byte_str;
			}
			AddResultString( cmd_preview.str().c_str() );

			std::ostringstream rsp_preview;
			rsp_preview << "rsp:";
			for( U32 i = 0; i < rsp_preview_count; ++i )
			{
				const U32 byte_value = U32( ( frame.mData1 >> ( ( kPreviewCommandByteCount + i ) * 8 ) ) & 0xff );
				char byte_str[16];
				snprintf( byte_str, sizeof( byte_str ), "%02X", byte_value );
				rsp_preview << ' ' << byte_str;
			}
			if( rsp_preview_count > 0 )
				AddResultString( rsp_preview.str().c_str() );

			char edge_summary[192];
			snprintf( edge_summary, sizeof( edge_summary ), "mode=%s edges=%u cmd_bytes=%u rsp_bytes=%u wait_states=%u expected=%u cmd_partial=%u rsp_partial=%u", GetIoModeName( io_mode ), edge_count, cmd_byte_count, rsp_byte_count, wait_state_count, expected_byte_count, cmd_partial_bits, rsp_partial_bits );
			AddResultString( edge_summary );

			AddResultString( "eSPI cmd/rsp" );
			AddResultString( "CMD/RSP" );
			AddResultString( "eSPI transaction" );
#else
			if( cmd_preview_count >= 1 )
			{
				AddResultString( opcode_name.c_str() );

				const std::string& details = !status_details_multiline.empty() ? status_details_multiline :
					( !configuration_details_multiline.empty() ? configuration_details_multiline :
						( !short_io_details_multiline.empty() ? short_io_details_multiline : virtual_wire_details_multiline ) );
				if( !details.empty() )
				{
					std::ostringstream formatted;
					if( transaction_details.has_configuration )
					{
						formatted << opcode_name << " - " << details;
					}
					else if( transaction_details.has_status )
					{
						formatted << opcode_name;
						const std::string pending = FormatPendingService( transaction_details.status );
						if( !pending.empty() )
							formatted << " - " << pending << " pending";
						if( ( rsp_b0 & 0x0f ) != 0x08 )
							formatted << " - " << response_name;
						formatted << '\n' << details;
						if( !virtual_wire_details_multiline.empty() )
							formatted << '\n' << virtual_wire_details_multiline;
					}
					else if( transaction_details.has_short_io )
					{
						formatted << opcode_name;
						if( rsp_preview_count == 0 || ( rsp_b0 & 0x0f ) != 0x08 )
							formatted << " - " << response_name;
						formatted << '\n' << details;
					}
					else
					{
						formatted << opcode_name << '\n' << details;
					}
					AddResultString( formatted.str().c_str() );
				}
			}
#endif
			break;
		}

	case AlertFrame:
		AddResultString( "ALERT#" );
		AddResultString( "ALERT# - Target requests service" );
		break;

	default:
		AddResultString( "Unknown eSPI frame" );
		break;
	}
}

void EspiAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Type,IoMode,NextIoMode,EdgeCount,CmdByteCount,RspByteCount,WaitStateCount,ExpectedCmdByteCount,CmdPartialBits,RspPartialBits,CmdByte0,CmdByte1,CmdPrefix32,RspByte0,RspByte1,CmdPreviewBytes,RspPreviewBytes,OpcodeName,ResponseName,Status,StatusFlags,ResponseModifier,VirtualWireDetails,ConfigurationDetails,ShortIoDetails" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		TransactionDetails transaction_details;
		GetTransactionDetails( i, transaction_details );
		const std::string virtual_wire_details = FormatVirtualWireDetails( transaction_details );
		const std::string configuration_details = FormatConfigurationDetailsMultiline( transaction_details );
		const std::string short_io_details = FormatShortIoDetailsMultiline( transaction_details );
		const std::string status_flags = transaction_details.has_status ? FormatStatusFlags( transaction_details.status ) : "";
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
		if( frame.mType == AlertFrame )
		{
			file_stream << time_str << ",alert";
			for( U32 column = 0; column < 24; ++column )
				file_stream << ',';
			file_stream << std::endl;
			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				file_stream.close();
				return;
			}
			continue;
		}

		const U32 edge_count = U32( frame.mData2 & 0xffffffffULL );
		const U32 cmd_byte_count = U32( ( frame.mData2 >> 32 ) & 0xffULL );
		const U32 rsp_byte_count = U32( ( frame.mData2 >> 40 ) & 0xffULL );
		const U32 wait_state_count = U32( ( frame.mData2 >> 48 ) & 0x3fULL );
		const U32 cmd_partial_bits = U32( ( frame.mData2 >> 54 ) & 0x07ULL );
		const U32 rsp_partial_bits = U32( ( frame.mData2 >> 57 ) & 0x07ULL );
		const U32 next_io_mode = U32( ( frame.mData2 >> 60 ) & 0x03ULL );
		const U32 io_mode = U32( ( frame.mData2 >> 62 ) & 0x03ULL );
		const U32 cmd_preview_count = cmd_byte_count < kPreviewCommandByteCount ? cmd_byte_count : kPreviewCommandByteCount;
		const U32 rsp_preview_count = rsp_byte_count < kPreviewResponseByteCount ? rsp_byte_count : kPreviewResponseByteCount;
		const U32 cmd_byte0 = cmd_preview_count >= 1 ? U32( frame.mData1 & 0xffULL ) : 0;
		const U32 cmd_byte1 = cmd_preview_count >= 2 ? U32( ( frame.mData1 >> 8 ) & 0xffULL ) : 0;
		const U32 cmd_byte2 = cmd_preview_count >= 3 ? U32( ( frame.mData1 >> 16 ) & 0xffULL ) : 0;
		const U32 cmd_byte3 = cmd_preview_count >= 4 ? U32( ( frame.mData1 >> 24 ) & 0xffULL ) : 0;
		const U32 cmd_prefix32 = cmd_preview_count >= 4 ? U32( frame.mData1 & 0xffffffffULL ) : 0;
		const U32 expected_byte_count = cmd_preview_count >= 1 ? EspiCommand::GetExpectedByteCount( U8( cmd_byte0 ), U8( cmd_byte1 ), U8( cmd_byte2 ), U8( cmd_byte3 ) ) : kPreviewCommandByteCount;
		const U32 rsp_byte0 = rsp_preview_count >= 1 ? U32( ( frame.mData1 >> 32 ) & 0xffULL ) : 0;
		const U32 rsp_byte1 = rsp_preview_count >= 2 ? U32( ( frame.mData1 >> 40 ) & 0xffULL ) : 0;
		const std::string opcode_name = cmd_preview_count >= 1 ? FormatOpcodeName( U8( cmd_byte0 ) ) : "UNKNOWN_CMD";
		const char* response_name = rsp_preview_count >= 1 ? GetResponseName( U8( rsp_byte0 ) ) : "NO_RESPONSE";

		std::ostringstream cmd_preview;
		for( U32 i = 0; i < cmd_preview_count; ++i )
		{
			if( i != 0 )
				cmd_preview << ' ';
			const U32 byte_value = U32( ( frame.mData1 >> ( i * 8 ) ) & 0xff );
			char byte_str[16];
			snprintf( byte_str, sizeof( byte_str ), "%02X", byte_value );
			cmd_preview << byte_str;
		}

		std::ostringstream rsp_preview;
		for( U32 i = 0; i < rsp_preview_count; ++i )
		{
			if( i != 0 )
				rsp_preview << ' ';
			const U32 byte_value = U32( ( frame.mData1 >> ( ( kPreviewCommandByteCount + i ) * 8 ) ) & 0xff );
			char byte_str[16];
			snprintf( byte_str, sizeof( byte_str ), "%02X", byte_value );
			rsp_preview << byte_str;
		}

		const char* type_str = frame.mType == TransactionFrame ? "transaction" : "unknown";
		char cmd_byte0_str[16];
		char cmd_byte1_str[16];
		char cmd_prefix32_str[16];
		char rsp_byte0_str[16];
		char rsp_byte1_str[16];
		snprintf( cmd_byte0_str, sizeof( cmd_byte0_str ), cmd_preview_count >= 1 ? "%02X" : "" , cmd_byte0 );
		snprintf( cmd_byte1_str, sizeof( cmd_byte1_str ), cmd_preview_count >= 2 ? "%02X" : "" , cmd_byte1 );
		snprintf( cmd_prefix32_str, sizeof( cmd_prefix32_str ), cmd_preview_count >= 4 ? "%08X" : "" , cmd_prefix32 );
		snprintf( rsp_byte0_str, sizeof( rsp_byte0_str ), rsp_preview_count >= 1 ? "%02X" : "" , rsp_byte0 );
		snprintf( rsp_byte1_str, sizeof( rsp_byte1_str ), rsp_preview_count >= 2 ? "%02X" : "" , rsp_byte1 );
		char status_str[16];
		snprintf( status_str, sizeof( status_str ), transaction_details.has_status ? "%04X" : "", transaction_details.status );
		file_stream << time_str << "," << type_str << "," << GetIoModeName( io_mode ) << "," << GetIoModeName( next_io_mode ) << "," << edge_count << "," << cmd_byte_count << "," << rsp_byte_count << "," << wait_state_count << "," << expected_byte_count << "," << cmd_partial_bits << "," << rsp_partial_bits << "," << cmd_byte0_str << "," << cmd_byte1_str << "," << cmd_prefix32_str << "," << rsp_byte0_str << "," << rsp_byte1_str << "," << cmd_preview.str() << "," << rsp_preview.str() << "," << opcode_name << "," << response_name << "," << status_str << ",\"" << status_flags << "\"," << ( transaction_details.has_status ? GetResponseModifierName( transaction_details.response_modifier ) : "" ) << ",\"" << virtual_wire_details << "\",\"" << configuration_details << "\",\"" << short_io_details << "\"" << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void EspiAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	static_cast<void>( display_base );
	Frame frame = GetFrame( frame_index );
	TransactionDetails transaction_details;
	GetTransactionDetails( frame_index, transaction_details );
	ClearTabularText();
	if( frame.mType == AlertFrame )
	{
		AddTabularText( "ALERT# - Target requests service\n" );
		return;
	}
	if( frame.mType != TransactionFrame )
	{
		AddTabularText( "unknown" );
		return;
	}

	const U32 edge_count = U32( frame.mData2 & 0xffffffffULL );
	const U32 cmd_byte_count = U32( ( frame.mData2 >> 32 ) & 0xffULL );
	const U32 rsp_byte_count = U32( ( frame.mData2 >> 40 ) & 0xffULL );
	const U32 wait_state_count = U32( ( frame.mData2 >> 48 ) & 0x3fULL );
	const U32 cmd_partial_bits = U32( ( frame.mData2 >> 54 ) & 0x07ULL );
	const U32 rsp_partial_bits = U32( ( frame.mData2 >> 57 ) & 0x07ULL );
	const U32 next_io_mode = U32( ( frame.mData2 >> 60 ) & 0x03ULL );
	const U32 io_mode = U32( ( frame.mData2 >> 62 ) & 0x03ULL );
	const U32 cmd_preview_count = cmd_byte_count < kPreviewCommandByteCount ? cmd_byte_count : kPreviewCommandByteCount;
	const U32 rsp_preview_count = rsp_byte_count < kPreviewResponseByteCount ? rsp_byte_count : kPreviewResponseByteCount;
	const U32 cmd_byte0 = cmd_preview_count >= 1 ? U32( frame.mData1 & 0xffULL ) : 0;
	const U32 cmd_byte1 = cmd_preview_count >= 2 ? U32( ( frame.mData1 >> 8 ) & 0xffULL ) : 0;
	const U32 cmd_byte2 = cmd_preview_count >= 3 ? U32( ( frame.mData1 >> 16 ) & 0xffULL ) : 0;
	const U32 cmd_byte3 = cmd_preview_count >= 4 ? U32( ( frame.mData1 >> 24 ) & 0xffULL ) : 0;
	const U32 cmd_prefix32 = cmd_preview_count >= 4 ? U32( frame.mData1 & 0xffffffffULL ) : 0;
	const U32 expected_byte_count = cmd_preview_count >= 1 ? EspiCommand::GetExpectedByteCount( U8( cmd_byte0 ), U8( cmd_byte1 ), U8( cmd_byte2 ), U8( cmd_byte3 ) ) : kPreviewCommandByteCount;
	const U32 rsp_byte0 = rsp_preview_count >= 1 ? U32( ( frame.mData1 >> 32 ) & 0xffULL ) : 0;
	const std::string opcode_name = cmd_preview_count >= 1 ? FormatOpcodeName( U8( cmd_byte0 ) ) : "UNKNOWN_CMD";
	const char* response_name = rsp_preview_count >= 1 ? GetResponseName( U8( rsp_byte0 ) ) : "NO_RESPONSE";
		const std::string virtual_wire_details = FormatVirtualWireDetails( transaction_details );
		const std::string virtual_wire_details_multiline = FormatVirtualWireDetailsMultiline( transaction_details );
		const std::string configuration_details_multiline = FormatConfigurationDetailsMultiline( transaction_details );
		const std::string status_details_multiline = FormatStatusDetailsMultiline( transaction_details );
		const std::string short_io_details_multiline = FormatShortIoDetailsMultiline( transaction_details );
#ifdef ESPI_DEBUG_TRANSACTION_DETAILS
		char mode_summary[32];
		if( io_mode == next_io_mode )
			snprintf( mode_summary, sizeof( mode_summary ), "%s", GetIoModeName( io_mode ) );
		else
			snprintf( mode_summary, sizeof( mode_summary ), "%s->%s", GetIoModeName( io_mode ), GetIoModeName( next_io_mode ) );

		char summary[320];
		if( cmd_preview_count >= 4 )
		{
			snprintf(
				summary,
				sizeof( summary ),
				"opcode=%s mode=%s rsp=%s edges=%u cmd_bytes=%u rsp_bytes=%u wait_states=%u expected=%u cmd_partial=%u rsp_partial=%u cmd_b0=%02X cmd_b1=%02X cmd32=%08X\n",
				opcode_name.c_str(),
				mode_summary,
				response_name,
				edge_count,
				cmd_byte_count,
				rsp_byte_count,
				wait_state_count,
				expected_byte_count,
				cmd_partial_bits,
				rsp_partial_bits,
				cmd_byte0,
				cmd_byte1,
				cmd_prefix32 );
		}
		else if( cmd_preview_count >= 2 )
		{
			snprintf(
				summary,
				sizeof( summary ),
				"opcode=%s mode=%s rsp=%s edges=%u cmd_bytes=%u rsp_bytes=%u wait_states=%u expected=%u cmd_partial=%u rsp_partial=%u cmd_b0=%02X cmd_b1=%02X\n",
				opcode_name.c_str(),
				mode_summary,
				response_name,
				edge_count,
				cmd_byte_count,
				rsp_byte_count,
				wait_state_count,
				expected_byte_count,
				cmd_partial_bits,
				rsp_partial_bits,
				cmd_byte0,
				cmd_byte1 );
		}
		else if( cmd_preview_count >= 1 )
		{
			snprintf(
				summary,
				sizeof( summary ),
				"opcode=%s mode=%s rsp=%s edges=%u cmd_bytes=%u rsp_bytes=%u wait_states=%u expected=%u cmd_partial=%u rsp_partial=%u cmd_b0=%02X\n",
				opcode_name.c_str(),
				mode_summary,
				response_name,
				edge_count,
				cmd_byte_count,
				rsp_byte_count,
				wait_state_count,
				expected_byte_count,
				cmd_partial_bits,
				rsp_partial_bits,
				cmd_byte0 );
		}
		else
		{
			snprintf(
				summary,
				sizeof( summary ),
				"mode=%s rsp=%s edges=%u cmd_bytes=%u rsp_bytes=%u wait_states=%u expected=%u cmd_partial=%u rsp_partial=%u\n",
				mode_summary,
				response_name,
				edge_count,
				cmd_byte_count,
				rsp_byte_count,
				wait_state_count,
				expected_byte_count,
				cmd_partial_bits,
				rsp_partial_bits );
		}

		std::string tabular_summary( summary );
		if( !virtual_wire_details.empty() )
		{
			if( !tabular_summary.empty() && tabular_summary[tabular_summary.size() - 1] == '\n' )
				tabular_summary.erase( tabular_summary.size() - 1 );
			tabular_summary += ' ';
			tabular_summary += FormatVerboseTransactionDetails( transaction_details );
			tabular_summary += '\n';
		}
		if( !status_details_multiline.empty() )
		{
			if( !tabular_summary.empty() && tabular_summary[tabular_summary.size() - 1] != '\n' )
				tabular_summary += '\n';
			tabular_summary += status_details_multiline;
			tabular_summary += '\n';
		}
		if( !short_io_details_multiline.empty() )
		{
			if( !tabular_summary.empty() && tabular_summary[tabular_summary.size() - 1] != '\n' )
				tabular_summary += '\n';
			tabular_summary += short_io_details_multiline;
			tabular_summary += '\n';
		}
		AddTabularText( tabular_summary.c_str() );
#else
		if( cmd_preview_count >= 1 )
		{
			std::ostringstream summary;
			summary << opcode_name;
			const std::string& details = !status_details_multiline.empty() ? status_details_multiline :
				( !configuration_details_multiline.empty() ? configuration_details_multiline :
					( !short_io_details_multiline.empty() ? short_io_details_multiline : virtual_wire_details_multiline ) );
			if( !details.empty() )
			{
				if( transaction_details.has_configuration )
				{
					summary << " - " << details;
				}
				else if( transaction_details.has_status )
				{
					const std::string pending = FormatPendingService( transaction_details.status );
					if( !pending.empty() )
						summary << " - " << pending << " pending";
					if( ( rsp_byte0 & 0x0f ) != 0x08 )
						summary << " - " << response_name;
					summary << '\n' << details;
				}
				else if( transaction_details.has_short_io )
				{
					if( rsp_preview_count == 0 || ( rsp_byte0 & 0x0f ) != 0x08 )
						summary << " - " << response_name;
					summary << '\n' << details;
				}
				else
				{
					summary << '\n' << details;
				}
			}
			summary << '\n';
			AddTabularText( summary.str().c_str() );
		}
		else
		{
			AddTabularText( "eSPI transaction\n" );
		}
#endif
#endif
}

void EspiAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void EspiAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}
