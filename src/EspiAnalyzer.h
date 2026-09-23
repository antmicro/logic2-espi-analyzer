#ifndef ESPI_ANALYZER_H
#define ESPI_ANALYZER_H

#include "EspiAnalyzerResults.h"
#include "EspiAnalyzerSettings.h"
#include "EspiSimulationDataGenerator.h"
#include <Analyzer.h>
#include <memory>

class ANALYZER_EXPORT EspiAnalyzer : public Analyzer2 {
public:
  EspiAnalyzer();
  virtual ~EspiAnalyzer();

  virtual void SetupResults();
  virtual void WorkerThread();

  virtual U32
  GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate,
                         SimulationChannelDescriptor **simulation_channels);
  virtual U32 GetMinimumSampleRateHz();

  virtual const char *GetAnalyzerName() const;
  virtual bool NeedsRerun();

protected: // vars
  EspiAnalyzerSettings mSettings;
  std::unique_ptr<EspiAnalyzerResults> mResults;
  AnalyzerChannelData *mClock;
  AnalyzerChannelData *mChipSelect;
  AnalyzerChannelData *mReset;
  AnalyzerChannelData *mIo0;
  AnalyzerChannelData *mIo1;
  AnalyzerChannelData *mIo2;
  AnalyzerChannelData *mIo3;

  EspiSimulationDataGenerator mSimulationDataGenerator;
  bool mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif // ESPI_ANALYZER_H
