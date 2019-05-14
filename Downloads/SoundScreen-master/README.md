# RaspberryPi Machine Learning Setup Instructions

## raspbian-lite 11-13-18
* sudo raspi-config
* connect to wifi and enable ssh
* sudo apt-get install git
* git clone https://github.com/m47jiang/SoundScreen.git
* sudo apt-get install python3-pip
* sudo apt-get install libatlas-base-dev
* sudo pip3 install tensorflow==1.11.0
* sudo pip3 install joblib==0.11.0
* sudo pip3 install librosa==0.4.2
* sudo apt-get install libsamplerate0-dev
* sudo pip3 install git+https://github.com/gregorias/samplerate.git
* sudo pip3 install soundfile
* sudo apt-get install libsndfile1
* sudo apt-get install portaudio19-dev #not needed
* sudo pip3 install pyaudio # no longer needed

Will need model in /SoundScreen/music-.../checkpoints/4frames-.../ for inferencing to work

Current model trained on Michel's voice is located at /SoundScreen/music-.../Michel\ Model\ 1.0/4frames-...

To run offline filtering, put wav in /SoundScreen/music-.../dataset/test and run eval_offline.py

Output will be in /SoundScreen/music-.../results/output/

To play .wav files: aplay name-of-file.wav
