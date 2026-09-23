#ifndef ESPI_ALERT_H
#define ESPI_ALERT_H

// Dedicated ALERT# is independent of CS#. Preserve an open pulse between
// calls, and consume all edges rather than only checking the final level.
template <class ChannelData, class Sample, class Emit>
void EspiAdvanceAlert(ChannelData *alert, Sample end, bool &asserted,
                      Sample &start, Emit emit) {
  if (!asserted && !alert->GetBitState()) {
    asserted = true;
    start = alert->GetSampleNumber();
  }
  while (alert->WouldAdvancingToAbsPositionCauseTransition(end)) {
    alert->AdvanceToNextEdge();
    if (!alert->GetBitState()) {
      start = alert->GetSampleNumber();
      asserted = true;
    } else {
      if (asserted)
        emit(start, alert->GetSampleNumber());
      asserted = false;
    }
  }
  alert->AdvanceToAbsPosition(end);
}

#endif
