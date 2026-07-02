#include "EspiCommand.h"
#include <cassert>

int main()
{
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
