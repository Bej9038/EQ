/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::Colour MainColor = juce::Colour(255, 171, 145);
juce::Colour BGColor = juce::Colour(33, 33, 33);
juce::Colour GridColor = juce::Colour(66, 66, 66);
float overlay1Alpha = .04;
float gridAlpha = .8;
float gridGap = 35;

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    g.setColour(Colours::white.withAlpha(overlay1Alpha));
    g.fillEllipse(bounds);

    g.setColour(Colours::white.withAlpha(overlay1Alpha));
    g.drawEllipse(bounds, 1.0f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(bounds.getY() + 8);
        r.setWidth(3);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight().getHeight()+ 2);
        r.setCentre(bounds.getCentre());

        g.setColour(juce::Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }

    auto center = bounds.getCentre();
    Path p;
    Rectangle<float> r;
    r.setLeft(center.getX() - 2);
    r.setRight(center.getX() + 2);
    r.setTop(bounds.getY());
    r.setBottom(bounds.getY() + 8);
    r.setWidth(3);

    p.addRectangle(r);
    jassert(rotaryStartAngle < rotaryEndAngle);

    auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
    p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
    g.setColour(MainColor);
    g.fillPath(p);
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    auto range = getRange();
    auto sliderBounds = getSliderBounds();

    

    getLookAndFeel().drawRotarySlider(g,    sliderBounds.getX(), 
                                            sliderBounds.getY(),
                                            sliderBounds.getWidth(),
                                            sliderBounds.getHeight(), 
                                            jmap(getValue(), range.getStart(), 
                                                             range.getEnd(), 0.0, 1.0),
                                                             startAng, endAng, *this);
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * .5f;
    g.setColour(MainColor);
    g.setFont(getLabelTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(1.f >= pos);
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        auto c = center.getPointOnCircumference(radius + getTextHeight().getHeight() * .5f + 1, ang);
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight().getHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight().getHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight().getHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(5);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;
    bool addK = false;
    float val = getValue();
    if (val > 999.f)
    {
        val /= 1000.f;
        addK = true;
    }
    str = juce::String(val, (addK ? 2 : 0));
    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
        {
            str << "k";
        }
        str << suffix;
    }
    return str;
}

//==============================================================================

ResponseCurveComponent::ResponseCurveComponent(EQAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    updateChain();
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
        updateChain();
        repaint();
    }
}

void ResponseCurveComponent::updateChain()
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
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;

    auto responseArea = getRenderArea();
    auto w = responseArea.getWidth();
    g.fillAll(Colours::black);
    g.drawImage(background, getLocalBounds().toFloat());
    
    
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
        auto freq = std::pow(2.0, (double(i) / double(w) * (std::log2(20000) - std::log2(20)) + std::log2(20)));
        /*auto freq = (double(i) / double(w)) * (20000 - 20) + 20;*/

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

    g.setColour(MainColor);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);
    auto renderArea = getRenderArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    Array<float> freqs
    {
        20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 4000, 5000, 10000, 20000
    };

    Array<float> xs;
    for (auto f : freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }

    ColourGradient vgradient1 = ColourGradient(Colours::white.withAlpha(0.f), 0, 0, GridColor.withAlpha(gridAlpha), 0, gridGap, false);
    ColourGradient vgradient2 = ColourGradient(Colours::white.withAlpha(0.f), 0, getHeight(), GridColor.withAlpha(gridAlpha), 0, getHeight() - gridGap, false);
    g.setGradientFill(vgradient1);
    for (auto f : freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        g.drawVerticalLine(getWidth() * normX, 0.f, getHeight() * .5);
    }
    g.setGradientFill(vgradient2);
    for (auto f : freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        g.drawVerticalLine(getWidth() * normX, getHeight() * .5, getHeight());
    }

    Array<float> gain
    {
        -24, -12, 0, 12, 24
    };
    ColourGradient hgradient1 = ColourGradient(Colours::white.withAlpha(0.f), 0, 0, GridColor.withAlpha(gridAlpha), gridGap, 0, false);
    ColourGradient hgradient2 = ColourGradient(Colours::white.withAlpha(0.f), getWidth(), 0, GridColor.withAlpha(gridAlpha), getWidth() - gridGap, 0, false);

    g.setGradientFill(hgradient1);
    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(getHeight()), 0.f);
        g.drawHorizontalLine(y, 0, getWidth() * .5);
    }
    g.setGradientFill(hgradient2);
    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(getHeight()), 0.f);
        g.drawHorizontalLine(y, getWidth() * .5, getWidth());
    }

    auto y = jmap(0.f, -24.f, 24.f, float(getHeight()), 0.f);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.f), 0, 0, MainColor.withAlpha(.25f), gridGap, 0, false));
    g.drawHorizontalLine(y, 0, getWidth() * .5);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.f), getWidth(), 0, MainColor.withAlpha(.25f), getWidth() - gridGap, 0, false));
    g.drawHorizontalLine(y, getWidth() * .5, getWidth());

}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.reduce(10, 8);
    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(gridGap);
    bounds.removeFromBottom(gridGap);
    return bounds;
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
    

    peak1FreqSlider.labels.add({ 0.f, "20Hz" });
    peak1FreqSlider.labels.add({ 1.f, "20kHz" });
    peak1GainSlider.labels.add({ 0.f, "-24dB" });
    peak1GainSlider.labels.add({ 1.f, "24dB" });
    peak1QSlider.labels.add({ 0.f, ".025" });
    peak1QSlider.labels.add({ 1.f, "10" });

    peak2FreqSlider.labels.add({ 0.f, "20Hz" });
    peak2FreqSlider.labels.add({ 1.f, "20kHz" });
    peak2GainSlider.labels.add({ 0.f, "-24dB" });
    peak2GainSlider.labels.add({ 1.f, "24dB" });
    peak2QSlider.labels.add({ 0.f, ".025" });
    peak2QSlider.labels.add({ 1.f, "10" });

    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    setSize (800, 600);
}

void EQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BGColor);
}

void EQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .33);
    bounds.removeFromTop(bounds.getHeight() * .05);

    responseCurveComponent.setBounds(responseArea);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * .25);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * .33);
    auto peak1Area = bounds.removeFromRight(bounds.getWidth() * .5);
    auto peak2Area = bounds.removeFromRight(bounds.getWidth());

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
