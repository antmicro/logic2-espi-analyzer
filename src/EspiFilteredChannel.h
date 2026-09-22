#ifndef ESPI_FILTERED_CHANNEL_H
#define ESPI_FILTERED_CHANNEL_H

#include <cstdint>
#include <limits>

// Keep a logical cursor behind the raw cursor so lookahead never moves CLK or
// data past an accepted CS edge. Accepted edges retain their original timestamp.
template<class ChannelData>
class EspiFilteredChannel
{
public:
    using Sample = std::uint64_t;
    using State = decltype( static_cast<ChannelData*>( nullptr )->GetBitState() );

    EspiFilteredChannel( ChannelData* raw, Sample minimum_width ) :
        mRaw( raw ), mMinimumWidth( minimum_width ),
        mSample( raw->GetSampleNumber() ), mState( raw->GetBitState() ) {}

    Sample GetSampleNumber() const { return mSample; }
    State GetBitState() const { return mState; }

    Sample GetSampleOfNextEdge()
    {
        if( mHaveNext )
            return mNext;
        for( ;; )
        {
            const Sample candidate = mRaw->GetSampleOfNextEdge();
            mRaw->AdvanceToNextEdge();
            // A pulse exactly as wide as the threshold is retained. Only
            // inspect through threshold - 1, including across SDK data blocks.
            const Sample lookahead = mMinimumWidth > 0 ? mMinimumWidth - 1 : 0;
            const Sample end = candidate > std::numeric_limits<Sample>::max() - lookahead ?
                std::numeric_limits<Sample>::max() : candidate + lookahead;
            if( lookahead != 0 && mRaw->WouldAdvancingToAbsPositionCauseTransition( end ) )
            {
                mRaw->AdvanceToNextEdge(); // swallow the pulse and its return edge
                continue;
            }
            mNext = candidate;
            mHaveNext = true;
            return mNext;
        }
    }

    void AdvanceToNextEdge()
    {
        mSample = GetSampleOfNextEdge();
        mState = mRaw->GetBitState();
        mHaveNext = false;
    }

    void AdvanceToAbsPosition( Sample sample )
    {
        if( sample <= mSample )
            return;
        while( mSample < sample && GetSampleOfNextEdge() <= sample )
            AdvanceToNextEdge();
        mSample = sample;
    }

private:
    ChannelData* mRaw;
    Sample mMinimumWidth;
    Sample mSample;
    State mState;
    Sample mNext = 0;
    bool mHaveNext = false;
};

#endif
