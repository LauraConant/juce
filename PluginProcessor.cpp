/*==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TekhneAudioProcessor::TekhneAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    treeState.addParameterListener("frequency", this);
    treeState.addParameterListener("modFreq", this);
    treeState.addParameterListener("fmDepth", this);
    treeState.addParameterListener("modFreq2", this);
    treeState.addParameterListener("fmDepth2", this);
}

TekhneAudioProcessor::~TekhneAudioProcessor()
{
    treeState.removeParameterListener("frequency", this);
    treeState.removeParameterListener("modFreq", this);
    treeState.removeParameterListener("fmDepth", this);
    treeState.removeParameterListener("modFreq2", this);
    treeState.removeParameterListener("fmDepth2", this);
    
}

void TekhneAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "frequency")
    {
//        osc.setFrequency(newValue);
    }
//    if (parameterID == "carrierFreq")
//    {
//        carrierOscillator.setFrequency(newValue);
//    }
    if (parameterID == "modFreq")
    {
//        modulatorOscillator.setFrequency(newValue);
    }
    if (parameterID == "fmDepth")
    {
        fmDepth = newValue;
    }
    if (parameterID == "modFreq2")
    {
//        modulatorOscillator2.setFrequency(newValue);
    }
    if (parameterID == "fmDepth2")
    {
        fmDepth2 = newValue;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TekhneAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params;

    auto freq = std::make_unique<juce::AudioParameterInt>((juce::ParameterID{"frequency", 1 }), "FREQUENCY", 5.0, 2000.0, 440.0);
    
    params.push_back(std::move(freq));
    
    auto modFreq = std::make_unique<juce::AudioParameterFloat>((juce::ParameterID{"modFreq", 1 }), "MODFREQ", 5.0f, 2000.0f, 500.0f);
        
    params.push_back(std::move(modFreq));
        
    auto fmDepth = std::make_unique<juce::AudioParameterFloat>((juce::ParameterID{"fmDepth", 1 }), "FMDEPTH", 1.0f, 1500.0f, 500.0f);
        
    params.push_back(std::move(fmDepth));
    
    auto modFreq2 = std::make_unique<juce::AudioParameterFloat>((juce::ParameterID{"modFreq2", 1 }), "MODFREQ2", 5.0f, 2000.0f, 500.0f);

    params.push_back(std::move(modFreq2));
        
    auto fmDepth2 = std::make_unique<juce::AudioParameterFloat>((juce::ParameterID{"fmDepth2", 1 }), "FMDEPTH2", 1.0f, 1500.0f, 500.0f);

    params.push_back(std::move(fmDepth2));
    
    return { params.begin(), params.end() };
}

float TekhneAudioProcessor::calculateFunctionFmDepth(float x)
    {
        if (x <= 500.0f)
            return 0.0f;
        else
        {
            const float C = 14.f; // Calculated scaling factor
            const float B = 0.005f; // Chosen growth rate
            return static_cast<int>(C * (std::exp(B * (x - 500.0f)) - 1));
        }
    }

void TekhneAudioProcessor::setModulatorParameters(float newDistance, int modulationIndexID, int waveLife)
{
    
    float frequencyValue = juce::jmap(static_cast<float>(waveLife), 2.0f, 9.0f, 2000.0f, 5.0f);
    
    distance_center = newDistance;
    
    float maxScaledDistance = 1000.0f;
    float scaling_ratio = maxScaledDistance / 350;
    scaled_distance = distance_center * scaling_ratio;
    
    modulationTarget = 1000.f;
    
    const float maxRampTime = 10.0f;
    rampTime = maxRampTime * (scaled_distance / maxScaledDistance);
    
    double rampSamples = getSampleRate() * rampTime;

    modulationIncrement = modulationTarget / rampSamples;
    
    switch (modulationIndexID) {
            case 1:
                freq_modulator = frequencyValue;
                modulationIncrement1 = modulationIncrement;
                rampTime1 = rampTime;
                modulationCompleted = false;
                break;
            case 2:
                freq_modulator2 = frequencyValue;
                modulationIncrement2 = modulationIncrement;
                rampTime2 = rampTime;
                modulationCompleted2 = false;
                break;
            case 3:
                freq_modulator3 = frequencyValue;
                modulationIncrement3 = modulationIncrement;
                rampTime3 = rampTime;
                modulationCompleted3 = false;
                break;
            case 4:
                freq_modulator4 = frequencyValue;
                modulationIncrement3 = modulationIncrement;
                rampTime4 = rampTime;
                modulationCompleted4 = false;
                break;
            default:
                DBG("Invalid modulationIndexID: " << modulationIndexID);
                break;
        }
}

//==============================================================================
const juce::String TekhneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TekhneAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TekhneAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TekhneAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TekhneAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TekhneAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TekhneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TekhneAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TekhneAudioProcessor::getProgramName (int index)
{
    return {};
}

void TekhneAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TekhneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();
    
//    updateAngleDelta();
    freq_carrier = *treeState.getRawParameterValue("frequency");
    
    gain.prepare(spec);
    gain.setGainLinear(0.01f);
    
//    modulationIndex = distance_center;

//    lastFreq = *treeState.getRawParameterValue("frequency");
//    osc.setFrequency(*treeState.getRawParameterValue("frequency") + fmDepth);
//
//    modulatorOscillator.setFrequency(*treeState.getRawParameterValue("modFreq")
//                                     );
}

void TekhneAudioProcessor::releaseResources()
{
    // Free up any resources when playback stops
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TekhneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void TekhneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
    
    // ScopedNoDenormals noDenormals;
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();
    
        freq_carrier = *treeState.getRawParameterValue("frequency");
        auto cyclesPerSample = freq_carrier / getSampleRate();
        incr_carrier = cyclesPerSample * juce::MathConstants<double>::pi;
    
        auto cyclesPerSample_mod = freq_modulator / getSampleRate();
        incr_modulator = cyclesPerSample_mod * juce::MathConstants<double>::pi;

        auto cyclesPerSample_mod2 = freq_modulator2 / getSampleRate();
        incr_modulator2 = cyclesPerSample_mod2 * juce::MathConstants<double>::pi;
    
        auto cyclesPerSample_mod3 = freq_modulator3 / getSampleRate();
        incr_modulator3 = cyclesPerSample_mod3 * juce::MathConstants<double>::pi;
    
        auto cyclesPerSample_mod4 = freq_modulator4 / getSampleRate();
        incr_modulator4 = cyclesPerSample_mod4 * juce::MathConstants<double>::pi;

        
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        {
           buffer.clear(i, 0, buffer.getNumSamples());
        }
            
            int numChannels = buffer.getNumChannels();
    
            DBG("Completed: " + juce::String(modulationCompleted ? "true" : "false"));
            DBG("Completed2: " + juce::String(modulationCompleted2 ? "true" : "false"));
         
            for (auto sample = 0; sample < buffer.getNumSamples(); sample++)
            {
               
                if (!modulationCompleted) {
                    if (increasing1) {
                        modulationIndex += modulationIncrement;

                        if (modulationIndex >= modulationTarget) {
                            modulationIndex = modulationTarget;
                            increasing1 = false;
                        }
                    } else {
                        modulationIndex -= modulationIncrement;

                        if (modulationIndex <= modulationStart) {
                            modulationIndex = modulationStart;
                            increasing1 = true;
                            modulationCompleted = true;
                        }
                    }
                }
                
                if (!modulationCompleted2) {
                    if (increasing2) {
                        modulationIndex2 += modulationIncrement2;
                        if (modulationIndex2 >= modulationTarget) {
                            modulationIndex2 = modulationTarget;
                            increasing2 = false;
                        }
                    } else {
                        modulationIndex2 -= modulationIncrement2;

                        if (modulationIndex2 <= modulationStart) {
                            modulationIndex2 = modulationStart;
                            increasing2 = true;
                            modulationCompleted2 = true;
                        }
                    }
                }
                
                if (!modulationCompleted3) {
                    if (increasing3) {
                        modulationIndex3 += modulationIncrement3;

                        if (modulationIndex3 >= modulationTarget) {
                            modulationIndex3 = modulationTarget;
                            increasing3 = false;
                        }
                    } else {
                        modulationIndex3 -= modulationIncrement3;

                        if (modulationIndex3 <= modulationStart)
                        {
                            modulationIndex3 = modulationStart;
                            increasing3 = true;
                            modulationCompleted3 = true;
                        }
                    }
                }
                
                if (!modulationCompleted4) {
                    if (increasing4) {
                        modulationIndex4 += modulationIncrement4;

                        if (modulationIndex4 >= modulationTarget)
                        {
                            modulationIndex4 = modulationTarget;
                            increasing4 = false;
                        }
                    } else {
                        modulationIndex4 -= modulationIncrement2;

                        if (modulationIndex4 <= modulationStart)
                        {
                            modulationIndex4 = modulationStart;
                            increasing4 = true;
                            modulationCompleted4 = true;
                        }
                    }
                }
                
                double modulatorSignal = std::sin(phase_modulator);
                phase_modulator += incr_modulator;
                
                if (phase_modulator >= two_pi)
                {
                    phase_modulator -= two_pi;
                }
                
                double modulatorSignal2 = std::sin(phase_modulator2);
                phase_modulator2 += incr_modulator2;
                
                if (phase_modulator2 >= two_pi)
                {
                    phase_modulator2 -= two_pi;
                }
                
                double modulatorSignal3 = std::sin(phase_modulator3);
                phase_modulator3 += incr_modulator3;
                
                if (phase_modulator3 >= two_pi)
                {
                    phase_modulator3 -= two_pi;
                }
                
                double modulatorSignal4 = std::sin(phase_modulator4);
                phase_modulator4 += incr_modulator4;
                
                if (phase_modulator4 >= two_pi)
                {
                    phase_modulator4 -= two_pi;
                }
                
                double modulatedFreq = freq_carrier +
                                       modulationIndex * modulatorSignal +
                                       modulationIndex2 * modulatorSignal2 +
                                       modulationIndex3 * modulatorSignal3 +
                                       modulationIndex4 * modulatorSignal4;
                
                auto cyclesPerSample_modulated = modulatedFreq / getSampleRate();
                double incr_carrier_modulated = cyclesPerSample_modulated * juce::MathConstants<double>::pi * 2.0;
                
                phase_carrier += incr_carrier_modulated;
                
                if (phase_carrier >= two_pi)
                {
                    phase_carrier -= two_pi;
                }
                
                float output = (float) (std::sin(phase_carrier)) * 0.5f;

                
                   for (int channel = 0; channel < numChannels; ++channel)
                   {
                       // Get write pointer for the current channel
                       auto* channelData = buffer.getWritePointer(channel);
                       // Assign the signal value to the current sample
                       channelData[sample] = output;
                   }
            }
        
    }
    
//    juce::ScopedNoDenormals noDenormals;
//    auto totalNumInputChannels = getTotalNumInputChannels();
//    auto totalNumOutputChannels = getTotalNumOutputChannels();
//
//    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
//    {
//        buffer.clear(i, 0, buffer.getNumSamples());
//    }
//
//    juce::dsp::AudioBlock<float> audioBlock { buffer };
//
//
//    for (int ch = 0; ch < audioBlock.getNumChannels(); ++ch)
//    {
//        for (int s = 0; s < audioBlock.getNumSamples(); ++s)
//        {
//
//            freq_carrier = *treeState.getRawParameterValue("modFreq");;
//            incr_carrier = freq_carrier * 0.00013f;//two_pi * freq_carrier / getSampleRate();
//
//            phase_carrier += incr_carrier;
//
//            if(phase_carrier >= two_pi)
//            {
//                phase_carrier -= two_pi;
//            }
//
//            out_carrier = std::sin(phase_carrier);
//
//            buffer.setSample(ch, s, out_carrier);
            
//            float fmMod = modulatorOscillator.processSample(audioBlock.getSample(ch, s)) * fmDepth;
//
//            float modFreq = *treeState.getRawParameterValue("modFreq");
//            modulatorOscillator.setFrequency(modFreq);  // Set the modulator oscillator's frequency
//
//            float modulatedFrequency = *treeState.getRawParameterValue("frequency") + fmMod;
//            modulatedFrequency = std::abs(modulatedFrequency);
//
//
//            float quantizedModulatedFreq = quantizeFrequency(modulatedFrequency);
//            osc.setFrequency(modulatedFrequency);
        

//    }

//    modulatorOscillator.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
//    osc.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    //gain.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    
//}


//==============================================================================
bool TekhneAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TekhneAudioProcessor::createEditor()
{
    return new TekhneAudioProcessorEditor (*this);
}

//==============================================================================
void TekhneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TekhneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TekhneAudioProcessor();
}
