#ifndef ESPI_COMMAND_H
#define ESPI_COMMAND_H

#include <cstdint>

namespace EspiCommand
{
    static constexpr std::uint32_t kPreviewByteCount = 4;

    inline bool IsShortIoOpcode( std::uint8_t opcode )
    {
        const std::uint8_t size_encoding = opcode & 0x03;
        return ( opcode & 0xf8 ) == 0x40 && size_encoding != 0x02;
    }

    inline bool IsShortIoWrite( std::uint8_t opcode )
    {
        return IsShortIoOpcode( opcode ) && ( opcode & 0x04 ) != 0;
    }

    inline std::uint32_t GetShortAccessByteCount( std::uint8_t opcode )
    {
        if( !IsShortIoOpcode( opcode ) )
            return 0;

        return ( opcode & 0x03 ) == 0x03 ? 4 : ( opcode & 0x03 ) + 1;
    }

    inline std::uint32_t GetPacketLength( std::uint8_t tag_length, std::uint8_t length_low )
    {
        const std::uint32_t length = ( std::uint32_t( tag_length & 0x0f ) << 8 ) | length_low;
        return length == 0 ? 4096 : length;
    }

    inline bool CycleHasData( std::uint8_t cycle_type )
    {
        return cycle_type == 0x01 || cycle_type == 0x03 || cycle_type == 0x05 ||
            ( cycle_type >= 0x08 && cycle_type <= 0x0b );
    }

    inline std::uint32_t GetPacketByteCount( std::uint8_t opcode, std::uint8_t cycle_type,
        std::uint8_t tag_length, std::uint8_t length_low )
    {
        std::uint32_t header_bytes;
        switch( cycle_type )
        {
        case 0x00: // Memory Read 32 / Flash Read
        case 0x01: // Memory Write 32 / Flash Write
            header_bytes = 7;
            break;
        case 0x02: // Memory Read 64 / Flash Erase
            header_bytes = opcode == 0x0a ? 7 : 11;
            break;
        case 0x03: // Memory Write 64
            header_bytes = 11;
            break;
        case 0x04: // I/O Read
        case 0x05: // I/O Write
            header_bytes = 5;
            break;
        default: // Completion packet
            header_bytes = 3;
            break;
        }

        return header_bytes + ( CycleHasData( cycle_type ) ? GetPacketLength( tag_length, length_low ) : 0 );
    }

    inline std::uint32_t GetExpectedByteCount( std::uint8_t opcode, std::uint8_t byte1 = 0,
        std::uint8_t byte2 = 0, std::uint8_t byte3 = 0 )
    {
        switch( opcode )
        {
        case 0x00: // PUT_PC
        case 0x02: // PUT_NP
        case 0x08: // PUT_FLASH_C
        case 0x0a: // PUT_FLASH_NP
            return 2 + GetPacketByteCount( opcode, byte1, byte2, byte3 ); // Opcode, packet, CRC
        case 0x01: // GET_PC
        case 0x03: // GET_NP
        case 0x05: // GET_VWIRE
        case 0x07: // GET_OOB
        case 0x09: // GET_FLASH_NP
        case 0x0b: // GET_FLASH_C
        case 0x25: // GET_STATUS
            return 2; // Opcode, CRC
        case 0x04: // PUT_VWIRE: opcode, count, 2 bytes/group, CRC
            return 5 + ( 2 * ( byte1 & 0x3f ) );
        case 0x06: // PUT_OOB: opcode, 3-byte header, data, CRC
            return 5 + GetPacketLength( byte2, byte3 );
        case 0x21: // GET_CONFIGURATION
            return 4;
        case 0x22: // SET_CONFIGURATION
            return 8;
        case 0xff: // RESET
            return 1;
        default:
            break;
        }

        if( IsShortIoOpcode( opcode ) )
        {
            const std::uint32_t data_bytes = IsShortIoWrite( opcode ) ? GetShortAccessByteCount( opcode ) : 0;
            return 4 + data_bytes; // Opcode, 16-bit address, optional data, CRC
        }

        if( opcode >= 0x48 && opcode <= 0x4f && ( opcode & 0x03 ) != 0x02 )
        {
            const bool is_write = ( opcode & 0x04 ) != 0;
            const std::uint32_t data_bytes = is_write ? ( ( opcode & 0x03 ) == 0x03 ? 4 : ( opcode & 0x03 ) + 1 ) : 0;
            return 6 + data_bytes; // Opcode, 32-bit address, optional data, CRC
        }

        return kPreviewByteCount;
    }
}

#endif // ESPI_COMMAND_H
