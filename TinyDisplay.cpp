#include <juce/juce.h>
#include "MidiPlayerEngine.h"

class TinyDisplay : public Component,
                    public ButtonListener,
                    public SliderListener,
                    public ChangeListener
{
public:
    TinyDisplay() : mPlayBtn(T("Play")),
                    mStopBtn(T("Stop")),
                    mOpenBtn(T("Open")),
                    mSlider (T("Seek"))
    {
        mPlayBtn.setBounds(10, 10, 50, 30);
        mPlayBtn.addButtonListener(this);
        mStopBtn.setBounds(70, 10, 50, 30);
        mStopBtn.addButtonListener(this);
        mOpenBtn.setBounds(130, 10, 50, 30);
        mOpenBtn.addButtonListener(this);
        
        mSlider.setBounds(10, 50, 180, 30);
        mSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
        mSlider.addListener(this);
        
        addAndMakeVisible(&mPlayBtn);
        addAndMakeVisible(&mStopBtn);
        addAndMakeVisible(&mOpenBtn);
        addAndMakeVisible(&mSlider);
        
        mSlider.setEnabled(false);
        mStopBtn.setEnabled(false);
        mPlayBtn.setEnabled(false);
        
        midiOut = MidiOutput::createNewDevice(JUCEApplication::getInstance()->getApplicationName());
        engine  = 0;
    }
    
    void sliderValueChanged(Slider* slider) {
        if (engine)
            engine->seek(slider->getValue());
    }
    
    void buttonClicked(Button* button) {
        if (button == (Button*)&mOpenBtn) {
            FileChooser fc(T("Open MIDI File"),
                           File::getSpecialLocation(File::userMusicDirectory),
                           "*.mid;*.kar");
            if (!fc.browseForFileToOpen())
                return;
            
            if (engine) {
                engine->stop();
                delete engine;
            }
            
            engine = new MidiPlayerEngine(File(fc.getResult()).createInputStream(), midiOut);
            engine->addChangeListener(this);
            mPlayBtn.setEnabled(true);
            mPlayBtn.setToggleState(false, false);
            mStopBtn.setEnabled(true);
            mSlider.setEnabled(true);
            mSlider.setRange(0, engine->getLength());
            Logger::writeToLog(String("Total length = ") + String(engine->getLength()));
            
        } else if (button == (Button*)&mStopBtn) {
            engine->stop();
            mPlayBtn.setToggleState(false, false);
            mSlider.setValue(0, false);
            mSlider.setEnabled(false);
            
        } else if (button == (Button*)&mPlayBtn) {
            if (mPlayBtn.getToggleState())
                engine->play();
            else
                engine->pause();
                engine->reset();
        }
    }
    
    void changeListenerCallback(void* obj) {
        MidiPlayerEngine* k = (MidiPlayerEngine*)obj;
        mSlider.setValue(k->getSeekPosition());
        Logger::writeToLog(String("Position ") + String(k->getSeekPosition()));
        repaint();
    }

protected:
    ToggleButton    mPlayBtn;
    TextButton      mStopBtn;
    TextButton      mOpenBtn;
    Slider          mSlider;
    MidiPlayerEngine* engine;
    MidiOutput*     midiOut;
};
