#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
const juce::Colour bg        { 0xff08070c };
const juce::Colour chassis   { 0xff100d16 };
const juce::Colour panel     { 0xff15111d };
const juce::Colour panel2    { 0xff1c1725 };
const juce::Colour raised    { 0xff252030 };
const juce::Colour edge      { 0xff4a365e };
const juce::Colour violet    { 0xffa65cff };
const juce::Colour bright    { 0xffe6d5ff };
const juce::Colour white     { 0xfff6f0fb };
const juce::Colour muted     { 0xffa99bb8 };
const juce::Colour dim       { 0xff70667d };
const juce::Colour green     { 0xff6be0b0 };
const juce::Colour red       { 0xffff7189 };

struct SliderSpec { int module; const char* id; const char* title; const char* suffix; };
static constexpr SliderSpec sliderSpecs[] = {
    {0,"opto_peak","PEAK REDUCTION"," %"}, {0,"opto_gain","GAIN"," dB"},
    {0,"opto_emphasis","EMPHASIS"," %"}, {0,"opto_mix","MIX"," %"},

    {1,"peq_hp","HIGH PASS"," Hz"}, {1,"peq_lp","LOW PASS"," Hz"},
    {1,"peq_f1","BAND 1 FREQ"," Hz"}, {1,"peq_g1","BAND 1 GAIN"," dB"}, {1,"peq_q1","BAND 1 Q"," Q"},
    {1,"peq_f2","BAND 2 FREQ"," Hz"}, {1,"peq_g2","BAND 2 GAIN"," dB"}, {1,"peq_q2","BAND 2 Q"," Q"},
    {1,"peq_f3","BAND 3 FREQ"," Hz"}, {1,"peq_g3","BAND 3 GAIN"," dB"}, {1,"peq_q3","BAND 3 Q"," Q"},
    {1,"peq_f4","BAND 4 FREQ"," Hz"}, {1,"peq_g4","BAND 4 GAIN"," dB"}, {1,"peq_q4","BAND 4 Q"," Q"},

    {2,"deess_freq","FREQUENCY"," Hz"}, {2,"deess_threshold","THRESHOLD"," dB"},
    {2,"deess_amount","AMOUNT"," %"}, {2,"deess_attack","ATTACK"," ms"},
    {2,"deess_release","RELEASE"," ms"}, {2,"deess_range","MAX REDUCTION"," dB"},

    {3,"console_low_f","LOW FREQ"," Hz"}, {3,"console_low_g","LOW GAIN"," dB"},
    {3,"console_lm_f","LOW-MID FREQ"," Hz"}, {3,"console_lm_g","LOW-MID GAIN"," dB"},
    {3,"console_hm_f","HIGH-MID FREQ"," Hz"}, {3,"console_hm_g","HIGH-MID GAIN"," dB"},
    {3,"console_high_f","HIGH FREQ"," Hz"}, {3,"console_high_g","HIGH GAIN"," dB"},
    {3,"console_drive","CONSOLE DRIVE"," %"},

    {4,"air_mid","MID AIR"," %"}, {4,"air_high","HIGH AIR"," %"},
    {4,"air_tone","TONE"," %"}, {4,"air_trim","TRIM"," dB"},

    {5,"fet_threshold","THRESHOLD"," dB"}, {5,"fet_ratio","RATIO"," :1"},
    {5,"fet_attack","ATTACK"," ms"}, {5,"fet_release","RELEASE"," ms"},
    {5,"fet_makeup","MAKEUP"," dB"}, {5,"fet_mix","PARALLEL MIX"," %"},

    {6,"reverb_mix","MIX"," %"}, {6,"reverb_size","SIZE"," %"},
    {6,"reverb_decay","DECAY"," s"}, {6,"reverb_predelay","PRE-DELAY"," ms"},
    {6,"reverb_damping","DAMPING"," %"}, {6,"reverb_width","WIDTH"," %"},
    {6,"reverb_lowcut","LOW CUT"," Hz"}, {6,"reverb_highcut","HIGH CUT"," Hz"},

    {7,"delay_mix","MIX"," %"}, {7,"delay_time","TIME"," ms"},
    {7,"delay_feedback","FEEDBACK"," %"}, {7,"delay_duck","DUCKING"," %"},
    {7,"delay_lowcut","LOW CUT"," Hz"}, {7,"delay_highcut","HIGH CUT"," Hz"}
};

struct ChoiceSpec { int module; const char* id; const char* title; const char* const* items; int count; };
static const char* optoChoices[] = { "COMPRESS", "LIMIT" };
static const char* optoMeterChoices[] = { "GAIN REDUCTION", "OUTPUT" };
static const char* deessChoices[] = { "SPLIT BAND", "WIDE BAND" };
static const char* reverbChoices[] = { "ROOM", "PLATE", "HALL" };
static const char* delayChoices[] = { "STEREO", "PING-PONG" };
static const ChoiceSpec choiceSpecs[] = {
    {0,"opto_mode","OPTO MODE",optoChoices,2},
    {0,"opto_meter_mode","METER MODE",optoMeterChoices,2},
    {2,"deess_mode","DE-ESSER MODE",deessChoices,2},
    {6,"reverb_type","REVERB TYPE",reverbChoices,3},
    {7,"delay_mode","DELAY MODE",delayChoices,2}
};

