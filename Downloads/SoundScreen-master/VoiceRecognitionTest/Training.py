#!/usr/bin/env python

from piwho import recognition

recog = recognition.SpeakerRecognizer('./SoundSamples/MichelSounds')
recog.speaker_name = 'Michel'
recog.train_new_data()

recog = recognition.SpeakerRecognizer('./SoundSamples/Sally')
recog.speaker_name = 'Sally'
recog.train_new_data()
