#!/bin/bash

# Shell-Skript zum Aktualisieren der Kalenderdateien und Ausführen von index.php

# URL zur neuen Kalenderdatei (pro Klasse!)
ICS_URL="https://intranet.bib.de/ical/d819a07653892b46b6e4d2765246b7ab"

# Verzeichnis, in dem sich die Kalenderdateien befinden
KALENDER_DIR="kalenderdateien"

# Sicherstellen, dass das Verzeichnis existiert
mkdir -p "$KALENDER_DIR"

# Alte Datei löschen
if [ -f "$KALENDER_DIR/kalender_alt.ics" ]; then
    echo "Lösche kalender_alt.ics..."
    rm "$KALENDER_DIR/kalender_alt.ics"
fi

# kalender_neu.ics umbenennen
if [ -f "$KALENDER_DIR/kalender_neu.ics" ]; then
    echo "Benenne kalender_neu.ics in kalender_alt.ics um..."
    mv "$KALENDER_DIR/kalender_neu.ics" "$KALENDER_DIR/kalender_alt.ics"
else
    echo "Nope."
fi

# Neue Datei herunterladen
echo "Lade neue Kalenderdatei herunter..."
curl -f -s -o "$KALENDER_DIR/kalender_neu.ics" "$ICS_URL"
chmod 666 "$KALENDER_DIR/kalender_neu.ics"
if [ $? -ne 0 ]; then
    echo "Nope."
    exit 1
fi

# index.php ausführen
echo "Führe index.php aus :)"
php index.php
