#!/usr/bin/env python

from piwho import recognition

recog = recognition.SpeakerRecognizer()
name = []
name = recog.identify_speaker('./SoundSamples/SampleRecognize/46.wav')
print(name[0])
