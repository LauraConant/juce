/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class TekhneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::Slider::Listener,
//                                   private juce::MidiInputCallback, // For handling incoming MIDI messages
//                                   private juce::MidiKeyboardStateListener, // For handling keyboard state changes
                                   private juce::Timer
{
public:
    TekhneAudioProcessorEditor (TekhneAudioProcessor&);
    ~TekhneAudioProcessorEditor() override;

    //==============================================================================
    void update();
    
    static juce::String getMidiMessageDescription (const juce::MidiMessage& m);

    void sliderValueChanged(juce::Slider* slider) override;
    
    void getIntersectionsX();
    
    void mouseDown(const juce::MouseEvent& event) override;
    
    void erasingCircles();
    bool doCirclesIntersect(int x1, int y1, float r1, int x2, int y2, float r2);
    
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void updateToggleButtonState();
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    TekhneAudioProcessor& audioProcessor;
    
    juce::Slider radiusSlider;
    juce::Slider growthSlider;
    
    juce::Slider waveDistance;
    juce::Label waveDistanceLabel;

    juce::Slider carrierFreq;
    juce::Label carrierFreqLabel;
    juce::Label carrierFreqValueLabel;


    juce::Slider modFreq;
    juce::Slider fmDepth;
    
    juce::Slider modFreq2;
    juce::Slider fmDepth2;
    
    float distance_center;
    int waveLife;
    
    std::set<int> availableIDs;  // Keeps track of available IDs for reuse
    int nextID = 1;
    
//    const std::array<float, 128> midiNoteFrequencies = []{
//        std::array<float, 128> frequencies = {};
//        for (int i = 0; i < 128; ++i)
//            frequencies[i] = 440.0f * std::pow(2.0f, (i - 69) / 12.0f); // A4 = MIDI note 69
//        return frequencies;
//    }();
    
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
    //-------------//

    void timerCallback() override
    {
        update();
    }
    
    
    //------//
    
    static int generateUniqueId()
        {
            static int idCounter = 0;
            return idCounter++;
        }
    
    struct Circle
        {
            int x;
            int y;
            int baseRadius;
            int growthRate;
            int waveDistance;
            int id;
        
            juce::Time creationTime;
            float opacity = 1;
        };
    
    std::vector<Circle> circles;
    
    
    struct Wave
        {
            int x;
            int y;
            int baseRadius;
            int growthRate;
            int circleID;
        
            juce::Time creationTime;
        
        Wave(const Circle& circle)
                : x(circle.x),
                  y(circle.y),
                  baseRadius(circle.baseRadius),
                  growthRate(circle.growthRate),
                  circleID(circle.id),
                  creationTime(juce::Time::getCurrentTime())
            {}
        };
    
    std::vector<Wave> waves;
    
    //------//
    
   struct IntersectionPair
      {
          float x1;
          float y1;
          float x2;
          float y2;
       
          bool operator==(const IntersectionPair& other) const
              {
                  return (x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2);
              }
      };
    
    std::vector<IntersectionPair> intersectionPairs;

    float roundToDecimalPlaces(float value, int decimalPlaces) {
        float factor = std::pow(10.0f, decimalPlaces);
        return std::round(value * factor) / factor;
    }

   float calculateRadius(const Wave& wave) const
       {
           float elapsedTime = (juce::Time::getCurrentTime() - wave.creationTime).inSeconds();
           return wave.baseRadius + (elapsedTime * wave.growthRate);
       }
    
    
    const float fadeOutDuration = 20.0f;  // Duration in seconds (lifespan of the circle)
    const int timerIntervalMs = 60;  // Timer interval in milliseconds as set by startTimer(60)
    const int framesPerSecond = 1000 / timerIntervalMs;  // Calculate frames per second
    const int totalFrames = static_cast<int>(fadeOutDuration * framesPerSecond);  // Total number of frames for fade-out
    const float decrementRate = 1.0f / totalFrames;  // Opacity decrement per frame (callback)Rate = 1.0f / totalFrames;  // Opacity decrement per frame
    
    float calculateOpacity(Circle& circle, float decrementRate)
    {
        
        circle.opacity -= decrementRate; // Modify the circle's opacity directly
        
           if (circle.opacity <= 0.0f)  // Ensure opacity does not go below 0
           {
               circle.opacity = 0.0f;
           }

           return circle.opacity;
    }

    
   std::tuple<float, float, float, float> calculateIntersections(const Wave& w1, float r1, const Wave& w2, float r2) const
       {
           float distance = std::sqrt(std::pow(w2.x - w1.x, 2) + std::pow(w2.y - w1.y, 2));
           float a = (r1 * r1 - r2 * r2 + distance * distance) / (2 * distance);
           float h = std::sqrt(r1 * r1 - a * a);

           float dx = (w2.x - w1.x) / distance;
           float dy = (w2.y - w1.y) / distance;

           float ix1 = w1.x + a * dx + h * dy;
           float iy1 = w1.y + a * dy - h * dx;
           float ix2 = w1.x + a * dx - h * dy;
           float iy2 = w1.y + a * dy + h * dx;

           return { ix1, iy1, ix2, iy2 };
       }


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TekhneAudioProcessorEditor)
};
