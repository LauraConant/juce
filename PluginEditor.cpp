/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TekhneAudioProcessorEditor::TekhneAudioProcessorEditor (TekhneAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{

    radiusSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    radiusSlider.setRange(10.0, 100.0, 1.0);
    radiusSlider.setValue(55.0);
    radiusSlider.addListener(this);

    radiusSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    radiusSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
    
    addAndMakeVisible(radiusSlider);
    
    growthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    growthSlider.setRange(1.0, 20.0, 1.0);
    growthSlider.setValue(4.0);
    growthSlider.addListener(this);

    growthSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    growthSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
    
    addAndMakeVisible(growthSlider);
    
//    carrierFreq.setSliderStyle(juce::Slider::LinearHorizontal);
//    carrierFreq.setRange(200.0, 2000.0, 1.0);
//    carrierFreq.setValue(440.0);
//    carrierFreq.addListener(this);
//
    modFreq.setSliderStyle(juce::Slider::LinearHorizontal);
    modFreq.setRange(5.0, 2000.0, 1.0);
    modFreq.setValue(880.0);
    modFreq.addListener(this);

    fmDepth.setSliderStyle(juce::Slider::LinearHorizontal);
    fmDepth.setRange(1.0, 1500.0, 1.0);
    fmDepth.setValue(500.0);
    fmDepth.addListener(this);

    addAndMakeVisible(modFreq);
    addAndMakeVisible(fmDepth);
    
    modFreq2.setSliderStyle(juce::Slider::LinearHorizontal);
    modFreq2.setRange(5.0, 1000.0, 1.0);
    modFreq2.setValue(5.0);
    modFreq2.addListener(this);

    fmDepth2.setSliderStyle(juce::Slider::LinearHorizontal);
    fmDepth2.setRange(1.0, 1500.0, 1.0);
    fmDepth2.setValue(500.0);
    fmDepth2.addListener(this);
//
//    addAndMakeVisible(modFreq2);
//    addAndMakeVisible(fmDepth2);
    
    
    setSize (1200, 800);
    startTimer(60);
}

TekhneAudioProcessorEditor::~TekhneAudioProcessorEditor()
{
    stopTimer();
}

void TekhneAudioProcessorEditor::getIntersectionsX()
{
    if (!intersectionPairs.empty())
       {
           auto latestIntersectionX = intersectionPairs.back().x1;
           auto width = getWidth();

           // Map the X position to a frequency range (100 to 800)
           juce::NormalisableRange<float> frequencyRange(5.0f, 2000.0f);
           float frequencyValue = frequencyRange.convertFrom0to1(latestIntersectionX / static_cast<float>(width));
           
           float quantizedFrequency = quantizeFrequency(frequencyValue);

//           DBG(static_cast<float>(frequencyValue));

           // Update the frequency parameter in the AudioProcessor
           audioProcessor.treeState.getParameter("frequency")->setValueNotifyingHost(frequencyRange.convertTo0to1(static_cast<float>(quantizedFrequency)));
       }
}


void TekhneAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &radiusSlider)
    {
        update();
    }
    else if (slider == &modFreq)
       {
           float value = modFreq.getValue();
           audioProcessor.treeState.getParameter("modFreq")->setValueNotifyingHost(audioProcessor.treeState.getParameter("modFreq")->getNormalisableRange().convertTo0to1(value));
       }
    else if (slider == &fmDepth)
       {
           float value = fmDepth.getValue();
           audioProcessor.treeState.getParameter("fmDepth")->setValueNotifyingHost(audioProcessor.treeState.getParameter("fmDepth")->getNormalisableRange().convertTo0to1(value));
       }
    else if (slider == &modFreq2)
       {
           float value = modFreq.getValue();
           audioProcessor.treeState.getParameter("modFreq2")->setValueNotifyingHost(audioProcessor.treeState.getParameter("modFreq2")->getNormalisableRange().convertTo0to1(value));
       }
    else if (slider == &fmDepth2)
       {
           float value = fmDepth.getValue();
           audioProcessor.treeState.getParameter("fmDepth2")->setValueNotifyingHost(audioProcessor.treeState.getParameter("fmDepth2")->getNormalisableRange().convertTo0to1(value));
       }
}

