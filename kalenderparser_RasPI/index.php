<?php
function parseIcsEvents($icsContent) {
    preg_match_all('/BEGIN:VEVENT(.*?)END:VEVENT/s', $icsContent, $matches);
    $events = [];
    foreach ($matches[1] as $event) {
        preg_match('/UID:(.*)/', $event, $uid);
        preg_match('/SUMMARY:(.*)/', $event, $summary);
        preg_match('/DTSTART(?:;TZID=[^:]+)?:([0-9T]+)/', $event, $start);
        preg_match('/DTEND(?:;TZID=[^:]+)?:([0-9T]+)/', $event, $end);
        preg_match('/LOCATION:(.*)/', $event, $location);
        $id = trim($uid[1] ?? uniqid());
        $events[$id] = [
            'summary' => trim($summary[1] ?? ''),
            'start' => trim($start[1] ?? ''),
            'end' => trim($end[1] ?? ''),
        ];
    }
    return $events;
}

function loadDiffs($filename = 'unterschiede.json') {
    if (file_exists($filename)) {
        $json = file_get_contents($filename);
        $data = json_decode($json, true);
        if (is_array($data)) return $data;
    }
    return [];
}

function saveDiffs($diffs, $filename = 'unterschiede.json') {
    file_put_contents($filename, json_encode($diffs));
}

// Kalender laden
$icsA = file_get_contents('kalenderdateien/kalender_alt.ics');
$icsB = file_get_contents('kalenderdateien/kalender_neu.ics');
$eventsA = parseIcsEvents($icsA);
$eventsB = parseIcsEvents($icsB);

// Unterschiede berechnen
$diffs = loadDiffs();
$changed = false;

foreach ($eventsB as $uid => $event) {
    if (!isset($eventsA[$uid]) && !isset($diffs[$uid])) {
        $diffs[$uid] = [
            'type' => 'neu',
            'summary' => $event['summary'],
            'start' => $event['start'],
        ];
        $changed = true;
    }
}

foreach ($eventsB as $uid => $eventB) {
    if (isset($eventsA[$uid]) && $eventsA[$uid] !== $eventB && !isset($diffs[$uid])) {
        $eventA = $eventsA[$uid];
        $diffs[$uid] = [
            'type' => 'geaendert',
            'alt_summary' => $eventA['summary'],
            'alt_start' => $eventA['start'],
            'neu_summary' => $eventB['summary'],
            'neu_start' => $eventB['start'],
        ];
        $changed = true;
    }
}

foreach ($eventsA as $uid => $event) {
    if (!isset($eventsB[$uid]) && !isset($diffs[$uid])) {
        $diffs[$uid] = [
            'type' => 'geloescht',
            'summary' => $event['summary'],
            'start' => $event['start'],
        ];
        $changed = true;
    }
}

if ($changed) {
    saveDiffs($diffs);
}
?>