#include "EspiCommand.h"
#include <cassert>
#include <cstring>

int main()
{
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x00 ), "PUT_PC" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x01 ), "GET_PC" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x02 ), "PUT_NP" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x03 ), "GET_NP" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x04 ), "PUT_VWIRE" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x05 ), "GET_VWIRE" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x06 ), "PUT_OOB" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x07 ), "GET_OOB" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x08 ), "PUT_FLASH_C" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x09 ), "GET_FLASH_NP" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x0a ), "PUT_FLASH_NP" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x0b ), "GET_FLASH_C" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x48 ), "PUT_MEMRD32_SHORT" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0x4c ), "PUT_MEMWR32_SHORT" ) == 0 );
    assert( std::strcmp( EspiCommand::GetOpcodeName( 0xfe ), "UNKNOWN_CMD" ) == 0 );

    assert( EspiCommand::IsShortIoOpcode( 0x40 ) );
    assert( EspiCommand::IsShortIoOpcode( 0x41 ) );
    assert( EspiCommand::IsShortIoOpcode( 0x43 ) );
    assert( EspiCommand::IsShortIoOpcode( 0x44 ) );
    assert( EspiCommand::IsShortIoOpcode( 0x45 ) );
    assert( EspiCommand::IsShortIoOpcode( 0x47 ) );
    assert( !EspiCommand::IsShortIoOpcode( 0x42 ) );
    assert( !EspiCommand::IsShortIoOpcode( 0x46 ) );
    assert( !EspiCommand::IsShortIoOpcode( 0x48 ) );

    assert( EspiCommand::GetShortAccessByteCount( 0x40 ) == 1 );
    assert( EspiCommand::GetShortAccessByteCount( 0x41 ) == 2 );
    assert( EspiCommand::GetShortAccessByteCount( 0x43 ) == 4 );
    assert( EspiCommand::GetShortAccessByteCount( 0x47 ) == 4 );
    assert( EspiCommand::GetShortAccessByteCount( 0x42 ) == 0 );

    assert( !EspiCommand::IsShortIoWrite( 0x40 ) );
    assert( EspiCommand::IsShortIoWrite( 0x44 ) );

    assert( EspiCommand::GetExpectedByteCount( 0x40 ) == 4 );
    assert( EspiCommand::GetExpectedByteCount( 0x41 ) == 4 );
    assert( EspiCommand::GetExpectedByteCount( 0x43 ) == 4 );
    assert( EspiCommand::GetExpectedByteCount( 0x44 ) == 5 );
    assert( EspiCommand::GetExpectedByteCount( 0x45 ) == 6 );
    assert( EspiCommand::GetExpectedByteCount( 0x47 ) == 8 );

    assert( EspiCommand::GetExpectedByteCount( 0x48 ) == 6 );
    assert( EspiCommand::GetExpectedByteCount( 0x4c ) == 7 );
    assert( EspiCommand::GetExpectedByteCount( 0x4f ) == 10 );
    return 0;
}
