/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // first apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);       // [1]

        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  // [2]

        int numBins = (int)fftSize / 2;

        //normalize the fft values.
        for (int i = 0; i < numBins; ++i)
        {
            auto v = fftData[i];
            //            fftData[i] /= (float) numBins;
            if (!std::isinf(v) && !std::isnan(v))
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }

        //convert them to decibels
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 0.f,
                float(bottom + 10), top);
        };

        auto y = map(renderData[0]);

        //        jassert( !std::isnan(y) && !std::isinf(y) );
        if (std::isnan(y) || std::isinf(y))
            y = bottom;

        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            //            jassert( !std::isnan(y) && !std::isinf(y) );

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;

    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& toggleButton,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g,
        int 	width,
        int 	height,
        bool 	isButtonDown,
        int 	buttonX,
        int 	buttonY,
        int 	buttonW,
        int 	buttonH,
        juce::ComboBox& comboBox) override;

};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : 
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }
    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }
    struct LabelPos
    {
        float pos;
        juce::String label;
    };
    juce::Array<LabelPos> labels;
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    juce::Font getTextHeight() const { return juce::Font("Roboto", 14, 1); }
    juce::Font getLabelTextHeight() const { return juce::Font("Roboto", 10, 0); }
    juce::String getDisplayString() const;
private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<EQAudioProcessor::BlockType>& scsf) : leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order4096);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath; }
private:
    SingleChannelSampleFifo<EQAudioProcessor::BlockType>* leftChannelFifo;
    juce::AudioBuffer<float> monoBuffer;
    FFTDataGenerator<std::vector<float>>leftChannelFFTDataGenerator;
    AnalyzerPathGenerator<juce::Path> pathProducer;
    juce::Path leftChannelFFTPath;
};

struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    ResponseCurveComponent(EQAudioProcessor&);
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool genstureIsStarting) override {};
    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    EQAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };
    MonoChain monoChain;
    void updateChain();
    juce::Image background;
    juce::Rectangle<int> getRenderArea();
    PathProducer leftPathProducer, rightPathProducer;
};

//==============================================================================
/**
*/
class EQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    EQAudioProcessorEditor (EQAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    EQAudioProcessor& audioProcessor;
    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using ComboBoxAttachment = APVTS::ComboBoxAttachment;
    using ToggleButtonAttachment = APVTS::ButtonAttachment;

    RotarySliderWithLabels  
        peak1FreqSlider,
        peak1GainSlider,
        peak1QSlider,
        peak2FreqSlider,
        peak2GainSlider,
        peak2QSlider,
        lowCutFreqSlider,
        lowCutQSlider,
        highCutQSlider,
        highCutFreqSlider;
    SliderAttachment  
        lowCutFreqSliderAttachment,
        lowCutQSliderAttachment,
        highCutFreqSliderAttachment,
        highCutQSliderAttachment,
        peak1FreqSliderAttachment,
        peak1GainSliderAttachment,
        peak1QSliderAttachment,
        peak2FreqSliderAttachment,
        peak2GainSliderAttachment,
        peak2QSliderAttachment;
    juce::ToggleButton 
        lowCutBypassButton, 
        highCutBypassButton, 
        peak1BypassButton,
        peak2BypassButton;
    ToggleButtonAttachment 
        lowCutBypassButtonAttachment, 
        highCutBypassButtonAttachment, 
        peak1BypassButtonAttachment, 
        peak2BypassButtonAttachment;
    juce::ComboBox lowCutSlope, highCutSlope;
    ComboBoxAttachment lowCutSlopeAttachment,
        highCutSlopeAttachment;

    std::vector<juce::Component*> getComps();
    LookAndFeel lnf;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQAudioProcessorEditor)
};
