/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQAudioProcessorEditor::EQAudioProcessorEditor(EQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    peak1FreqSliderAttachment(audioProcessor.apvts, "Peak1 Freq", peak1FreqSlider),
    peak1GainSliderAttachment(audioProcessor.apvts, "Peak1 Gain", peak1GainSlider),
    peak1QSliderAttachment(audioProcessor.apvts, "Peak1 Q", peak1QSlider),
    peak2FreqSliderAttachment(audioProcessor.apvts, "Peak2 Freq", peak2FreqSlider),
    peak2GainSliderAttachment(audioProcessor.apvts, "Peak2 Gain", peak2GainSlider),
    peak2QSliderAttachment(audioProcessor.apvts, "Peak2 Q", peak2QSlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlope),
    highCutSlopeAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlope)

{
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    setSize (800, 400);
}

EQAudioProcessorEditor::~EQAudioProcessorEditor()
{
}

//==============================================================================
void EQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
}

void EQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .33);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * .25);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * .33);
    auto peak1Area = bounds.removeFromRight(bounds.getWidth() * .5);
    peak1Area.removeFromTop(peak1Area.getHeight() * .1);
    auto peak2Area = bounds.removeFromRight(bounds.getWidth());
    peak2Area.removeFromTop(peak2Area.getHeight() * .1);

    lowCutFreqSlider.setBounds(lowCutArea);
    lowCutSlope.setBounds(lowCutArea.removeFromBottom(lowCutArea.getCentreY() * .15));
    highCutFreqSlider.setBounds(highCutArea);
    highCutSlope.setBounds(highCutArea.removeFromBottom(highCutArea.getCentreY() * .15));

    peak1FreqSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .4));
    peak1GainSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .5));
    peak1QSlider.setBounds(peak1Area);

    peak2FreqSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * .4));
    peak2GainSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * .5));
    peak2QSlider.setBounds(peak2Area);
}

std::vector<juce::Component*> EQAudioProcessorEditor::getComps()
{
    return
    {
        &peak1FreqSlider,
        &peak1GainSlider,
        &peak1QSlider,
        &peak2FreqSlider,
        &peak2GainSlider,
        &peak2QSlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlope,
        &highCutSlope
    };
}
