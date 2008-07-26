#include <juce/juce.h>
#include <exception>
#include "MidiPlayerEngine.h"

MidiPlayerEngine::~MidiPlayerEngine() {
    // Release references and delete objects
    if (mListener) mListener = 0;
    if (mMidiOut) mMidiOut = 0;
    waitForThreadToExit(4000);
}

MidiPlayerEngine::MidiPlayerEngine
    (InputStream* stream, MidiOutput* midiDevice)
    : Thread(JUCEApplication::getInstance()->getApplicationName() + T(" MIDI"))
{
    bool readSuccessfully = mFile.readFrom(*stream);
    if (!readSuccessfully) throw String("Midi file appears corrupted");
    if (mFile.getTimeFormat() <= 0) // SMPTE format file, not yet supported
        throw String("SMPTE format timing is not yet supported");

    mBuffer.clear();
    mPosition = 0;
    mListener = 0; // Initially no listener
    mFilter   = 0;
    const MidiMessageSequence* oneTrack;

    // Start mixing
    for (int track = 0; track < mFile.getNumTracks(); track++) {
        oneTrack = mFile.getTrack(track);
        mBuffer.addSequence(*oneTrack,
                            0, // don't add any offset to timestamp
                            0, // use events from the beginning
                            oneTrack->getEndTime() /* to the end of track*/);
    }
    // update note-offs
    mBuffer.updateMatchedPairs();

    // Loading and mixing done, now set the MIDI device
    if (midiDevice)
        mMidiOut = midiDevice;
    else // null pointer
        throw String("Invalid MIDI output device specified");

    // All done.
    this->sendActionMessage("FileLoaded");
}

void MidiPlayerEngine::setMessageListener(MessageListener* listener) {
    mListener = listener;
}

void MidiPlayerEngine::setFilterCallback(MidiPlayerEngine::MessageFilter* m) {
    mFilter = m;
}

double MidiPlayerEngine::getLength() {
    return mBuffer.getEndTime();
}

double MidiPlayerEngine::getSeekPosition() {
    MidiMessage m = mBuffer.getEventPointer(getPosition())->message;
    return m.getTimeStamp();
}

void MidiPlayerEngine::play() {
    // Let there be music
    const ScopedWriteLock lock(mPauseLock);
    mIsPaused = false; // undo any previous pausing
    startThread(8); // with thread priority
}

void MidiPlayerEngine::pause() {
    const ScopedWriteLock lock(mPauseLock);
    mIsPaused = true;
    sendActionMessage("Pause");
}

bool MidiPlayerEngine::isPaused() {
    const ScopedReadLock lock(mPauseLock);
    return mIsPaused;
}

void MidiPlayerEngine::stop() {
    signalThreadShouldExit();
    waitForThreadToExit(4000);
    sendActionMessage("Stop");
}

void MidiPlayerEngine::seek(const int beatNumber) {
    const ScopedWriteLock lock(mPositionLock);
    const int TPQN = mFile.getTimeFormat();
    if ((beatNumber * TPQN) > mBuffer.getEndTime() || beatNumber < 0) // position is outside
        throw String("Position is out of bounds");
    mPosition = mBuffer.getNextIndexAtTime(beatNumber * TPQN);
}

void MidiPlayerEngine::resetControllers(const double timestamp) {
    const ScopedLock lock(mResetMutex);
    OwnedArray<MidiMessage> controllers;
    for (int ch = 1; ch <= 16; ch++) {
        mBuffer.createControllerUpdatesForTime(ch, timestamp, controllers);
        for (int message = 0; message < controllers.size(); message++)
            mMidiOut->sendMessageNow(*(controllers[message]));
    }
}

void MidiPlayerEngine::reset() {
    const ScopedLock lock(mResetMutex);
    for (int ch = 1; ch <= 16; ch++) {
        mMidiOut->sendMessageNow(MidiMessage::allNotesOff(ch));
    }
    mMidiOut->reset();
}

void MidiPlayerEngine::setPosition(const int newPos) {
    const ScopedWriteLock lock(mPositionLock);
    mPosition = newPos;
}

int MidiPlayerEngine::getPosition() {
    const ScopedReadLock lock(mPositionLock);
    return mPosition;
}

bool MidiPlayerEngine::MessageFilter::filterMessage(MidiMessage& msg) {
    return false;
}

void MidiPlayerEngine::run() {
    // Es kann beginnen
    MidiMessage message(0, 0, 0, 0);
    Message* listenerMsg = new Message(0, 0, 0, 0);
    int numEvents = mBuffer.getNumEvents();
    int currentPosition = 0;
    double TPQN = static_cast<double>(mFile.getTimeFormat());
    double nextEventTime = 0.;
    double msPerTick = 500. / TPQN; // default 120 BPM
    double prevTimestamp = 0.;
    setPosition(0);

    sendActionMessage("Play");

    while (( !threadShouldExit() ) && ( currentPosition < numEvents )) {

        if (isPaused()) continue;

        message = mBuffer.getEventPointer(currentPosition)->message;

        if (mFilter) mFilter->filterMessage(message);

        nextEventTime = msPerTick *
                        (message.getTimeStamp() - prevTimestamp);

        Time::waitForMillisecondCounter(Time::getMillisecondCounter()
                                        + nextEventTime);

        if (mListener->isValidMessageListener()) {
            listenerMsg->pointerParameter = (void*) &message;
            mListener->postMessage(listenerMsg);
        }

        if (message.isTempoMetaEvent()) {
            msPerTick = message.getTempoSecondsPerQuarterNote() * 1000. / TPQN;
        } else if (!message.isMetaEvent()) {
            mMidiOut->sendMessageNow(message);
        }

        // Send a change message if the timestamp happens to be at a beat
        if (static_cast<int>(prevTimestamp) % static_cast<int>(TPQN) == 0)
            sendChangeMessage(this);

        prevTimestamp = message.getTimeStamp();
        setPosition(++currentPosition);

    }
    delete listenerMsg;
    reset();
    sendActionMessage("Stop");
}
