#ifndef _MIDIPLAYERENGINE_H
#define _MIDIPLAYERENGINE_H

#include <juce/juce.h>

/** This class handles playback of MIDI files. It is also an ActionBroadcaster
 *  which broadcasts a string when certain events happen:
 *  "Play"      When playback begins
 *  "Stop"      When playback has finished
 *  "Pause"     When playback has been temporarily paused
 *  Frequently during playback, the class will send broadcast a change message
 *  so that clients can check for the current playback position.
 */
class MidiPlayerEngine :
            public    ActionBroadcaster,
            public    ChangeBroadcaster,
            public    DeletedAtShutdown,
            protected Thread {
public:
    /** Constructor. Loads the specified MIDI file asynchronously and makes it
     *  ready for playback.
     *  @param stream The input stream represneting the MIDI file
     *  @param  midiDevice The MIDI device to use for playback.
     *          Caller must take care of deleting the MidiOutput device. */
    MidiPlayerEngine(InputStream* stream, MidiOutput* midiDevice);

    /** Destructor */
    virtual ~MidiPlayerEngine();

    /** Set the MessageListener object which will get each MIDI message as it is
     *  played.
     *  @param  listener The object which will receive a Message whose integer
     *          parameters will be 0, 0, 0 and the pointer parameter will be a
     *          pointer to the actual MidiMessage being played back. Set to 0 to
     *          disable messages.*/
    void setMessageListener(MessageListener* listener);
    
    /** This allows the client to filter the MIDI events in real time while
     *  playback is going on. @see setFilterCallback */
    class MessageFilter {
    public:
        /** This function is called when a message is about to be played back.
         *  Perform any filtering/changes to this message. Make the processing
         *  as short as possible, as this is sent on the high priority playback
         *  thread, just before the message is queued.
         *  @param message The message to be filtered
         *  @return true if the message should NOT be played at all */
        virtual bool filterMessage(MidiMessage& message);
    };
    
    /** Sets the object which will receive callbacks to filter MIDI messages
     *  before they are played back. @see MessageFilter::filterMessage
     *  @param filterObject The object which will receive the callback.
     *                      Pass 0 to turn off filtering. */
    void setFilterCallback(MessageFilter* filterObject);

    /** Begins or resumes playback.*/
    void play();

    /** Stops playback, resetting play position to the beginning of the file.*/
    void stop();

    /** Temporarily pauses the playback. Stops all sounding notes.*/
    void pause();
    
    /** Whether the playback is paused or not
     *  @return true if the playback is paused */
    bool isPaused();

    /** Seek to the given position
     *  @param beatNumber The beat number to seek to.
     *  @see getNumBeats() */
    void seek(const int beatNumber);

    /** Returns the length of the midi file in number of ticks
     *  @return The number of midi ticks until the end*/
    double getLength();
    
    /** Returns the position of the currently playing file (in midi ticks).
     *  @return Position in ticks */
    double getSeekPosition();
    
    /** Resets the midi output (all sounds and controllers off).
     *  Beware! This is NOT called on the playback thread, but on the calling
     *  thread instead! */
    void reset();
    
    BitArray channelMute;  ///< Which channels are totally muted
    BitArray channelFreeze;///< Which have only controllers muted

protected:
    MessageListener*        mListener;
    MidiOutput*             mMidiOut;
    MessageFilter*          mFilter;
    MidiMessageSequence     mBuffer;
    MidiFile                mFile;
    int                     mPosition;
    ReadWriteLock           mPauseLock;
    ReadWriteLock           mPositionLock;
    CriticalSection         mResetMutex;
    bool                    mIsPaused;
    void resetControllers(const double timestamp); // Backtrack controllers
    void setPosition(const int newPos);
    int getPosition();
    void run(); ///< @internal The thread function
};

#endif  /* _MIDIPLAYERENGINE_H */
