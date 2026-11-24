"""Build the HOME Menu banner.

The clip is a later part of the track. It is already near full scale on the
peaks, so a plain normalize does not make it louder. The body is pushed up
with a soft clip, then it fades out. There is no fade in. The menu wants
stereo 16-bit PCM under 3 seconds and not looping, which bannertool's -a
path writes.
"""
import math
import struct
import subprocess
import wave
from pathlib import Path

HERE = Path(__file__).resolve().parent
PNG = HERE / "banner.png"
WAV = HERE / "banner.wav"
OUT = HERE / "banner.bin"
MP3_CANDIDATES = (
    Path("/workspace/attachments/bgm.mp3"),
    HERE / "bgm.mp3",
)
CLIP_START = 94.5
CLIP_SECONDS = 2.0
RATE = 32000
FADE_OUT = 0.40
DRIVE = 4.0


def boost(samples):
    scale = DRIVE / 32768.0
    out = []
    for sample in samples:
        value = int(math.tanh(sample * scale) * 32767.0)
        if value > 32767:
            value = 32767
        elif value < -32767:
            value = -32767
        out.append(value)
    return out


def write_clip():
    mp3 = next((path for path in MP3_CANDIDATES if path.exists()), None)
    if mp3 is None:
        if not WAV.exists():
            raise SystemExit("missing banner audio")
        return
    frames = int(CLIP_SECONDS * RATE)
    raw = subprocess.check_output(
        [
            "ffmpeg", "-v", "error",
            "-ss", str(CLIP_START), "-t", str(CLIP_SECONDS),
            "-i", str(mp3),
            "-ac", "2", "-ar", str(RATE),
            "-f", "s16le", "-",
        ]
    )
    samples = list(struct.unpack("<%dh" % (len(raw) // 2), raw))
    if len(samples) < frames * 2:
        raise SystemExit("clip is shorter than 2 seconds")
    samples = boost(samples[: frames * 2])
    fade = int(FADE_OUT * RATE)
    for i in range(fade):
        gain = (fade - 1 - i) / fade
        frame = frames - fade + i
        samples[frame * 2] = int(samples[frame * 2] * gain)
        samples[frame * 2 + 1] = int(samples[frame * 2 + 1] * gain)
    with wave.open(str(WAV), "w") as handle:
        handle.setnchannels(2)
        handle.setsampwidth(2)
        handle.setframerate(RATE)
        handle.writeframes(struct.pack("<%dh" % len(samples), *samples))


write_clip()

if not WAV.exists() or WAV.stat().st_size < 64:
    raise SystemExit(f"missing banner audio: {WAV}")

with wave.open(str(WAV), "rb") as handle:
    if handle.getnchannels() != 2 or handle.getsampwidth() != 2:
        raise SystemExit("banner wav must be stereo 16-bit")
    frames = handle.getnframes()
    rate = handle.getframerate()
seconds = frames / rate
if not (0 < seconds < 3.0):
    raise SystemExit(f"banner audio is {seconds:.3f}s, must be under 3 seconds")

subprocess.check_call(
    ["bannertool", "makebanner", "-i", str(PNG), "-a", str(WAV), "-o", str(OUT)]
)
print(f"banner {seconds:.2f}s at {rate} Hz")
