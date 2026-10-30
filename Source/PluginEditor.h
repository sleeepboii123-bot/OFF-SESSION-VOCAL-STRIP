#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

class VocalStripLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;
};

class VocalStripModuleTile final : public juce::Component
{
public:
    VocalStripModuleTile();
    void setModule(int module, const juce::String& title, int chainPosition,
                   bool enabled, bool selected);
    std::function<void(int)> onClick;
    std::function<void(int, int)> onDrop;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    int module_ = 0;
    int chainPosition_ = 1;
    juce::String title_;
    bool enabled_ = true;
    bool selected_ = false;
    bool dragging_ = false;
};

class OffSessionVocalStripAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                        private juce::Timer
{
public:
    explicit OffSessionVocalStripAudioProcessorEditor(OffSessionVocalStripAudioProcessor&);
    ~OffSessionVocalStripAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct SliderControl
    {
        int module = 0;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct ChoiceControl
    {
        int module = 0;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void selectModule(int module);
    void refreshModuleControls();
    void refreshTiles();
    void moveModule(int module, int targetSlot);
    void writeParameter(const juce::String& id, float value);
    void applyPreset(int preset);
    void timerCallback() override;
    void clearLookAndFeel();
    static juce::String peakToDb(float peak);
    static juce::String moduleEnabledId(int module);
    static juce::String moduleName(int module);
    static juce::String moduleDescription(int module);
    void configureSlider(juce::Slider&, const juce::String& suffix, const juce::String& tooltip);

    VocalStripLookAndFeel lookAndFeel_;
    OffSessionVocalStripAudioProcessor& processor_;
    std::array<VocalStripModuleTile, 8> tiles_;
    std::array<int, 8> order_ { 0, 1, 2, 3, 4, 5, 6, 7 };
    std::vector<std::unique_ptr<SliderControl>> sliders_;
    std::vector<std::unique_ptr<ChoiceControl>> choices_;

    juce::Label brand_, title_, subtitle_, selectedTitle_, selectedDescription_, status_;
    juce::Label presetLabel_, inputLabel_, outputLabel_;
    juce::Slider inputSlider_, outputSlider_;
    juce::ComboBox presetBox_;
    juce::ToggleButton moduleEnable_ { "MODULE ACTIVE" };
    juce::ToggleButton bypassButton_ { "BYPASS" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> inputAttachment_, outputAttachment_;
    std::unique_ptr<ButtonAttachment> moduleEnableAttachment_, bypassAttachment_;
    int selectedModule_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OffSessionVocalStripAudioProcessorEditor)
};