float meterLevel(float peak)
{
    if (!std::isfinite(peak) || peak <= 0.000001f)
        return 0.0f;
    const float db = 20.0f * std::log10(peak);
    return juce::jlimit(0.0f, 1.0f, (db + 54.0f) / 54.0f);
}
}

void VocalStripLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const float size = juce::jmin(static_cast<float>(width), static_cast<float>(height)) * 0.68f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.42f;
    const float radius = size * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour(juce::Colour(0xff050409));
    g.fillEllipse(cx - radius - 3.0f, cy - radius + 4.0f, size + 6.0f, size + 6.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff51475c), cx - radius, cy - radius,
                                           juce::Colour(0xff17131e), cx + radius, cy + radius, false));
    g.fillEllipse(cx - radius, cy - radius, size, size);
    g.setColour(edge);
    g.drawEllipse(cx - radius, cy - radius, size, size, 1.2f);
    g.setColour(juce::Colour(0xff09080d));
    g.fillEllipse(cx - radius * 0.72f, cy - radius * 0.72f, size * 0.72f, size * 0.72f);
    g.setColour(violet.withAlpha(0.22f));
    g.drawArc(cx - radius - 4.0f, cy - radius - 4.0f, size + 8.0f, size + 8.0f,
              startAngle, endAngle, 2.0f);
    g.setColour(violet);
    juce::Path arc;
    arc.addArc(cx - radius - 4.0f, cy - radius - 4.0f, size + 8.0f, size + 8.0f,
               startAngle, angle, true);
    g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float pointerLength = radius * 0.66f;
    juce::Path pointer;
    pointer.startNewSubPath(cx, cy);
    pointer.lineTo(cx + std::sin(angle) * pointerLength, cy - std::cos(angle) * pointerLength);
    g.setColour(bright);
    g.strokePath(pointer, juce::PathStrokeType(2.4f, juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xffc9c1d3));
    g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

void VocalStripLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour&, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool active = button.getToggleState();
    const auto fill = active ? juce::Colour(0xff38234f) : panel2;
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(active || highlighted ? violet : edge.withAlpha(0.65f));
    g.drawRoundedRectangle(bounds, 7.0f, active ? 1.7f : 1.0f);
    if (down)
    {
        g.setColour(violet.withAlpha(0.12f));
        g.fillRoundedRectangle(bounds, 7.0f);
    }
}

VocalStripModuleTile::VocalStripModuleTile()
{
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void VocalStripModuleTile::setModule(int module, const juce::String& title, int chainPosition,
                                     bool enabled, bool selected)
{
    module_ = module;
    title_ = title;
    chainPosition_ = chainPosition;
    enabled_ = enabled;
    selected_ = selected;
    repaint();
}

void VocalStripModuleTile::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff07060a));
    g.fillRoundedRectangle(r.translated(0.0f, 3.0f), 10.0f);
    g.setColour(selected_ ? juce::Colour(0xff241832) : panel);
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(selected_ ? violet : edge.withAlpha(0.7f));
    g.drawRoundedRectangle(r, 10.0f, selected_ ? 2.0f : 1.0f);
    auto stripe = r.removeFromTop(4.0f);
    g.setColour(selected_ ? violet : edge);
    g.fillRoundedRectangle(stripe, 2.0f);

    g.setColour(selected_ ? bright : violet);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText(juce::String(chainPosition_).paddedLeft('0', 2), 10, 11, 28, 18, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
    g.drawText("MODULE", 39, 11, getWidth() - 50, 18, juce::Justification::centredLeft);

    g.setColour(white);
    g.setFont(juce::Font(juce::FontOptions(juce::jmin(13.0f, getWidth() * 0.082f), juce::Font::bold)));
    g.drawFittedText(title_, 10, 35, getWidth() - 20, 28, juce::Justification::centredLeft, 1);
    g.setColour(enabled_ ? green : dim);
    g.fillEllipse(11.0f, static_cast<float>(getHeight() - 20), 7.0f, 7.0f);
    g.setColour(enabled_ ? green : dim);
    g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
    g.drawText(enabled_ ? "ACTIVE" : "BYPASSED", 24, getHeight() - 23, getWidth() - 30, 14,
               juce::Justification::centredLeft);
    g.setColour(dim);
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText("DRAG TO MOVE", 10, getHeight() - 42, getWidth() - 20, 12,
               juce::Justification::centredRight);
}

void VocalStripModuleTile::mouseDown(const juce::MouseEvent&)
{
    dragging_ = false;
}

void VocalStripModuleTile::mouseDrag(const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() > 7)
        dragging_ = true;
}