void TekhneAudioProcessorEditor::erasingCircles()
{
    auto now = juce::Time::getCurrentTime();
    
    int lifeSpan = static_cast<int>(fadeOutDuration);

    waves.erase(std::remove_if(waves.begin(), waves.end(),
           [now, lifeSpan](const Wave& wave)
           {
               // Find if the corresponding circle is dead
               return (now - wave.creationTime).inSeconds() >= lifeSpan ;
           }),
           waves.end());

    circles.erase(std::remove_if(circles.begin(), circles.end(),
        [now, lifeSpan](const Circle& circle)
        {
        return (now - circle.creationTime).inSeconds() >= lifeSpan;
        }),
    circles.end());

    // Remove intersections from the vector after 20s
    intersectionPairs.erase(
            std::remove_if(intersectionPairs.begin(), intersectionPairs.end(),
                [this](const IntersectionPair& intersection)
                {
                    // Get the circles from the stored coordinates
                    auto circle1It = std::find_if(waves.begin(), waves.end(),
                        [&intersection](const Wave& w) {
                            return w.x == intersection.x1 && w.y == intersection.y1;
                        });

                    auto circle2It = std::find_if(waves.begin(), waves.end(),
                        [&intersection](const Wave& w) {
                            return w.x == intersection.x2 && w.y == intersection.y2;
                        });

                    // Check if circles are still present
                    if (circle1It == waves.end() || circle2It == waves.end())
                        return true; // Remove if either circle is missing

                    const Wave& w1 = *circle1It;
                    const Wave& w2 = *circle2It;
                    float r1 = calculateRadius(w1);
                    float r2 = calculateRadius(w2);

                    // Check if the circles still intersect
                    return !doCirclesIntersect(w1.x, w1.y, r1, w2.x, w2.y, r2);
                }),
            intersectionPairs.end());
}

void TekhneAudioProcessorEditor::update()
    {
        erasingCircles();

        for (auto& circle : circles)
            {
                float elapsedTime = (juce::Time::getCurrentTime() - circle.creationTime).inSeconds();
                if (elapsedTime >= 1.0f && std::fmod(elapsedTime, 4.0f) < 0.1f)
                {
                    waves.push_back(circle);
//                    DBG(waves.size());
                }

            }
    
//    DBG(opacity);
    
        repaint();
    }

void TekhneAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
    {
    // Get the click position relative to the component
    juce::Point<int> clickPosition = event.getPosition();

    // Store the click position and current time
    circles.push_back(Circle{
                            clickPosition.x,
                            clickPosition.y,
                            static_cast<int>(radiusSlider.getValue()),
                            static_cast<int>(growthSlider.getValue()),
                            juce::Time::getCurrentTime()
                        }
    );
    
    erasingCircles();

    repaint();

}

bool TekhneAudioProcessorEditor::doCirclesIntersect(int x1, int y1, float r1, int x2, int y2, float r2) {
    // Calculate the distance between the centers of the circles
    float distance = std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));

    // Avoid issues with very small or negative radii
    r1 = std::max(r1, 0.0f);
    r2 = std::max(r2, 0.0f);

    // Check for intersection
    if (distance > r1 + r2) {
        return false; // Circles are too far apart to intersect
    } else if (distance < std::abs(r1 - r2)) {
        return false; // One circle is completely inside the other without touching
    } else {
        return true; // Circles intersect or touch
    }
}

void TekhneAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill the background with a solid colour
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Set the colour for the circles
    g.setColour(getLookAndFeel().findColour(juce::Slider::thumbColourId));

    setColour(juce::Slider::thumbColourId, juce::Colours::white);
    setColour(juce::Slider::trackColourId, juce::Colours::white);

    bool newIntersectionFound = false;
    
