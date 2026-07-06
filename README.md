# whisperFlow

Local Wispr Flow clone for macOS, driven from the menu bar: click the mic icon,
speak, and the cleaned-up text pastes wherever your cursor is. Transcription is
fully on-device (NVIDIA Parakeet TDT v3 via MLX); a Claude Haiku pass removes
filler words and fixes formatting.

## Status: milestone 4

- [x] Mic capture (with 500ms pre-roll) → Parakeet transcription on-device
- [x] Insert text into the focused app (clipboard + Cmd-V, clipboard restored after)
- [x] Claude Haiku cleanup pass (filler removal, formatting, personal dictionary, per-app tone)
- [x] Menu-bar app: click to start, auto-stop on silence (or click again); secure-input detection

## Setup

```sh
uv venv --python 3.12
uv pip install -e .
```

First run downloads the ~1.2GB Parakeet model from HuggingFace.

## Usage

```sh
# the app: mic icon appears in the menu bar
.venv/bin/whisperflow

# without the Claude cleanup pass
.venv/bin/whisperflow --raw
```

- **Click the mic** → recording starts (icon fills in)
- **Stop talking for ~2s** → recording stops by itself, text pastes at your cursor
- **Or click again** → stops immediately
- **Right-click** → Quit
- Icon states: hourglass = loading model · mic = idle · filled mic = recording ·
  waveform = transcribing/cleaning
- If you click by accident and say nothing, it cancels itself after 10s

Test helpers (no mic/menu bar needed):

```sh
.venv/bin/whisperflow transcribe path/to/16khz-mono.wav
.venv/bin/whisperflow type "hello"     # pastes after 3s — focus a text field
.venv/bin/whisperflow clean "um so can you uh send the report by like friday no wait thursday"
```

Quick transcription self-test without speaking:

```sh
say -o /tmp/t.aiff "testing one two three" && afconvert -f WAVE -d LEI16@16000 -c 1 /tmp/t.aiff /tmp/t.wav
.venv/bin/whisperflow transcribe /tmp/t.wav
```

## Claude cleanup pass

Uses `claude-haiku-4-5` via the Anthropic API to remove filler words, apply
self-corrections ("friday no wait thursday" → "Thursday"), fix formatting, and
match tone to the frontmost app. Needs `ANTHROPIC_API_KEY` in your environment
(or an `ant auth login` profile). Without credentials — or on any API error or
timeout — the raw transcript is pasted instead; dictation never blocks on the
network for more than 8s.

Utterances under 5 words skip the LLM entirely (they're pasted as-is; Parakeet
already punctuates).

**Personal dictionary:** put one word/name/term per line in
`~/.config/whisperflow/dictionary.txt` (`#` for comments) and misheard variants
get corrected to those exact spellings.

## Permissions

Under System Settings → Privacy & Security, the app hosting whisperflow (your
terminal, while prototyping) needs:

- **Microphone** (prompted automatically on first recording)
- **Accessibility** (to post the Cmd-V paste keystroke; add your terminal manually)

No Input Monitoring needed — there are no global hotkeys.

Pasting is skipped automatically when a password field holds secure event input
(the text would otherwise vanish into the password box).

## Performance (M-series, measured)

- Model load: ~0.7s (warm start; 43s first-ever download)
- Warm transcription: **~0.06s** for a 3.5s utterance
- Haiku cleanup: ~0.5–1s for typical dictation (skipped for short utterances)