void VocalStripModuleTile::mouseUp(const juce::MouseEvent& e)
{
    if (dragging_)
    {
        if (onDrop != nullptr && getParentComponent() != nullptr)
        {
            const auto point = getParentComponent()->getLocalPoint(this, e.getPosition());
            onDrop(module_, point.x);
        }
    }
    else if (onClick != nullptr)
        onClick(module_);
    dragging_ = false;
}

OffSessionVocalStripAudioProcessorEditor::OffSessionVocalStripAudioProcessorEditor(
    OffSessionVocalStripAudioProcessor& processor)
    : AudioProcessorEditor(&processor), processor_(processor)
{
    setLookAndFeel(&lookAndFeel_);
    setSize(1440, 930);
    setResizable(true, true);
    setResizeLimits(1200, 780, 1900, 1200);

    brand_.setText("OFF SESSION  /  NYXV", juce::dontSendNotification);
    brand_.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    brand_.setColour(juce::Label::textColourId, violet);
    addAndMakeVisible(brand_);

    title_.setText("VOCAL STRIP", juce::dontSendNotification);
    title_.setFont(juce::Font(juce::FontOptions(31.0f, juce::Font::bold)));
    title_.setColour(juce::Label::textColourId, white);
    addAndMakeVisible(title_);

    subtitle_.setText("ANALOG CHARACTER  /  DIGITAL PRECISION  /  ZERO-LOOKAHEAD ENGINE", juce::dontSendNotification);
    subtitle_.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    subtitle_.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(subtitle_);

    presetLabel_.setText("PRESET", juce::dontSendNotification);
    presetLabel_.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
    presetLabel_.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(presetLabel_);
    presetBox_.addItem("LEAD VOCAL  /  NYXV", 1);
    presetBox_.addItem("R&B AIR", 2);
    presetBox_.addItem("TRAP FORWARD", 3);
    presetBox_.addItem("INTIMATE", 4);
    presetBox_.addItem("AD-LIB SPACE", 5);
    presetBox_.setSelectedId(1, juce::dontSendNotification);
    presetBox_.setColour(juce::ComboBox::backgroundColourId, panel2);
    presetBox_.setColour(juce::ComboBox::textColourId, white);
    presetBox_.setColour(juce::ComboBox::outlineColourId, edge);
    presetBox_.onChange = [this] { if (presetBox_.getSelectedId() > 0) applyPreset(presetBox_.getSelectedId() - 1); };
    addAndMakeVisible(presetBox_);

    auto& apvts = processor_.parameters;
    inputLabel_.setText("INPUT", juce::dontSendNotification);
    outputLabel_.setText("OUTPUT", juce::dontSendNotification);
    for (auto* label : { &inputLabel_, &outputLabel_ })
    {
        label->setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
        label->setColour(juce::Label::textColourId, muted);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*label);
    }
    configureSlider(inputSlider_, " dB", "Global input trim before the module chain.");
    configureSlider(outputSlider_, " dB", "Global output trim after the module chain.");
    inputSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    addAndMakeVisible(inputSlider_);
    addAndMakeVisible(outputSlider_);
    inputAttachment_ = std::make_unique<SliderAttachment>(apvts, "input_db", inputSlider_);
    outputAttachment_ = std::make_unique<SliderAttachment>(apvts, "output_db", outputSlider_);

    bypassButton_.setLookAndFeel(&lookAndFeel_);
    bypassButton_.setColour(juce::ToggleButton::textColourId, bright);
    addAndMakeVisible(bypassButton_);
    bypassAttachment_ = std::make_unique<ButtonAttachment>(apvts, "global_bypass", bypassButton_);

    selectedTitle_.setFont(juce::Font(juce::FontOptions(19.0f, juce::Font::bold)));
    selectedTitle_.setColour(juce::Label::textColourId, white);
    addAndMakeVisible(selectedTitle_);
    selectedDescription_.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    selectedDescription_.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(selectedDescription_);

    moduleEnable_.setLookAndFeel(&lookAndFeel_);
    moduleEnable_.setColour(juce::ToggleButton::textColourId, bright);
    addAndMakeVisible(moduleEnable_);

    for (const auto& spec : sliderSpecs)
    {
        auto control = std::make_unique<SliderControl>();
        control->module = spec.module;
        control->slider = std::make_unique<juce::Slider>();
        control->label = std::make_unique<juce::Label>();
        configureSlider(*control->slider, spec.suffix, juce::String(spec.title) + " control.");
        control->label->setText(spec.title, juce::dontSendNotification);
        control->label->setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
        control->label->setColour(juce::Label::textColourId, muted);
        control->label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*control->label);
        addAndMakeVisible(*control->slider);
        control->attachment = std::make_unique<SliderAttachment>(apvts, spec.id, *control->slider);
        sliders_.push_back(std::move(control));
    }

    for (const auto& spec : choiceSpecs)
    {
        auto control = std::make_unique<ChoiceControl>();
        control->module = spec.module;
        control->combo = std::make_unique<juce::ComboBox>();
        control->label = std::make_unique<juce::Label>();
        for (int i = 0; i < spec.count; ++i)
            control->combo->addItem(spec.items[i], i + 1);
        control->combo->setColour(juce::ComboBox::backgroundColourId, raised);
        control->combo->setColour(juce::ComboBox::textColourId, bright);
        control->combo->setColour(juce::ComboBox::outlineColourId, edge);
        control->label->setText(spec.title, juce::dontSendNotification);
        control->label->setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
        control->label->setColour(juce::Label::textColourId, muted);
        control->label->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(*control->label);
        addAndMakeVisible(*control->combo);
        control->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, spec.id, *control->combo);
        choices_.push_back(std::move(control));
    }

    status_.setText("READY  /  DRAG MODULES TO REORDER  /  SELECT A MODULE TO EDIT", juce::dontSendNotification);
    status_.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
    status_.setColour(juce::Label::textColourId, violet);
    addAndMakeVisible(status_);

    for (int i = 0; i < 8; ++i)
    {
        auto& tile = tiles_[static_cast<std::size_t>(i)];
        tile.onClick = [this](int module) { selectModule(module); };
        tile.onDrop = [this](int module, int x) {
            const int margin = static_cast<int>(24.0f * getWidth() / 1440.0f);
            const int gap = static_cast<int>(9.0f * getWidth() / 1440.0f);
            const int tileWidth = (getWidth() - margin * 2 - gap * 7) / 8;
            const int slot = juce::jlimit(0, 7, (x - margin) / juce::jmax(1, tileWidth + gap));
            moveModule(module, slot);
        };
        addAndMakeVisible(tile);
    }

    const auto* raw = apvts.getRawParameterValue("slot0");
    if (raw != nullptr)
    {
        std::array<bool, 8> used {};
        int next = 0;
        for (int i = 0; i < 8; ++i)
        {
            const auto id = "slot" + juce::String(i);
            const auto* v = apvts.getRawParameterValue(id);
            const int candidate = v != nullptr ? juce::jlimit(0, 7, static_cast<int>(std::round(v->load()))) : i;
            if (!used[static_cast<std::size_t>(candidate)])
            {
                order_[static_cast<std::size_t>(i)] = candidate;
                used[static_cast<std::size_t>(candidate)] = true;
            }
            else
            {
                while (next < 8 && used[static_cast<std::size_t>(next)]) ++next;
                order_[static_cast<std::size_t>(i)] = juce::jmin(7, next);
                if (next < 8) used[static_cast<std::size_t>(next)] = true;
            }
        }
    }
    refreshTiles();
    selectModule(0);
    startTimerHz(30);
}

