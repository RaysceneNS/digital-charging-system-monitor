# Overview

This is a microcontroller project that monitors a vehicles charging system for voltage related problems.

## Theory of Operation

The input voltage from the vehicle is fed into a voltage divider network
and then passed to the micro controllers ADC input to be sampled continuously using an interrupt service handler. Samples are added to a simple averaging filter to improve resolution and filter noise.

A state machine is used to switch display modes for the attached bi-color led based on the system voltage falling within defined ranges.

The state of the display is updated once per second.

## System States

- 18v
    green/red alternating over-charging  
- 15.20v
    off steady normal charging
- 13.20v
    amber steady under-charging
- 12.45v
    red slow flashing not charging
- 12.25v
    red 2 flashes, repeating not charging
- 12.05v
    red 3 flashes, repeating not charging
- 11.80v
    red 4 flashes, repeating not charging

## Bill of materials

- 1 pcs 5mm Red/Green (optionally RGB) Common Anode LED
- 1 pcs Digi spark board
- 1 pcs 39K resistors for voltage divider
- 1 pcs 12K resistors for voltage divider
