#ifndef ESPI_SIMULATION_DATA_GENERATOR
#define ESPI_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class EspiAnalyzerSettings;

class EspiSimulationDataGenerator
{
public:
	EspiSimulationDataGenerator();
	~EspiSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, EspiAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	EspiAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
};
#endif //ESPI_SIMULATION_DATA_GENERATOR