OffSessionVocalStripAudioProcessorEditor::~OffSessionVocalStripAudioProcessorEditor()
{
    stopTimer();
    clearLookAndFeel();
    setLookAndFeel(nullptr);
}

void OffSessionVocalStripAudioProcessorEditor::configureSlider(juce::Slider& slider,
    const juce::String& suffix, const juce::String& tooltip)
{
    slider.setLookAndFeel(&lookAndFeel_);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 94, 19);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::textBoxTextColourId, white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, raised);
    slider.setColour(juce::Slider::textBoxOutlineColourId, edge);
    slider.setTooltip(tooltip + " Double-click to reset to the parameter default.");
}

void OffSessionVocalStripAudioProcessorEditor::clearLookAndFeel()
{
    for (auto& tile : tiles_)
        tile.setLookAndFeel(nullptr);
    inputSlider_.setLookAndFeel(nullptr);
    outputSlider_.setLookAndFeel(nullptr);
    bypassButton_.setLookAndFeel(nullptr);
    moduleEnable_.setLookAndFeel(nullptr);
    for (auto& control : sliders_)
        control->slider->setLookAndFeel(nullptr);
}

void OffSessionVocalStripAudioProcessorEditor::selectModule(int module)
{
    selectedModule_ = juce::jlimit(0, 7, module);
    moduleEnableAttachment_.reset();
    moduleEnableAttachment_ = std::make_unique<ButtonAttachment>(processor_.parameters,
        moduleEnabledId(selectedModule_), moduleEnable_);
    selectedTitle_.setText(juce::String(selectedModule_ + 1).paddedLeft('0', 2) + "  /  " + moduleName(selectedModule_),
                           juce::dontSendNotification);
    selectedDescription_.setText(moduleDescription(selectedModule_), juce::dontSendNotification);
    refreshModuleControls();
    refreshTiles();
    status_.setText("EDITING  /  " + moduleName(selectedModule_), juce::dontSendNotification);
}

void OffSessionVocalStripAudioProcessorEditor::refreshModuleControls()
{
    for (auto& control : sliders_)
    {
        const bool show = control->module == selectedModule_;
        control->slider->setVisible(show);
        control->label->setVisible(show);
    }
    for (auto& control : choices_)
    {
        const bool show = control->module == selectedModule_;
        control->combo->setVisible(show);
        control->label->setVisible(show);
    }
    resized();
}

