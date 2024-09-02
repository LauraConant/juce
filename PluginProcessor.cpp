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
        osc.setFrequency(newValue);
    }
//    if (parameterID == "carrierFreq")
//    {
//        carrierOscillator.setFrequency(newValue);
//    }
    if (parameterID == "modFreq")
    {
        modulatorOscillator.setFrequency(newValue);
    }
    if (parameterID == "fmDepth")
    {
        fmDepth = newValue;
    }
    if (parameterID == "modFreq2")
    {
        modulatorOscillator2.setFrequency(newValue);
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
    
    auto modFreq2 = std::make_unique<juce::AudioParameterFloat>((juce::ParameterID{"modFreq2", 1 }), "MODFREQ2", 0.0f, 1000.0f, 5.0f);

    params.push_back(std::move(modFreq2));
        
    auto fmDepth2 = std::make_unique<juce::AudioParameterFloat>((juce::ParameterID{"fmDepth2", 1 }), "FMDEPTH2", 1.0f, 1500.0f, 500.0f);

    params.push_back(std::move(fmDepth2));
    
    return { params.begin(), params.end() };
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
    
    osc.prepare(spec);
    modulatorOscillator.prepare(spec);
    gain.prepare(spec);
    gain.setGainLinear(0.01f);

    lastFreq = *treeState.getRawParameterValue("frequency");
    osc.setFrequency(*treeState.getRawParameterValue("frequency") + fmDepth);
    
    lastFreq2 = *treeState.getRawParameterValue("modFreq");
    modulatorOscillator.setFrequency(*treeState.getRawParameterValue("modFreq")
//                                     + fmDepth2
                                     );
    
    modulatorOscillator2.setFrequency(*treeState.getRawParameterValue("modFreq2"));
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
//    cyclesPerSecond = *treeState.getRawParameterValue("modFreq");
//    double TWO_PI = 2 * M_PI;
//
//    auto cyclesPerSample = cyclesPerSecond / getSampleRate();
//
//    angleDelta = cyclesPerSample * TWO_PI;
//    delta = cyclesPerSample * 2.0;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    juce::dsp::AudioBlock<float> audioBlock { buffer };

    for (int ch = 0; ch < audioBlock.getNumChannels(); ++ch)
    {
        for (int s = 0; s < audioBlock.getNumSamples(); ++s)
        {

            float fmMod = modulatorOscillator.processSample(audioBlock.getSample(ch, s)) * fmDepth;
            
            // Apply modulation to the oscillator frequency
            
            float modulatedFrequency = *treeState.getRawParameterValue("frequency") + fmMod;
            modulatedFrequency = std::abs(modulatedFrequency);
            
//            float modulatedFrequency2 = lastFreq2 + fmMod2;
//            while (modulatedFrequency2 < 0.0f)
//                modulatedFrequency2 += baseFrequency;
//
//            modulatorOscillator.setFrequency(modulatedFrequency2);
            
            float quantizedModulatedFreq = quantizeFrequency(modulatedFrequency);
            osc.setFrequency(quantizedModulatedFreq);
        }
    }

    osc.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    gain.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    
}


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
