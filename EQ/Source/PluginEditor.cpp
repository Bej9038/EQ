/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::Colour MainColor = juce::Colour(255, 138, 101);
juce::Colour BGColor = juce::Colour(33, 33, 33);
juce::Colour GridColor = juce::Colour(66, 66, 66);
juce::Colour labelColorMain = MainColor;
float overlayEnabledAlpha = .04;
float overlayDisabledAlpha = .025;
float textAlpha = .5;
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
    slider.isEnabled() ? labelColorMain = MainColor : labelColorMain = MainColor.withAlpha(textAlpha);

    g.setColour(slider.isEnabled() ? Colours::white.withAlpha(overlayEnabledAlpha) : Colours::white.withAlpha(overlayDisabledAlpha));
    g.fillEllipse(bounds);

    g.setColour(slider.isEnabled() ? Colours::white.withAlpha(overlayEnabledAlpha) : Colours::white.withAlpha(overlayDisabledAlpha));
    g.drawEllipse(bounds, 1.0f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(bounds.getY() + 10);
        r.setWidth(3);
        p.addRectangle(r);
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        g.setColour(labelColorMain);
        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight().getHeight()+ 2);
        r.setCentre(bounds.getCentre());

        g.setColour(slider.isEnabled() ? Colours::white : Colours::white.withAlpha(textAlpha));
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& toggleButton,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDrown)
{
    using namespace juce;
    Path powerButton;
    auto bounds = toggleButton.getLocalBounds();
    auto size = jmin(bounds.getWidth()-10, bounds.getHeight()) - 10; 
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
    size -= 6;
    bounds.removeFromRight(bounds.getWidth() * .33);
    bounds.removeFromLeft(bounds.getWidth() * .33);
  
    PathStrokeType pst = PathStrokeType(2.f, PathStrokeType::JointStyle::curved);
    auto colorEdge = toggleButton.getToggleState() ? Colours::white.withAlpha(.1f) : Colours::white.withAlpha(0.f);
    auto colorFill = toggleButton.getToggleState() ? ColourGradient(Colours::white.withAlpha(0.f), 0, 0, Colours::white.withAlpha(0.f), 0, 0, false) : ColourGradient(MainColor, r.getCentreX(), r.getCentreY(), MainColor.withAlpha(.1f), r.getCentreX()-size + 2, r.getCentreY()-size + 2, true);
    g.setColour(colorEdge);
    g.strokePath(powerButton, pst);
    g.drawRoundedRectangle(r, 4.f, 1.f);
    g.setGradientFill(colorFill);
    g.fillRoundedRectangle(r, 4.f);
}

void LookAndFeel::drawComboBox(juce::Graphics& g,
    int 	width,
    int 	height,
    bool 	isButtonDown,
    int 	buttonX,
    int 	buttonY,
    int 	buttonW,
    int 	buttonH,
    juce::ComboBox& comboBox)
{
    using namespace juce;
    auto bounds = comboBox.getLocalBounds();
    bounds.reduce(15,15);
    g.setColour(Colours::red);
    g.drawRect(bounds);
    Rectangle<float> r;

    r.setSize(40, 20);
    


}

//==============================================================================

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto sliderBounds = getSliderBounds();
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAngSlider = degreesToRadians(180.f - 45.f) + 2*MathConstants<float>::pi;
    auto endAngJmap = degreesToRadians(180.f - 45.f);
    auto range = getRange();
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * .5f;
    getLookAndFeel().drawRotarySlider(g,    sliderBounds.getX(), 
                                            sliderBounds.getY(),
                                            sliderBounds.getWidth(),
                                            sliderBounds.getHeight(), 
                                            jmap(getValue(), range.getStart(), 
                                                             range.getEnd(), 0.0, 1.0),
                                                             startAng, endAngSlider, *this);
    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        if (i == 1)
        {
            g.setColour(labelColorMain);
            g.setFont(getLabelTextHeight());
            auto pos = labels[i].pos;
            auto ang = jmap(pos, 0.f, 2.f, startAng, endAngJmap);
            auto c = center.getPointOnCircumference(radius, ang);
            Rectangle<float> r;
            auto str = labels[i].label;
            r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight().getHeight());
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight().getHeight());
            g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
        }
        else
        {
            g.setColour(Colours::white.withAlpha(.4f));
            g.setFont(juce::Font("Roboto", 8, 0));

            auto pos = labels[i].pos;
            auto ang = jmap(pos, 0.f, 2.f, startAng, endAngJmap);
            auto c = center.getPointOnCircumference(radius + getTextHeight().getHeight(), ang);
            Rectangle<float> r;
            auto str = labels[i].label;
            r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight().getHeight());
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight().getHeight());
            g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
        }
        
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