void OffSessionVocalStripAudioProcessorEditor::refreshTiles()
{
    for (int slot = 0; slot < 8; ++slot)
    {
        const int module = order_[static_cast<std::size_t>(slot)];
        const auto id = moduleEnabledId(module);
        const auto* raw = processor_.parameters.getRawParameterValue(id);
        const bool enabled = raw == nullptr || raw->load(std::memory_order_relaxed) >= 0.5f;
        tiles_[static_cast<std::size_t>(module)].setModule(module, moduleName(module), slot + 1,
                                                          enabled, module == selectedModule_);
        tiles_[static_cast<std::size_t>(module)].toFront(false);
    }
    repaint();
}

void OffSessionVocalStripAudioProcessorEditor::moveModule(int module, int targetSlot)
{
    targetSlot = juce::jlimit(0, 7, targetSlot);
    int sourceSlot = -1;
    for (int i = 0; i < 8; ++i)
        if (order_[static_cast<std::size_t>(i)] == module)
            sourceSlot = i;
    if (sourceSlot < 0 || sourceSlot == targetSlot)
        return;

    const int moving = order_[static_cast<std::size_t>(sourceSlot)];
    if (sourceSlot < targetSlot)
        for (int i = sourceSlot; i < targetSlot; ++i)
            order_[static_cast<std::size_t>(i)] = order_[static_cast<std::size_t>(i + 1)];
    else
        for (int i = sourceSlot; i > targetSlot; --i)
            order_[static_cast<std::size_t>(i)] = order_[static_cast<std::size_t>(i - 1)];
    order_[static_cast<std::size_t>(targetSlot)] = moving;

    for (int i = 0; i < 8; ++i)
        writeParameter("slot" + juce::String(i), static_cast<float>(order_[static_cast<std::size_t>(i)]));
    status_.setText("CHAIN UPDATED  /  " + moduleName(module) + " MOVED TO POSITION " + juce::String(targetSlot + 1),
                    juce::dontSendNotification);
    resized();
    refreshTiles();
}

