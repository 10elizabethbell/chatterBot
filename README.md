# whisperFlow

Local Wispr Flow clone for macOS: hold a key, speak, release — text appears.
On-device transcription via NVIDIA Parakeet TDT v3 (MLX, runs on Apple Silicon),
with a Claude Haiku cleanup pass planned (milestone 3).

## Status: milestone 3

- [x] Hold-key push-to-talk → mic capture (with 500ms pre-roll) → Parakeet transcript printed to terminal
- [x] Milestone 2: insert text into the focused app (clipboard + Cmd-V, clipboard restored after)
- [x] Milestone 3: Claude Haiku cleanup pass (filler removal, formatting, personal dictionary, per-app tone)
- [ ] Milestone 4: skip-short-utterance rule, secure-input detection, menu-bar UI

## Setup

```sh
uv venv --python 3.12
uv pip install -e .
```

First run downloads the ~1.2GB Parakeet model from HuggingFace.

## Usage

```sh
# push-to-talk: hold RIGHT OPTION, speak, release -> cleaned text pastes at the cursor
.venv/bin/whisperflow

# skip the Claude cleanup pass (raw Parakeet transcript)
.venv/bin/whisperflow --raw

# terminal output only (no pasting)
.venv/bin/whisperflow --print-only

# paste test without the mic: focus any text field within 3s
.venv/bin/whisperflow type "hello from whisperflow"

# cleanup test without the mic
.venv/bin/whisperflow clean "um so can you uh send the report by like friday no wait thursday"

# transcribe a wav without mic/hotkey (16kHz mono; convert with afconvert)
.venv/bin/whisperflow transcribe path/to/audio.wav
```

Quick self-test without speaking:

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

Push-to-talk mode needs your terminal app to have, under System Settings → Privacy & Security:

- **Microphone** (prompted automatically on first run)
- **Input Monitoring** and **Accessibility** (for the global hotkey listener; add your terminal manually)

## Performance (M-series, measured)

- Model load: ~0.7s (warm start; 43s first-ever download)
- Warm transcription: **~0.06s** for a 3.5s utterance
