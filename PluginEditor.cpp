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
    radiusSlider.setRange(5.0, 50.0, 1.0);
    radiusSlider.setValue(20.0);
    radiusSlider.addListener(this);
    radiusSlider.setTextValueSuffix(" px");

    radiusSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    radiusSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
    
//    addAndMakeVisible(radiusSlider);
    
    growthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    growthSlider.setRange(1.0, 20.0, 1.0);
    growthSlider.setValue(4.0);
    growthSlider.addListener(this);

    growthSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    growthSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
    
//    addAndMakeVisible(growthSlider);
    
    waveDistance.setSliderStyle(juce::Slider::LinearHorizontal);
    waveDistance.setRange(2.0, 9.0, 1.0);
    waveDistance.setValue(4.0);
    waveDistance.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    waveDistance.setColour(juce::Slider::trackColourId, juce::Colours::white);
    waveDistance.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);  // Disable built-in text box

    addAndMakeVisible(waveDistance);

    // Create a label for the Wave Distance slider
    waveDistanceLabel.setText("Water viscosity", juce::dontSendNotification);
    waveDistanceLabel.attachToComponent(&waveDistance, false); // Attach it to the left side
    waveDistanceLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(waveDistanceLabel);
    
    carrierFreq.setSliderStyle(juce::Slider::LinearHorizontal);
    carrierFreq.setRange(200.0, 2000.0, 1.0);
    carrierFreq.setValue(440.0);
    carrierFreq.addListener(this);
    carrierFreq.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    carrierFreq.setColour(juce::Slider::trackColourId, juce::Colours::white);
    carrierFreq.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);  // Disable built-in text box

    addAndMakeVisible(carrierFreq);

    // Create a label for the Carrier Frequency slider
    carrierFreqLabel.setText("Water turbulence", juce::dontSendNotification);
    carrierFreqLabel.attachToComponent(&carrierFreq, true); // Attach it to the left side
    carrierFreqLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(carrierFreqLabel);
    
    setSize(700, 700);
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

           juce::NormalisableRange<float> frequencyRange(2000.0f, 5.0f);
           float frequencyValue = frequencyRange.convertFrom0to1(latestIntersectionX / static_cast<float>(width));
           
           float quantizedFrequency = quantizeFrequency(frequencyValue);

           audioProcessor.treeState.getParameter("frequency")->setValueNotifyingHost(frequencyRange.convertTo0to1(static_cast<float>(quantizedFrequency)));
       }
}


void TekhneAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &radiusSlider)
    {
        update();
    }
    if (slider == &waveDistance)
    {
//        audioProcessor.setModulatorParameters(distance_center, modulationIndexID, waveLife);
    }
    else if (slider == &carrierFreq)
       {
           carrierFreqValueLabel.setText(juce::String(carrierFreq.getValue()) + " Hz", juce::dontSendNotification);
           
           float freq = carrierFreq.getValue();
           float value = quantizeFrequency(freq);
           audioProcessor.treeState.getParameter("frequency")->setValueNotifyingHost(audioProcessor.treeState.getParameter("frequency")->getNormalisableRange().convertTo0to1(value));
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
           float value = modFreq2.getValue();
           audioProcessor.treeState.getParameter("modFreq2")->setValueNotifyingHost(audioProcessor.treeState.getParameter("modFreq2")->getNormalisableRange().convertTo0to1(value));
       }
    else if (slider == &fmDepth2)
       {
           float value = fmDepth2.getValue();
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

    for (auto it = circles.begin(); it != circles.end(); /* no increment */)
        {
            if ((now - it->creationTime).inSeconds() >= lifeSpan)
            {
                availableIDs.insert(it->id);  // Recycle the ID
                it = circles.erase(it);       // Remove the circle and update the iterator
            }
            else
            {
                ++it;  // Increment the iterator only if no circle was erased
            }
        }

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
        
                    if (elapsedTime <= 0.2f || (elapsedTime >= 1.0f && std::fmod(elapsedTime, circle.waveDistance) < 0.1f))
                    {
                        waves.push_back(circle);
                    }

            }
    
        repaint();
    }

void TekhneAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
    {

    juce::Point<int> clickPosition = event.getPosition();
    
    int id;
    
       if (!availableIDs.empty())
       {
           id = *availableIDs.begin();
           availableIDs.erase(availableIDs.begin());
       }
       else
       {
           id = nextID++;  // Use the next new ID
       }

    circles.push_back(Circle{
                            clickPosition.x,
                            clickPosition.y,
                            static_cast<int>(radiusSlider.getValue()),
                            static_cast<int>(growthSlider.getValue()),
                            static_cast<int>(waveDistance.getValue()),
                            id,
                            juce::Time::getCurrentTime()
                        }
    );
    
    erasingCircles();

    repaint();
    
//    for (const auto& circle : circles)  // Iterate over each circle
//        {
//            DBG("Circle ID: " << circle.id);
//        }

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

//    bool newIntersectionFound = false;
    
    g.setColour(juce::Colours::hotpink);
    g.fillEllipse(getWidth() / 2 - 4, getHeight() / 2 - 4, 8, 8);
    

    for (int i = 0; i < circles.size(); ++i)
    {
        Circle& c1 = circles[i];
        float c1Radius = c1.baseRadius;
        int c1Diameter = static_cast<int>(c1Radius * 2.0f);

        float opacity = calculateOpacity(c1, decrementRate);
        
        g.setColour(juce::Colours::white.withAlpha(opacity));
        g.fillEllipse(c1.x - c1Radius, c1.y - c1Radius, c1Diameter, c1Diameter);
        
        int s1 = (c1.x) - getWidth() / 2;
        int s2 = (c1.y) - getHeight() / 2;
        
        distance_center = sqrt(pow(s1, 2) + pow(s2, 2));
//        DBG(distance_center);
        
        int modulationIndexID = c1.id;
        
        audioProcessor.setModulatorParameters(distance_center, modulationIndexID, c1.waveDistance);
    }
    
    for (int i = 0; i < waves.size(); ++i)
    {
        Wave& w1 = waves[i];

        float w1Radius = calculateRadius(w1);
        int w1Diameter = static_cast<int>(w1Radius * 2.0f);
       
        g.setColour(juce::Colours::white);
        g.drawEllipse(w1.x - w1Radius, w1.y - w1Radius, w1Diameter, w1Diameter, 1.0f);
        
//        float expanded_r = w1.baseRadius + (w1.growthRate * fadeOutDuration);
//        std::cout << "expanded_r: " << expanded_r << std::endl;
//           std::cout << "w1.x - expanded_r: " << (w1.x - expanded_r) << std::endl;
//           std::cout << "w1.x - w1Radius: " << (w1.x - w1.baseRadius) << std::endl;
        
//        for (int j = i + 1; j < waves.size(); ++j)
//        {
//            Wave& w2 = waves[i];
//
//            float w2Radius = calculateRadius(w2);
//
//            if (w1.circleID != w2.circleID)
//            {
//            if (doCirclesIntersect(w1.x, w1.y, w1Radius, w2.x, w2.y, w2Radius))
//            {
//                DBG("intersecting");
//
//                g.setColour(juce::Colours::violet);
//                auto [ix1, iy1, ix2, iy2] = calculateIntersections(w1, w1Radius, w2, w2Radius);
//                g.fillEllipse(ix1 - 5, iy1 - 5, 10, 10);
//                g.fillEllipse(ix2 - 5, iy2 - 5, 10, 10);
//
//                IntersectionPair newIntersection{
//                    roundToDecimalPlaces(ix1, 1),
//                    roundToDecimalPlaces(iy1, 1),
//                    roundToDecimalPlaces(ix2, 1),
//                    roundToDecimalPlaces(iy2, 1)
//                };
//
//                // Check if the intersection is new
//                if (std::find(intersectionPairs.begin(), intersectionPairs.end(), newIntersection) == intersectionPairs.end())
//                {
//                    intersectionPairs.push_back(newIntersection);
//                    newIntersectionFound = true;
//                }
//            }
            }
        }
//    }

    // If a new intersection is found, update the frequency
//    if (newIntersectionFound)
//    {
//        getIntersectionsX();
//    }

//}

void TekhneAudioProcessorEditor::resized()
{
//     This is generally where you'll want to lay out the positions of any
//     subcomponents in your editor..

    waveDistance.setBounds(10, 10, getWidth() / 1.5, 20);

    carrierFreq.setBounds(10, 50, getWidth() / 1.5, 20);
    
    carrierFreqLabel.setBounds(10, 65, 120, 20);
    waveDistanceLabel.setBounds(10, 25, 105, 20);
    
}
