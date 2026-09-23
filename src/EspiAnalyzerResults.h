#ifndef ESPI_ANALYZER_RESULTS
#define ESPI_ANALYZER_RESULTS

#include <AnalyzerResults.h>
#include <mutex>
#include <vector>

class EspiAnalyzer;
class EspiAnalyzerSettings;

class EspiAnalyzerResults : public AnalyzerResults {
public:
  struct VirtualWireGroup {
    U8 index;
    U8 data;
  };

  struct TransactionDetails {
    U32 virtual_wire_group_count = 0;
    std::vector<VirtualWireGroup> virtual_wire_groups;
    bool has_configuration = false;
    bool configuration_is_write = false;
    U16 configuration_address = 0;
    U32 configuration_value = 0;
    bool has_status = false;
    U16 status = 0;
    U8 response_modifier = 0;
    bool has_short_io = false;
    bool short_io_is_write = false;
    bool short_io_has_data = false;
    bool short_io_has_status = false;
    U8 short_io_size = 0;
    U16 short_io_address = 0;
    U32 short_io_data = 0;
    U16 short_io_status = 0;
  };

  EspiAnalyzerResults(EspiAnalyzer *analyzer, EspiAnalyzerSettings *settings);
  virtual ~EspiAnalyzerResults();

  enum EspiFrameType {
    TransactionFrame = 1,
    AlertFrame = 2,
    InvalidChipSelectFrame = 3
  };

  virtual void GenerateBubbleText(U64 frame_index, Channel &channel,
                                  DisplayBase display_base);
  virtual void GenerateExportFile(const char *file, DisplayBase display_base,
                                  U32 export_type_user_id);

  virtual void GenerateFrameTabularText(U64 frame_index,
                                        DisplayBase display_base);
  virtual void GeneratePacketTabularText(U64 packet_id,
                                         DisplayBase display_base);
  virtual void GenerateTransactionTabularText(U64 transaction_id,
                                              DisplayBase display_base);

  void AddTransactionDetails(const TransactionDetails &details);
  bool GetTransactionDetails(U64 frame_index,
                             TransactionDetails &details) const;

protected: // functions
protected: // vars
  EspiAnalyzerSettings *mSettings;
  EspiAnalyzer *mAnalyzer;
  mutable std::mutex mTransactionDetailsMutex;
  std::vector<TransactionDetails> mTransactionDetails;
};

#endif // ESPI_ANALYZER_RESULTS
