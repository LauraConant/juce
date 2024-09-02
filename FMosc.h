#pragma once

#include <JuceHeader.h>

class FMOscillator
{
public:
    FMOscillator()
    {
        // Initialize the oscillators with sine waves
        carrier.initialise([](float x) { return std::sin(x); });
        modulator.initialise([](float x) { return std::sin(x); });
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        carrier.prepare(spec);
        modulator.prepare(spec);
    }

    void setCarrierFrequency(float frequency)
    {
        carrierFrequency = frequency;
        carrier.setFrequency(carrierFrequency);
    }

    void setModulatorFrequency(float frequency)
    {
        modulator.setFrequency(frequency);
    }

    void setModulationIndex(float index)
    {
        modulationIndex = index;
    }
    
    void setModulationDepth(float depth)
    {
        modulationDepth = depth;
    }

    void processBlock(juce::dsp::AudioBlock<float>& block)
    {
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            float modulatedFreq = carrierFrequency + modulationDepth * modulationIndex * modulator.processSample(0.0f);
            carrier.setFrequency(modulatedFreq);

            for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
            {
                block.setSample(channel, i, carrier.processSample(0.0f));
            }
        }
    }

private:
    juce::dsp::Oscillator<float> carrier { [](float x) { return std::sin(x); } };
    juce::dsp::Oscillator<float> modulator { [](float x) { return std::sin(x); } };

    float carrierFrequency = 440.0f;
    float modulationIndex = 100.0f;
    float modulationDepth = 1.0f;  // This represents the depth of the modulation
};
