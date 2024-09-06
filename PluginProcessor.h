/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class TekhneAudioProcessor  : public juce::AudioProcessor,
                              public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    TekhneAudioProcessor();
    ~TekhneAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
//    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    juce::AudioProcessorValueTreeState treeState;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    void setModulatorParameters(float newDistance, int modulationIndexID, int waveLife);
    float calculateFunctionFmDepth(float x);
    
    void setModulatorFrequency(float freq);
    
private:
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    float distance_center = 0;
    float scaled_distance = 0;
    int ID;
    
    bool rampComplete = false;
    
    bool modulationCompleted = false;
    bool modulationCompleted2 = false;
    bool modulationCompleted3 = false;
    bool modulationCompleted4 = false;
    
    float modulationIndex = 0.0f; // Target modulation value
    float modulationIndex2 = 0.0f; // Target modulation value
    float modulationIndex3 = 0.0f; // Target modulation value
    float modulationIndex4 = 0.0f; // Target modulation value

    float modulationStart = 0.0f;
    float modulationTarget = 0.0f;
    float modulationIncrement = 0.0f;
    float modulationIncrement1 = 0.0f;
    float modulationIncrement2 = 0.0f;
    float modulationIncrement3 = 0.0f;
    float modulationIncrement4 = 0.0f;
    float rampTime = 0.f;
    float rampTime1 = 0.f;
    float rampTime2 = 0.f;
    float rampTime3 = 0.f;
    float rampTime4 = 0.f;
    bool increasing1 = true;
    bool increasing2 = true;
    bool increasing3 = true;
    bool increasing4 = true;
    
    float out_carrier;
    float phase_carrier;
    float incr_carrier;
    float two_pi = 6.28318;
    float freq_carrier = *treeState.getRawParameterValue("modFreq");
    
    float out_modulator;
    float phase_modulator;
    float incr_modulator;
    float freq_modulator;
    
    float out_modulator2;
    float phase_modulator2;
    float incr_modulator2;
    float freq_modulator2;
    
    float out_modulator3;
    float phase_modulator3;
    float incr_modulator3;
    float freq_modulator3;
    
    float out_modulator4;
    float phase_modulator4;
    float incr_modulator4;
    float freq_modulator4;
    
    float fmIndex;

    juce::dsp::Oscillator<float> osc { [](float x) { return std::sin (x); }};
    juce::dsp::Oscillator<float> modulatorOscillator { [](float x) { return std::sin (x); }};
//    juce::dsp::Oscillator<float> modulatorOscillator2 { [](float x) { return std::sin (x); }};
    
    float fmMod { 0.0f };
    float fmDepth { 0.0f };
    float lastFreq { 0 };
    
    float fmMod2 { 0.0f };
    float fmDepth2 { 0.0f };
    float lastFreq2 { 0 };
    
//    float cyclesPerSecond;
//    float delta;
//    float angleDelta { 0.0f };
//    float phase { 0.0f };
    
    
    juce::dsp::Gain<float> gain;
    
    float quantizeFrequency(float frequency)
    {
            auto closest = std::min_element(lydianScaleFrequencies.begin(), lydianScaleFrequencies.end(),
                [frequency](float a, float b) {
                    return std::abs(a - frequency) < std::abs(b - frequency);
                });

            return *closest;
    }
    
    const std::array<float, 28> lydianScaleFrequencies = {
        // Octave 3
        130.81f, // C3 (Root)
        146.83f, // D3 (Major Second)
        164.81f, // E3 (Major Third)
        185.00f, // F#3 (Augmented Fourth)
        196.00f, // G3 (Perfect Fifth)
        220.00f, // A3 (Major Sixth)
        246.94f, // B3 (Major Seventh)

        // Octave 4
        261.63f, // C4 (Root)
        293.66f, // D4 (Major Second)
        329.63f, // E4 (Major Third)
        369.99f, // F#4 (Augmented Fourth)
        392.00f, // G4 (Perfect Fifth)
        440.00f, // A4 (Major Sixth)
        493.88f, // B4 (Major Seventh)

        // Octave 5
        523.25f, // C5 (Root)
        587.33f, // D5 (Major Second)
        659.26f, // E5 (Major Third)
        739.99f, // F#5 (Augmented Fourth)
        783.99f, // G5 (Perfect Fifth)
        880.00f, // A5 (Major Sixth)
        987.77f, // B5 (Major Seventh)

        // Octave 6
        1046.50f, // C6 (Root)
        1174.66f, // D6 (Major Second)
        1318.51f, // E6 (Major Third)
        1479.98f, // F#6 (Augmented Fourth)
        1567.98f, // G6 (Perfect Fifth)
        1760.00f, // A6 (Major Sixth)
        1975.53f  // B6 (Major Seventh)
    };


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TekhneAudioProcessor)
};