ResponseCurveComponent::ResponseCurveComponent(EQAudioProcessor& p) :
    audioProcessor(p),
    leftPathProducer(audioProcessor.leftChannelFifo), 
    rightPathProducer(audioProcessor.rightChannelFifo)
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
    auto fftBounds = getRenderArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);

    if (parametersChanged.compareAndSetBool(false, true))
    {
        updateChain();
    }
    repaint();
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypass);
    monoChain.setBypassed<ChainPositions::Peak1>(chainSettings.peak1Bypass);
    monoChain.setBypassed<ChainPositions::Peak2>(chainSettings.peak2Bypass);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypass);

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

        if (!monoChain.isBypassed<ChainPositions::LowCut>())
        {
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
        }
        if (!monoChain.isBypassed<ChainPositions::HighCut>())
        {
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

    g.setColour(Colours::white.withAlpha(.6f));
    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath = leftChannelFFTPath.createPathWithRoundedCorners(150.f);
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath = rightChannelFFTPath.createPathWithRoundedCorners(150.f);
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));

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

    g.setColour(Colours::white.withAlpha(textAlpha));
    Font font = juce::Font("Roboto", 8, 0);
    g.setFont(font);

    for (int i = 1; i < freqs.size() - 1; ++i)
    {
        auto f = freqs[i];
        auto normX = mapFromLog10(f, 20.f, 20000.f);

        bool addK = false;
        String str;
        if (f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }
        str << f;
        if (addK)
        {
            str << "k";
        }

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, font.getHeight());
        r.setCentre(getWidth() * normX, 0);
        r.setY(getLocalBounds().getHeight() - (font.getHeight() + 1));
        g.drawFittedText(str, r, juce::Justification::verticallyCentred, 1);
    }

    for (int i = 1; i < gain.size() - 1; ++i)
    {
        auto gDb = gain[i];
        auto y = jmap(gDb, -24.f, 24.f, float(getHeight()), 0.f);
        String str;
        if (gDb > 0)
        {
            str << "+";
        }
        str << gDb;
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, font.getHeight());
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX() - 1, y+1);
        if (i == 2)
        {
            g.setColour(MainColor.withAlpha(textAlpha));
        }
        else
        {
            g.setColour(Colours::white.withAlpha(textAlpha));
        }
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.reduce(10, 8);
    return bounds;
}

//==============================================================================

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;

    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size),
                monoBuffer.getNumSamples() - size);
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size), tempIncomingBuffer.getReadPointer(0, 0), size);
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -96.f);
        }
    }
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);
    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -96.f);
        }
    }
    while (pathProducer.getNumPathsAvailable() > 0)
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
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
    lowCutQSlider(*audioProcessor.apvts.getParameter("LowCut Q"), ""),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    highCutQSlider(*audioProcessor.apvts.getParameter("LowCut Q"), ""),

    responseCurveComponent(audioProcessor),
    peak1FreqSliderAttachment(audioProcessor.apvts, "Peak1 Freq", peak1FreqSlider),
    peak1GainSliderAttachment(audioProcessor.apvts, "Peak1 Gain", peak1GainSlider),
    peak1QSliderAttachment(audioProcessor.apvts, "Peak1 Q", peak1QSlider),
    peak1BypassButtonAttachment(audioProcessor.apvts, "Peak1 Bypass", peak1BypassButton),
    peak2FreqSliderAttachment(audioProcessor.apvts, "Peak2 Freq", peak2FreqSlider),
    peak2GainSliderAttachment(audioProcessor.apvts, "Peak2 Gain", peak2GainSlider),
    peak2QSliderAttachment(audioProcessor.apvts, "Peak2 Q", peak2QSlider),
    peak2BypassButtonAttachment(audioProcessor.apvts, "Peak2 Bypass", peak2BypassButton),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlope),
    highCutSlopeAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlope),
    lowCutQSliderAttachment(audioProcessor.apvts, "LowCut Q", lowCutQSlider),
    highCutQSliderAttachment(audioProcessor.apvts, "HighCut Q", highCutQSlider),
    lowCutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypass", lowCutBypassButton),
    highCutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypass", highCutBypassButton)

