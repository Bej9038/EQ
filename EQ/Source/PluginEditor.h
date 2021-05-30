/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {}
};

struct CustomComboBox : juce::ComboBox
{
    CustomComboBox() : juce::ComboBox()
    {}
};

//==============================================================================
/**
*/
class EQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EQAudioProcessorEditor (EQAudioProcessor&);
    ~EQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EQAudioProcessor& audioProcessor;

    CustomRotarySlider  peak1FreqSlider,
        peak1GainSlider,
        peak1QSlider,
        peak2FreqSlider,
        peak2GainSlider,
        peak2QSlider,
        lowCutFreqSlider,
        highCutFreqSlider;

    CustomComboBox lowCutSlope, highCutSlope;

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using ComboBoxAttachment = APVTS::ComboBoxAttachment;
    SliderAttachment  lowCutFreqSliderAttachment,
        highCutFreqSliderAttachment,
        peak1FreqSliderAttachment,
        peak1GainSliderAttachment,
        peak1QSliderAttachment,
        peak2FreqSliderAttachment,
        peak2GainSliderAttachment,
        peak2QSliderAttachment;

    ComboBoxAttachment lowCutSlopeAttachment,
        highCutSlopeAttachment;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQAudioProcessorEditor)
};
