class PolyphonyVoiceManager:

    def __init__(self):
        # The low-level voice stacking matrix is allocated inside engine.cpp
        self.max_voices = 4
