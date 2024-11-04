# Presentation timer

A simple on-screen timer with transparent background to time your presentations.


## Compiling

```
qmake && make
```

## Command line options

- "-s SIZE" set font size in points
- "-d DURATION" set countdown duration in seconds
- "-p PX PY" set initial position on screen (negative values = from end)

## User manual

Clicking while running will move the timer between screen corners.

Clicking while stopped will start the timer.

Right-clicking shows a popup with three entries:

- reset: reset to duration but do not start (click to start)
- reset and start: reset and start countdown
- quit: quit application
