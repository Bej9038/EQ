/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(EQAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    startTimerHz(60);
}
void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peak1Coefficients = makePeak1(chainSettings, audioProcessor.getSampleRate());
        auto peak2Coefficients = makePeak2(chainSettings, audioProcessor.getSampleRate());

        updateCoefficients(monoChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
        updateCoefficients(monoChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);

        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        repaint();
    }
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;

    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();
    /*g.fillAll(Colour(26, 33, 32));*/
    DropShadow shadow = DropShadow(Colour(0, 0, 0), -100, { 0, 0 });
    shadow.drawForRectangle(g, responseArea);
    
    
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

    g.setColour(Colour(128, 203, 196));
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

//==============================================================================

EQAudioProcessorEditor::EQAudioProcessorEditor(EQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    peak1FreqSlider(*audioProcessor.apvts.getParameter("Peak1 Freq"), "Hz"),
    peak1GainSlider(*audioProcessor.apvts.getParameter("Peak1 Gain"), "dB"),
    peak1QSlider(*audioProcessor.apvts.getParameter("Peak1 Q"), ""),
    peak2FreqSlider(*audioProcessor.apvts.getParameter("Peak2 Freq"), "Hz"),
    peak2GainSlider(*audioProcessor.apvts.getParameter("Peak2 Gain"), "dB"),
    peak2QSlider(*audioProcessor.apvts.getParameter("Peak2 Q"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),

    responseCurveComponent(audioProcessor),
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

void EQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(18, 18, 18));
}

void EQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .33);

    responseCurveComponent.setBounds(responseArea);

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
        &highCutSlope,
        &responseCurveComponent
    };
}