void OffSessionVocalStripAudioProcessorEditor::writeParameter(const juce::String& id, float value)
{
    if (auto* parameter = processor_.parameters.getParameter(id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
}

void OffSessionVocalStripAudioProcessorEditor::applyPreset(int preset)
{
    // Start every preset from a complete, deterministic baseline so values from a previous
    // preset or a hand-edited session can never leak into the next factory starting point.
    static constexpr std::pair<const char*, float> defaults[] = {
        {"opto_peak",42.0f}, {"opto_gain",2.0f}, {"opto_emphasis",25.0f}, {"opto_mix",100.0f},
        {"opto_mode",0.0f}, {"opto_meter_mode",0.0f},
        {"peq_hp",45.0f}, {"peq_lp",20000.0f},
        {"peq_f1",180.0f}, {"peq_g1",0.0f}, {"peq_q1",0.8f},
        {"peq_f2",450.0f}, {"peq_g2",0.0f}, {"peq_q2",0.8f},
        {"peq_f3",2800.0f}, {"peq_g3",0.0f}, {"peq_q3",0.8f},
        {"peq_f4",9500.0f}, {"peq_g4",0.0f}, {"peq_q4",0.8f},
        {"deess_freq",6500.0f}, {"deess_threshold",-24.0f}, {"deess_amount",55.0f},
        {"deess_attack",1.5f}, {"deess_release",80.0f}, {"deess_range",8.0f}, {"deess_mode",0.0f},
        {"console_low_f",100.0f}, {"console_low_g",0.0f}, {"console_lm_f",300.0f}, {"console_lm_g",0.0f},
        {"console_hm_f",3000.0f}, {"console_hm_g",0.0f}, {"console_high_f",10000.0f},
        {"console_high_g",0.0f}, {"console_drive",8.0f},
        {"air_mid",22.0f}, {"air_high",26.0f}, {"air_tone",55.0f}, {"air_trim",0.0f},
        {"fet_threshold",-18.0f}, {"fet_ratio",4.0f}, {"fet_attack",2.0f},
        {"fet_release",80.0f}, {"fet_makeup",0.0f}, {"fet_mix",100.0f},
        {"reverb_mix",12.0f}, {"reverb_size",42.0f}, {"reverb_decay",1.6f},
        {"reverb_predelay",20.0f}, {"reverb_damping",50.0f}, {"reverb_width",100.0f},
        {"reverb_lowcut",180.0f}, {"reverb_highcut",12000.0f}, {"reverb_type",1.0f},
        {"delay_mix",0.0f}, {"delay_time",350.0f}, {"delay_feedback",25.0f},
        {"delay_duck",0.0f}, {"delay_lowcut",150.0f}, {"delay_highcut",10000.0f}, {"delay_mode",0.0f}
    };
    for (const auto& item : defaults)
        writeParameter(item.first, item.second);

    // Factory starting points only; every exposed value remains editable and host-automatable.
    if (preset == 0) // Lead Vocal / NYXV
    {
        writeParameter("opto_peak", 42); writeParameter("opto_gain", 2); writeParameter("opto_emphasis", 24);
        writeParameter("opto_mix", 100); writeParameter("opto_mode", 0);
        writeParameter("peq_hp", 75); writeParameter("peq_lp", 19000);
        writeParameter("peq_f1", 180); writeParameter("peq_g1", -1.5f); writeParameter("peq_f2", 420);
        writeParameter("peq_g2", -1.0f); writeParameter("peq_f3", 3200); writeParameter("peq_g3", 1.5f);
        writeParameter("peq_f4", 10500); writeParameter("peq_g4", 1.0f);
        writeParameter("deess_threshold", -25); writeParameter("deess_amount", 52);
        writeParameter("console_low_g", 0.5f); writeParameter("console_hm_g", 1.5f);
        writeParameter("console_high_g", 1.0f); writeParameter("air_mid", 22); writeParameter("air_high", 25);
        writeParameter("fet_threshold", -18); writeParameter("fet_ratio", 4); writeParameter("fet_attack", 2);
        writeParameter("fet_release", 85); writeParameter("fet_mix", 100);
        writeParameter("reverb_mix", 9); writeParameter("reverb_size", 40); writeParameter("reverb_decay", 1.4f);
        writeParameter("delay_mix", 4); writeParameter("delay_time", 360); writeParameter("delay_feedback", 20);
    }
    else if (preset == 1) // R&B Air
    {
        writeParameter("opto_peak", 36); writeParameter("opto_gain", 1.5f); writeParameter("opto_emphasis", 38);
        writeParameter("peq_hp", 65); writeParameter("peq_g1", -1); writeParameter("peq_g3", 1.0f);
        writeParameter("deess_threshold", -27); writeParameter("deess_amount", 62);
        writeParameter("air_mid", 38); writeParameter("air_high", 48); writeParameter("air_tone", 68);
        writeParameter("fet_threshold", -22); writeParameter("fet_ratio", 3); writeParameter("fet_attack", 8);
        writeParameter("fet_release", 130); writeParameter("reverb_mix", 15); writeParameter("reverb_decay", 2.2f);
        writeParameter("delay_mix", 7); writeParameter("delay_feedback", 24);
    }
    else if (preset == 2) // Trap Forward
    {
        writeParameter("opto_peak", 55); writeParameter("opto_gain", 3); writeParameter("opto_mode", 0);
        writeParameter("peq_hp", 90); writeParameter("peq_f2", 350); writeParameter("peq_g2", -2.5f);
        writeParameter("peq_f3", 4200); writeParameter("peq_g3", 2.0f);
        writeParameter("deess_threshold", -25); writeParameter("deess_amount", 50);
        writeParameter("air_mid", 16); writeParameter("air_high", 31);
        writeParameter("fet_threshold", -24); writeParameter("fet_ratio", 6); writeParameter("fet_attack", 1);
        writeParameter("fet_release", 65); writeParameter("fet_makeup", 2);
        writeParameter("reverb_mix", 6); writeParameter("delay_mix", 5); writeParameter("delay_mode", 1);
    }
    else if (preset == 3) // Intimate
    {
        writeParameter("opto_peak", 34); writeParameter("opto_gain", 1); writeParameter("peq_hp", 55);
        writeParameter("peq_g2", -1); writeParameter("peq_g3", 0.5f); writeParameter("air_mid", 12);
        writeParameter("air_high", 10); writeParameter("fet_threshold", -20); writeParameter("fet_ratio", 2.8f);
        writeParameter("reverb_mix", 5); writeParameter("reverb_size", 24); writeParameter("delay_mix", 0);
    }
    else // Ad-Lib Space
    {
        writeParameter("opto_peak", 48); writeParameter("opto_gain", 2.5f); writeParameter("peq_hp", 110);
        writeParameter("air_mid", 28); writeParameter("air_high", 40); writeParameter("fet_threshold", -22);
        writeParameter("fet_ratio", 4.5f); writeParameter("reverb_mix", 28); writeParameter("reverb_size", 72);
        writeParameter("reverb_decay", 3.8f); writeParameter("reverb_type", 2);
        writeParameter("delay_mix", 22); writeParameter("delay_time", 430); writeParameter("delay_feedback", 38);
        writeParameter("delay_mode", 1); writeParameter("delay_duck", 35);
    }
    status_.setText("FACTORY STARTING POINT LOADED  /  " + presetBox_.getText(), juce::dontSendNotification);
}

void OffSessionVocalStripAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto outer = getLocalBounds().toFloat().reduced(8.0f);
    g.setColour(chassis);
    g.fillRoundedRectangle(outer, 17.0f);
    g.setColour(edge.withAlpha(0.8f));
    g.drawRoundedRectangle(outer, 17.0f, 1.2f);

    const float s = static_cast<float>(getWidth()) / 1440.0f;
    auto box = [s](float x, float y, float w, float h) { return juce::Rectangle<float>(x*s, y*s, w*s, h*s); };

    g.setColour(panel);
    g.fillRoundedRectangle(box(20, 15, 1400, 103), 12.0f*s);
    g.setColour(edge);
    g.drawRoundedRectangle(box(20, 15, 1400, 103), 12.0f*s, 1.0f);
    g.setColour(violet.withAlpha(0.18f));
    g.fillRoundedRectangle(box(20, 15, 5, 103), 2.0f*s);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(8.5f*s, juce::Font::bold)));
    g.drawText("DRAG MODULES TO REORDER  /  SELECT ONE TO EDIT", box(24, 120, 600, 15), juce::Justification::centredLeft);

    g.setColour(panel);
    g.fillRoundedRectangle(box(20, 255, 1400, 620), 13.0f*s);
    g.setColour(edge);
    g.drawRoundedRectangle(box(20, 255, 1400, 620), 13.0f*s, 1.0f);
    g.setColour(violet.withAlpha(0.08f));
    g.fillRoundedRectangle(box(21, 255, 1398, 60), 12.0f*s);

    g.setColour(violet);
    g.setFont(juce::Font(juce::FontOptions(8.0f*s, juce::Font::bold)));
    g.drawText("SELECTED PROCESSOR", box(42, 276, 250, 13), juce::Justification::centredLeft);

    // Analogue-inspired input/output meters with live values from the DSP callback.
    auto drawMeter = [&g, s](float x, float peak, const juce::String& label)
    {
        const auto bounds = juce::Rectangle<float>(x*s, 40.0f*s, 90.0f*s, 13.0f*s);
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(7.5f*s, juce::Font::bold)));
        g.drawText(label, x*s, 22.0f*s, 90.0f*s, 12.0f*s, juce::Justification::centred);
        g.setColour(juce::Colour(0xff08070b));
        g.fillRoundedRectangle(bounds, 4.0f*s);
        const float level = meterLevel(peak);
        g.setColour(level > 0.92f ? red : green);
        g.fillRoundedRectangle(bounds.withWidth(bounds.getWidth() * level), 4.0f*s);
        g.setColour(edge);
        g.drawRoundedRectangle(bounds, 4.0f*s, 0.8f*s);
        g.setColour(bright);
        const auto db = peak <= 0.000001f ? juce::String("-INF") : juce::String(20.0f*std::log10(peak), 1);
        g.setFont(juce::Font(juce::FontOptions(7.5f*s, juce::Font::bold)));
        g.drawText(db + " dB", x*s, 57.0f*s, 90.0f*s, 12.0f*s, juce::Justification::centred);
    };
    drawMeter(1160.0f, processor_.inputPeak(), "INPUT PEAK");
    drawMeter(1260.0f, processor_.outputPeak(), "OUTPUT PEAK");

    if (selectedModule_ == 0)
    {
        const auto* meterMode = processor_.parameters.getRawParameterValue("opto_meter_mode");
        const bool showOutput = meterMode != nullptr && meterMode->load(std::memory_order_relaxed) >= 0.5f;
        const float valueDb = showOutput ? peakToDb(processor_.outputPeak()) : -processor_.optoGainReductionDb();
        const float meterFill = showOutput ? meterLevel(processor_.outputPeak())
                                           : juce::jlimit(0.0f, 1.0f, processor_.optoGainReductionDb() / 18.0f);
        const auto meterBounds = box(965, 283, 235, 52);
        g.setColour(juce::Colour(0xff0a080e));
        g.fillRoundedRectangle(meterBounds, 7.0f*s);
        g.setColour(edge);
        g.drawRoundedRectangle(meterBounds, 7.0f*s, 1.0f*s);
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(7.5f*s, juce::Font::bold)));
        g.drawText(showOutput ? "OPTO OUTPUT" : "OPTO GAIN REDUCTION", box(976, 288, 140, 12), juce::Justification::centredLeft);
        const auto bar = box(976, 306, 195, 8);
        g.setColour(juce::Colour(0xff050408));
        g.fillRoundedRectangle(bar, 3.0f*s);
        g.setColour(violet);
        g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * meterFill), 3.0f*s);
        g.setColour(bright);
        g.drawText(juce::String(valueDb, 1) + " dB", box(1110, 286, 78, 15), juce::Justification::centredRight);
    }

    g.setColour(edge.withAlpha(0.55f));
    g.drawLine(24.0f*s, 890.0f*s, 1416.0f*s, 890.0f*s, 1.0f*s);
    g.setColour(violet);
    g.setFont(juce::Font(juce::FontOptions(8.5f*s, juce::Font::bold)));
    g.drawText("0 SAMPLES REPORTED  /  NO LOOK-AHEAD  /  DRY PATH NOT DELAYED",
               box(28, 895, 900, 18), juce::Justification::centredLeft);
    g.setColour(dim);
    g.drawText("NYXV  ×  OFF SESSION AUDIO  ·  VST3", box(1030, 895, 365, 18), juce::Justification::centredRight);
}