{
    juce::StringArray arr = {"12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct"};
    lowCutSlope.addItemList(arr, 1);
    lowCutSlope.setSelectedItemIndex(0);
    highCutSlope.addItemList(arr, 1);
    highCutSlope.setSelectedItemIndex(0);

    peak1FreqSlider.labels.add({ 0.f, "20Hz" });
    peak1FreqSlider.labels.add({ 1.f, "FREQ" });
    peak1FreqSlider.labels.add({ 2.f, "20kHz" });
    peak1GainSlider.labels.add({ 0.f, "-24dB" });
    peak1GainSlider.labels.add({ 1.f, "GAIN" });
    peak1GainSlider.labels.add({ 2.f, "24dB" });
    peak1QSlider.labels.add({ 0.f, ".025" });
    peak1QSlider.labels.add({ 1.f, "Q" });
    peak1QSlider.labels.add({ 2.f, "10" });

    peak2FreqSlider.labels.add({ 0.f, "20Hz" });
    peak2FreqSlider.labels.add({ 1.f, "FREQ" });
    peak2FreqSlider.labels.add({ 2.f, "20kHz" });
    peak2GainSlider.labels.add({ 0.f, "-24dB" });
    peak2GainSlider.labels.add({ 1.f, "GAIN" });
    peak2GainSlider.labels.add({ 2.f, "24dB" });
    peak2QSlider.labels.add({ 0.f, ".025" });
    peak2QSlider.labels.add({ 1.f, "Q" });
    peak2QSlider.labels.add({ 2.f, "10" });

    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "FREQ" });
    lowCutFreqSlider.labels.add({ 2.f, "20kHz" });
    lowCutQSlider.labels.add({ 0.f, ".025" });
    lowCutQSlider.labels.add({ 1.f, "Q" });
    lowCutQSlider.labels.add({ 2.f, "10" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "FREQ" });
    highCutFreqSlider.labels.add({ 2.f, "20kHz" });
    highCutQSlider.labels.add({ 0.f, ".025" });
    highCutQSlider.labels.add({ 1.f, "Q" });
    highCutQSlider.labels.add({ 2.f, "10" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    peak1BypassButton.setLookAndFeel(&lnf);
    peak2BypassButton.setLookAndFeel(&lnf);
    lowCutBypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);
    lowCutBypassButton.triggerClick();
    highCutBypassButton.triggerClick();

    lowCutSlope.setLookAndFeel(&lnf);
    highCutSlope.setLookAndFeel(&lnf);


    auto safePtr = juce::Component::SafePointer<EQAudioProcessorEditor>(this);
    peak1BypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->peak1BypassButton.getToggleState();
            comp->peak1FreqSlider.setEnabled(!bypassed);
            comp->peak1GainSlider.setEnabled(!bypassed);
            comp->peak1QSlider.setEnabled(!bypassed);
        }
    };
    peak2BypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->peak2BypassButton.getToggleState();
            comp->peak2FreqSlider.setEnabled(!bypassed);
            comp->peak2GainSlider.setEnabled(!bypassed);
            comp->peak2QSlider.setEnabled(!bypassed);
        }
    };
    lowCutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->lowCutBypassButton.getToggleState();
            comp->lowCutFreqSlider.setEnabled(!bypassed);
            comp->lowCutSlope.setEnabled(!bypassed);
            comp->lowCutQSlider.setEnabled(!bypassed);
        }
    };
    highCutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->highCutBypassButton.getToggleState();
            comp->highCutFreqSlider.setEnabled(!bypassed);
            comp->highCutSlope.setEnabled(!bypassed);
            comp->highCutQSlider.setEnabled(!bypassed);
        }
    };

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
    bounds.removeFromTop(bounds.getHeight() * .03);

    responseCurveComponent.setBounds(responseArea);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * .25);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * .33);
    auto peak1Area = bounds.removeFromRight(bounds.getWidth() * .5);
    auto peak2Area = bounds.removeFromRight(bounds.getWidth());

    lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * .4));
    lowCutQSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * .5));
    lowCutSlope.setBounds(lowCutArea);

    highCutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * .4));
    highCutQSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() *.5));
    highCutSlope.setBounds(highCutArea);

    peak1BypassButton.setBounds(peak1Area.removeFromTop(25));
    peak1FreqSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .4));
    peak1QSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .5));
    peak1GainSlider.setBounds(peak1Area);

    peak2BypassButton.setBounds(peak2Area.removeFromTop(25));
    peak2FreqSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * .4));
    peak2QSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * .5));
    peak2GainSlider.setBounds(peak2Area);
}

std::vector<juce::Component*> EQAudioProcessorEditor::getComps()
{
    return
    {
        &responseCurveComponent,

        &peak1FreqSlider,
        &peak1GainSlider,
        &peak1QSlider,
        &peak1BypassButton,

        &peak2FreqSlider,
        &peak2GainSlider,
        &peak2QSlider,
        &peak2BypassButton,

        &lowCutFreqSlider,
        &lowCutQSlider,
        &lowCutSlope,
        &lowCutBypassButton,

        &highCutFreqSlider,
        &highCutBypassButton,
        &highCutQSlider,
        &highCutSlope
    };
}
