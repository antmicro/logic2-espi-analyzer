#include "EspiSimulationDataGenerator.h"
#include "EspiAnalyzerSettings.h"

EspiSimulationDataGenerator::EspiSimulationDataGenerator()
:	mSettings( nullptr ),
	mSimulationSampleRateHz( 0 )
{
}

EspiSimulationDataGenerator::~EspiSimulationDataGenerator()
{
}

void EspiSimulationDataGenerator::Initialize( U32 simulation_sample_rate, EspiAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;
}

U32 EspiSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	static_cast<void>( largest_sample_requested );
	static_cast<void>( sample_rate );
	*simulation_channel = nullptr;
	return 0;
}