void OffSessionVocalStripAudioProcessorEditor::resized()
{
    const float s = static_cast<float>(getWidth()) / 1440.0f;
    auto S = [s](float v) { return static_cast<int>(std::round(v * s)); };

    brand_.setBounds(S(35), S(23), S(300), S(18));
    title_.setBounds(S(32), S(41), S(360), S(42));
    subtitle_.setBounds(S(35), S(82), S(560), S(18));
    presetLabel_.setBounds(S(520), S(28), S(250), S(14));
    presetBox_.setBounds(S(520), S(49), S(280), S(34));
    inputLabel_.setBounds(S(830), S(21), S(84), S(13));
    inputSlider_.setBounds(S(830), S(34), S(84), S(72));
    outputLabel_.setBounds(S(950), S(21), S(84), S(13));
    outputSlider_.setBounds(S(950), S(34), S(84), S(72));
    bypassButton_.setBounds(S(1350), S(43), S(58), S(36));

    const int margin = S(24), gap = S(9), tileW = (getWidth() - margin * 2 - gap * 7) / 8;
    for (int slot = 0; slot < 8; ++slot)
    {
        const int module = order_[static_cast<std::size_t>(slot)];
        tiles_[static_cast<std::size_t>(module)].setBounds(margin + slot * (tileW + gap), S(140), tileW, S(96));
    }

    selectedTitle_.setBounds(S(42), S(292), S(650), S(28));
    selectedDescription_.setBounds(S(44), S(322), S(820), S(17));
    moduleEnable_.setBounds(S(1245), S(294), S(150), S(30));

    const int left = S(35), top = S(385), cellW = (getWidth() - S(70)) / 4, cellH = S(110);
    int indexInModule = 0;
    for (auto& control : sliders_)
    {
        if (control->module != selectedModule_)
            continue;
        const int row = indexInModule / 4;
        const int col = indexInModule % 4;
        const int x = left + col * cellW;
        const int y = top + row * cellH;
        control->label->setBounds(x + S(8), y, cellW - S(16), S(18));
        control->slider->setBounds(x + (cellW - S(148)) / 2, y + S(15), S(148), S(94));
        ++indexInModule;
    }

    int choiceIndex = 0;
    for (auto& control : choices_)
    {
        if (control->module != selectedModule_)
            continue;
        const int labelX = selectedModule_ == 0 && choiceIndex == 1 ? S(410) : S(42);
        const int comboX = selectedModule_ == 0 && choiceIndex == 1 ? S(520) : S(160);
        control->label->setBounds(labelX, S(343), S(108), S(24));
        control->combo->setBounds(comboX, S(343), S(190), S(25));
        ++choiceIndex;
    }

    status_.setBounds(S(42), S(856), S(1000), S(16));
}