//    for (int i = 0; i < waves.size(); ++i)
//    {
//        Wave& w1 = waves[i];
//
//        float elapsedTimeW = (juce::Time::getCurrentTime() - w1.creationTime).inSeconds();
//
//        float w1Radius =  w1.baseRadius + (elapsedTimeW * w1.growthRate);
////        DBG(w1.x - w1Radius);
//
//        int w1Diameter = static_cast<int>(w1Radius * 2.0f);
//
//        g.setColour(juce::Colours::white);
//        g.drawEllipse(w1.x - w1Radius, w1.y - w1Radius, w1Diameter, w1Diameter, 1.0f);
//    }

    for (int i = 0; i < circles.size(); ++i)
    {
        Circle& c1 = circles[i];
        float c1Radius = c1.baseRadius;
        int c1Diameter = static_cast<int>(c1Radius * 2.0f);

        float opacity = calculateOpacity(c1, decrementRate);
        
        DBG(opacity);
        
        g.setColour(juce::Colours::white.withAlpha(opacity));
        g.fillEllipse(c1.x - c1Radius, c1.y - c1Radius, c1Diameter, c1Diameter);
        
    }
    
    for (int i = 0; i < waves.size(); ++i)
    {
        Wave& w1 = waves[i];

        float w1Radius = calculateRadius(w1);
        int w1Diameter = static_cast<int>(w1Radius * 2.0f);
       
        g.setColour(juce::Colours::white);
        g.drawEllipse(w1.x - w1Radius, w1.y - w1Radius, w1Diameter, w1Diameter, 1.0f);
        
        for (int j = i + 1; j < waves.size(); ++j)
        {
            Wave& w2 = waves[j];
            float w2Radius = w2.baseRadius;

            if (doCirclesIntersect(w1.x, w1.y, w1Radius, w2.x, w2.y, w2Radius))
            {
                g.setColour(juce::Colours::violet);
                auto [ix1, iy1, ix2, iy2] = calculateIntersections(w1, w1Radius, w2, w2Radius);
                g.fillEllipse(ix1 - 5, iy1 - 5, 10, 10);
                g.fillEllipse(ix2 - 5, iy2 - 5, 10, 10);

                IntersectionPair newIntersection{
                    roundToDecimalPlaces(ix1, 1),
                    roundToDecimalPlaces(iy1, 1),
                    roundToDecimalPlaces(ix2, 1),
                    roundToDecimalPlaces(iy2, 1)
                };

                // Check if the intersection is new
                if (std::find(intersectionPairs.begin(), intersectionPairs.end(), newIntersection) == intersectionPairs.end())
                {
                    intersectionPairs.push_back(newIntersection);
                    newIntersectionFound = true;
                }
            }
        }
    }

    // If a new intersection is found, update the frequency
    if (newIntersectionFound)
    {
        getIntersectionsX();
    }
}

//void TekhneAudioProcessorEditor::updateToggleButtonState()
//{
//    for (int i = 0; i < circles.size(); ++i)
//    {
//        const Circle& c1 = circles[i];
//        float r1 = calculateRadius(c1);
//
//        for (int j = i + 1; j < circles.size(); ++j)
//        {
//            const Circle& c2 = circles[j];
//            float r2 = calculateRadius(c2);
//
//
//            if (doCirclesIntersect(c1.x, c1.y, r1, c2.x, c2.y, r2))
//            {
//                onOffButton.setToggleState(true, juce::NotificationType::dontSendNotification);
//                return;
//            }
//        }
//    }
//
//    onOffButton.setToggleState(false, juce::NotificationType::dontSendNotification);
//}

void TekhneAudioProcessorEditor::resized()
{
//     This is generally where you'll want to lay out the positions of any
//     subcomponents in your editor..
    onOffButton.setBounds(150, 100, 100, 30);
    radiusSlider.setBounds(10, 10, getWidth() / 3, 20);
    growthSlider.setBounds(10, 40, getWidth() / 3, 20);

    modFreq.setBounds(10, 100, getWidth() / 1.5, 20);
    fmDepth.setBounds(10, 130, getWidth() / 3, 20);
//    modFreq2.setBounds(10, 150, getWidth() / 3, 20);
//    fmDepth2.setBounds(10, 170, getWidth() / 3, 20);
}
