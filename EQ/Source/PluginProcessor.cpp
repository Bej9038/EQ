/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQAudioProcessor::EQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

EQAudioProcessor::~EQAudioProcessor()
{
}

//==============================================================================
const juce::String EQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EQAudioProcessor::getProgramName (int index)
{
    return {};
}

void EQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

    updateFilters();
}

void EQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}

//==============================================================================
bool EQAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* EQAudioProcessor::createEditor()
{
    return new EQAudioProcessorEditor (*this);
}

//==============================================================================
void EQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void EQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.lowCutQ = apvts.getRawParameterValue("LowCut Q")->load();
    settings.lowCutBypass = apvts.getRawParameterValue("LowCut Bypass")->load() > .5f;

    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    settings.highCutQ = apvts.getRawParameterValue("HighCut Q")->load();
    settings.highCutBypass = apvts.getRawParameterValue("HighCut Bypass")->load() > .5f;

    settings.peak1Freq = apvts.getRawParameterValue("Peak1 Freq")->load();
    settings.peak1GainDB = apvts.getRawParameterValue("Peak1 Gain")->load();
    settings.peak1Q = apvts.getRawParameterValue("Peak1 Q")->load();
    settings.peak1Bypass = apvts.getRawParameterValue("Peak1 Bypass")->load() > .5f;

    settings.peak2Freq = apvts.getRawParameterValue("Peak2 Freq")->load();
    settings.peak2GainDB = apvts.getRawParameterValue("Peak2 Gain")->load();
    settings.peak2Q = apvts.getRawParameterValue("Peak2 Q")->load();
    settings.peak2Bypass = apvts.getRawParameterValue("Peak2 Bypass")->load() > .5f;
    
    return settings;
}

Coefficients makePeak1(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peak1Freq,
        chainSettings.peak1Q,
        juce::Decibels::decibelsToGain(chainSettings.peak1GainDB));
}
Coefficients makePeak2( const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peak2Freq,
        chainSettings.peak2Q,
        juce::Decibels::decibelsToGain(chainSettings.peak2GainDB));
}

void EQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    auto peak1Coefficients = makePeak1(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::Peak1>(chainSettings.peak1Bypass);
    rightChain.setBypassed<ChainPositions::Peak1>(chainSettings.peak1Bypass);

    updateCoefficients(leftChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);

    auto peak2Coefficients = makePeak2(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::Peak2>(chainSettings.peak2Bypass);
    rightChain.setBypassed<ChainPositions::Peak2>(chainSettings.peak2Bypass);
    updateCoefficients(leftChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

void EQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypass);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypass);

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void EQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.lowCutBypass);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.lowCutBypass);

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);

    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void EQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    updatePeakFilter(chainSettings);
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
}



juce::AudioProcessorValueTreeState::ParameterLayout 
EQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    juce::StringArray strArr;
    for (int i = 0; i < 4; i++)
    {
        juce::String str;
        str << (12 + i * 12);
        str << "dB/oct";
        strArr.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("LowCut Freq", "LowCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, .01f, .25f), 20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("LowCut Q", "LowCut Q", juce::NormalisableRange<float>(0.025f, 10.f, .001f, 1.f), 1.f));

    layout.add(std::make_unique < juce::AudioParameterBool>
        ("LowCut Bypass", "LowCut Bypass", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", strArr, 0));



    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("HighCut Freq", "HighCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, .01f, .25f), 20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("HighCut Q", "HighCut Q", juce::NormalisableRange<float>(0.025f, 10.f, .001f, 1.f), 1.f));

    layout.add(std::make_unique < juce::AudioParameterBool>
        ("HighCut Bypass", "HighCut Bypass", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", strArr, 0));



    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Peak1 Freq", "Peak1 Freq", juce::NormalisableRange<float>(20.f, 20000.f, .01f, .25f), 3000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Peak1 Gain", "Peak1 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.01f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Peak1 Q", "Peak1 Q", juce::NormalisableRange<float>(0.025f, 10.f, .001f, 1.f), 1.f));

    layout.add(std::make_unique < juce::AudioParameterBool>
        ("Peak1 Bypass", "Peak1 Bypass", false));



    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Peak2 Freq", "Peak2 Freq", juce::NormalisableRange<float>(20.f, 20000.f, .01f, .25f), 200.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Peak2 Gain", "Peak2 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.01f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>
        ("Peak2 Q", "Peak2 Q", juce::NormalisableRange<float>(0.025f, 10.f, .001f, 1.f), 1.f));

    layout.add(std::make_unique < juce::AudioParameterBool>
        ("Peak2 Bypass", "Peak2 Bypass", false));
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQAudioProcessor();
}
