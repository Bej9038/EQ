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
    juce::StringArray arr = {"Slope_12", "Slope_24", "Slope_36", "Slope_48"};
    lowCutSlope.addItemList(arr, 1);
    lowCutSlope.setSelectedId(1);
    highCutSlope.addItemList(arr, 1);
    highCutSlope.setSelectedId(1);
    

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
    using namespace juce;
    g.fillAll (Colours::black);
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .33);
    auto w = responseArea.getWidth();
    auto& lc = monoChain.get<ChainPositions::LowCut>();
    auto& p1 = monoChain.get<ChainPositions::Peak1>();
    auto& p2 = monoChain.get<ChainPositions::Peak2>();
    auto& hc = monoChain.get<ChainPositions::HighCut>();
    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;
    mags.resize(w);
    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak1>())
        {
            mag *= p1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!monoChain.isBypassed<ChainPositions::Peak2>())
        {
            mag *= p2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!lc.isBypassed<0>())
        {
            mag *= lc.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lc.isBypassed<1>())
        {
            mag *= lc.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lc.isBypassed<2>())
        {
            mag *= lc.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lc.isBypassed<3>())
        {
            mag *= lc.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!hc.isBypassed<0>())
        {
            mag *= hc.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!hc.isBypassed<1>())
        {
            mag *= hc.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!hc.isBypassed<2>())
        {
            mag *= hc.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!hc.isBypassed<3>())
        {
            mag *= hc.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
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

void EQAudioProcessorEditor::parameterValueChanged(int paramterIndex, float newValue)
{
    parametersChanged.set(true);
}

void EQAudioProcessorEditor::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {

    }
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
