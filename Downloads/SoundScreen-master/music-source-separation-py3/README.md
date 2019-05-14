To train a new profile:

In config.py, change:
Under TrainConfig:
CKPT_PATH = 'checkpoints/' + CASE
To a directory that you want to put the trained model in.
Eg: 
CKPT_PATH = 'Kevin_Model/' + CASE

VOICE_DATA_PATH = '../../Training_Data/Ali'
To a directory where the training files reside.
eg:
VOICE_DATA_PATH = '../../Training_Data/Kevin'

Inside the music-source-separation-py3 directory run:
>> python3 train.py

To run:

Inside the music-source-separation-py3 directory run:
>> python3 desktop_eval.py

To record training files:

Record a short sound sample on Audacity, save the file as a wav to one folder.


All the directories above are in reference to the music-source-separation-py3 directory.
Currently the voice wav files are located in /home/dan/git/Training_Data/<Name-of-person> on the linux desktop.
  
