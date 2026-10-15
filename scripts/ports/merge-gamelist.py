#!/usr/bin/env python3
"""Merge the Falllinie entry into an EmulationStation ports gamelist.

Usage: merge-gamelist.py EXISTING_OR_EMPTY OUTPUT
Keeps every other <game> entry and replaces only the one whose <path> is
./falllinie-spike.sh. An empty or missing input produces a new gamelist.
"""
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ENTRY = {
    "path": "./falllinie-spike.sh",
    "name": "Falllinie",
    "desc": "Native dual-screen port of the Falllinie downhill snowboarding game: "
            "third-person view on the top screen, first-person view on the bottom screen, "
            "print-style rendering and a reactive generated soundtrack.",
    "image": "./images/falllinie-spike.png",
    "genre": "Sports",
    "players": "1",
}

def main():
    source, target = Path(sys.argv[1]), Path(sys.argv[2])
    text = source.read_text() if source.exists() else ""
    root = ET.fromstring(text) if text.strip() else ET.Element("gameList")
    if root.tag != "gameList":
        raise SystemExit(f"unexpected root element {root.tag!r}; refusing to rewrite")
    for game in list(root.findall("game")):
        if (game.findtext("path") or "").strip() == ENTRY["path"]:
            root.remove(game)
    game = ET.SubElement(root, "game")
    for key, value in ENTRY.items():
        ET.SubElement(game, key).text = value
    ET.indent(root)
    target.write_text('<?xml version="1.0"?>\n' + ET.tostring(root, encoding="unicode") + "\n")

if __name__ == "__main__":
    main()