void OffSessionVocalStripAudioProcessorEditor::timerCallback()
{
    refreshTiles();
    repaint();
}

juce::String OffSessionVocalStripAudioProcessorEditor::peakToDb(float peak)
{
    if (!std::isfinite(peak) || peak <= 0.000001f)
        return "-INF";
    return juce::String(20.0f * std::log10(peak), 1);
}

juce::String OffSessionVocalStripAudioProcessorEditor::moduleEnabledId(int module)
{
    static const char* ids[] = { "enable_opto", "enable_peq", "enable_deess", "enable_console",
        "enable_air", "enable_fet", "enable_reverb", "enable_delay" };
    return ids[juce::jlimit(0, 7, module)];
}

juce::String OffSessionVocalStripAudioProcessorEditor::moduleName(int module)
{
    static const char* names[] = { "OPTO LEVELER", "PARAMETRIC EQ", "DE-ESSER", "CONSOLE EQ",
        "AIR ENHANCER", "FET COMPRESSOR", "REVERB", "DELAY" };
    return names[juce::jlimit(0, 7, module)];
}

juce::String OffSessionVocalStripAudioProcessorEditor::moduleDescription(int module)
{
    static const char* descriptions[] = {
        "CLASSIC OPTICAL LEVELING  /  PEAK REDUCTION, GAIN, EMPHASIS, COMPRESS OR LIMIT",
        "FOUR FULLY PARAMETRIC BANDS  /  HIGH-PASS AND LOW-PASS FILTERS",
        "ADAPTIVE SIBILANCE CONTROL  /  SPLIT-BAND OR WIDE-BAND DETECTION",
        "FOUR MUSICAL CONSOLE BANDS  /  LOW, LOW-MID, HIGH-MID, HIGH AND DRIVE",
        "ORIGINAL HARMONIC PRESENCE ENHANCER  /  MID AIR, HIGH AIR, TONE AND TRIM",
        "FAST FET-STYLE DYNAMICS  /  ATTACK, RELEASE, RATIO AND PARALLEL MIX",
        "ALGORITHMIC ROOM, PLATE OR HALL  /  PRE-DELAY, DAMPING, WIDTH AND FILTERS",
        "STEREO OR PING-PONG ECHO  /  FEEDBACK FILTERS AND VOCAL DUCKING"
    };
    return descriptions[juce::jlimit(0, 7, module)];
}
