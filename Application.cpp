#include <juce/juce.h>
#include "MidiPlayerEngine.h"
//#include "PrelimDisplay.cpp"
#include "TinyDisplay.cpp"

class BrusselsMidiPlayer : public JUCEApplication {
public:
    void initialise(const String& p) {
        TinyDisplay k;
        k.setBounds(0, 0, 200, 90);
        //Process::setPriority(Process::HighPriority);
        // the above seems to choke out Jack, so leaing it at normal priority
        DialogWindow::showModalDialog(T("Brussels Midi Player"), &k, 0,
                                      Colours::silver, false);
        systemRequestedQuit();
    }

    void shutdown() {
    }

    void systemRequestedQuit() {
        /*if (AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon,
                                         T("Quit?"), T("Do you wish to quit ") 
                                         + getApplicationName(),
                                         T("Yes"), T("No")))*/
            quit();
    }

    const String getApplicationName() {
        return String(T("Brussels Midi Player"));
    }
};

START_JUCE_APPLICATION(BrusselsMidiPlayer)
