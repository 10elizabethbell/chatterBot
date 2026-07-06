"""Insert text into the frontmost app: clipboard + simulated Cmd-V.

The clipboard route is the de-facto standard for dictation apps — one
paste event works in nearly every app (native, Electron, web) and is
instant regardless of text length. The previous clipboard string is
restored afterwards unless something else wrote to the pasteboard in
the meantime (e.g. a clipboard manager).

Posting the Cmd-V key event requires the hosting app to have
Accessibility permission; without it the event is silently dropped.
"""

from __future__ import annotations

import ctypes
import time

import Quartz
from AppKit import NSPasteboard, NSPasteboardTypeString

_carbon = ctypes.CDLL("/System/Library/Frameworks/Carbon.framework/Carbon")


def secure_input_active() -> bool:
    """True when a password field (or Terminal's Secure Keyboard Entry) holds
    secure event input — synthetic keystrokes are pointless and the paste
    would land in a password box. Callers should skip pasting."""
    return bool(_carbon.IsSecureEventInputEnabled())

KEY_V = 9  # kVK_ANSI_V

# pasteboard write -> target app reads it on paste; these delays let the
# pasteboard sync before the keystroke and let the paste complete before
# the old clipboard is restored
PRE_PASTE_DELAY = 0.05
POST_PASTE_DELAY = 0.25


def _post_cmd_v() -> None:
    for key_down in (True, False):
        event = Quartz.CGEventCreateKeyboardEvent(None, KEY_V, key_down)
        Quartz.CGEventSetFlags(event, Quartz.kCGEventFlagMaskCommand)
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)


def insert_text(text: str) -> None:
    """Paste `text` at the cursor of whatever app is focused."""
    if not text:
        return
    pasteboard = NSPasteboard.generalPasteboard()
    saved = pasteboard.stringForType_(NSPasteboardTypeString)

    pasteboard.clearContents()
    pasteboard.setString_forType_(text, NSPasteboardTypeString)
    our_change = pasteboard.changeCount()

    time.sleep(PRE_PASTE_DELAY)
    _post_cmd_v()
    time.sleep(POST_PASTE_DELAY)

    # restore only if the pasteboard still holds our text — if the user
    # (or a clipboard manager) wrote to it meanwhile, leave theirs alone
    if saved is not None and pasteboard.changeCount() == our_change:
        pasteboard.clearContents()
        pasteboard.setString_forType_(saved, NSPasteboardTypeString)
