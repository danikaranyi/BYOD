#pragma once

#include "SampleLSTM.h"

template <int hiddenSize, typename ResamplerType = chowdsp::ResamplingTypes::LanczosResampler<>>
class ResampledRNN
{
public:
    ResampledRNN() = default;
    ResampledRNN (ResampledRNN&&) noexcept = default;
    ResampledRNN& operator= (ResampledRNN&&) noexcept = default;

    void initialise (const void* modelData, int modelDataSize, double modelSampleRate);

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    template <bool useRedisuals = false>
    void process (juce::dsp::AudioBlock<float>& block)
    {
        auto&& processBlock = dsp::AudioBlock<float> { block };
        if (needsResampling)
            processBlock = resampler.processIn (block);

        const auto numSamples = (int) processBlock.getNumSamples();
        auto* x = processBlock.getChannelPointer (0);

        if constexpr (useRedisuals)
        {
            for (int i = 0; i < numSamples; ++i)
                x[i] += model.forward (&x[i]);
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
                x[i] = model.forward (&x[i]);
        }

        if (needsResampling)
            resampler.processOut (processBlock, block);
    }

private:
    using RecurrentLayerType = SampleLSTM<float, 1, hiddenSize>;
    using DenseLayerType = RTNeural::DenseT<float, hiddenSize, 1>;
    RTNeural::ModelT<float, 1, 1, RecurrentLayerType, DenseLayerType> model;

    chowdsp::ResampledProcess<ResamplerType> resampler;

    double targetSampleRate = 48000.0;
    bool needsResampling = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResampledRNN)
};