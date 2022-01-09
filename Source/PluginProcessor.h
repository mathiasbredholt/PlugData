/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Console.h"
#include "Pd/PdInstance.hpp"
#include "Pd/PdLibrary.hpp"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include <ff_meters/ff_meters.h>

//==============================================================================
/**
 */

class PlugDataPluginEditor;
class PlugDataAudioProcessor : public AudioProcessor, public pd::Instance, public Thread {

public:
    //==============================================================================
    PlugDataAudioProcessor(Console* console = nullptr);
    ~PlugDataAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // pure-data run loop when DAW isn't calling process block
    
    std::atomic<float> timeSinceProcess;
    void run() override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    //==============================================================================
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void receiveNoteOn(const int channel, const int pitch, const int velocity) override;
    void receiveControlChange(const int channel, const int controller, const int value) override;
    void receiveProgramChange(const int channel, const int value) override;
    void receivePitchBend(const int channel, const int value) override;
    void receiveAftertouch(const int channel, const int value) override;
    void receivePolyAftertouch(const int channel, const int pitch, const int value) override;
    void receiveMidiByte(const int port, const int byte) override;

    void receivePrint(const std::string& message) override
    {
        if(!message.empty())
           {
               if(!message.compare(0, 6, "error:"))
               {
                   const auto temp = String(message);
                   if (console) console->logError(temp.substring(7));
               }
               else if(!message.compare(0, 11, "verbose(4):"))
               {
                   const auto temp = String(message);
                   if (console) console->logError(temp.substring(12));
               }
               else if(!message.compare(0, 5, "tried"))
               {
                   if(console) console->logMessage(message);
               }
               else if(!message.compare(0, 16, "input channels ="))
               {
                   if(console) console->logMessage(message);
               }
               else
               {
                   if(console) console->logMessage(message);
               }
           }
    };

    void process(AudioSampleBuffer&, MidiBuffer&);

    // PD ENVIRONMENT VARIABLES
    static inline std::string getPatchPath() { return "/Users/timschoen/Documents/CircuitLab/.work/"; }

    //! @brief Gets the name of the Pd patch.
    static std::string getPatchName() { return "patch"; }

    void setBypass(bool bypass)
    {
        *enabled = !bypass;
    }

    void setCallbackLock(const CriticalSection* lock)
    {
        audioLock = lock;
    };

    const CriticalSection* getCallbackLock() override
    {
        return audioLock;
    };

    void initialiseFilesystem();
    void saveSettings();
    void updateSearchPaths();

    void sendMidiBuffer();
    
    void messageEnqueued() override;
    
    void loadPatch(String patch);

    Console* console;

    int lastUIWidth = 900, lastUIHeight = 600;

    AudioBuffer<float> processingBuffer;


    std::atomic<float>* volume;

    std::vector<pd::Atom> parameterAtom = std::vector<pd::Atom>(1);

    int numin;
    int numout;
    int sampsperblock = 512;

    ValueTree settingsTree = ValueTree("PlugDataSettings");

    pd::Library objectLibrary;

    static inline File homeDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("PlugData");
    static inline File appDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");

    static inline File settingsFile = appDir.getChildFile("Settings.xml");
    static inline File abstractions = appDir.getChildFile("Abstractions");

    bool locked = false;

    AudioProcessorValueTreeState parameters;

    foleys::LevelMeterSource meterSource;

private:
    void processInternal();

    bool ownsConsole;

    String const m_name = String("PlugData");
    bool const m_accepts_midi = true;
    bool const m_produces_midi = true;
    bool const m_is_midi_effect = false;
    std::atomic<float>* enabled;

    int m_audio_advancement;
    std::vector<float> m_audio_buffer_in;
    std::vector<float> m_audio_buffer_out;

    MidiBuffer m_midi_buffer_in;
    MidiBuffer m_midi_buffer_out;
    MidiBuffer m_midi_buffer_temp;

    bool m_midibyte_issysex = false;
    uint8 m_midibyte_buffer[512];
    size_t m_midibyte_index = 0;

    std::array<std::atomic<float>*, 8> parameterValues;
    std::array<float, 8> lastParameters;

    const CriticalSection* audioLock;
    double samplerate;
    
    std::atomic<bool> isDequeueing = false;

    MainLook mainLook;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataAudioProcessor)
};
