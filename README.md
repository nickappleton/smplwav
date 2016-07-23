# smplwav

A small library for managing wave files to be used by samplers.

## Dependencies

This library has a dependency on cop <https://www.github.com/nickappleton/cop.git> to handle endian related stuff.

## Overview

smplwav is designed to be simple and efficient and does not perform any memory allocation. There are two functions smplwav_mount and smplwav_serialise.

smplwav_mount() takes a wave file (either read into memory or memory-mapped) and will populate a smplwav structure. String metadata related to markers or found in the RIFF INFO chunk will be stored in the structure as pointers back to the input data (as opposed to being copied) and there is a maximum number of markers (which cover both cue point and smpl loops) that can be loaded, but I don't think the number will ever be hit in the use cases this library is intended for.

smplwav_serialise() will take smplwav structure and serialise it to a memory blob.

See the API headers for more information.

## Bundled Appliations

### app_samplauth

Is an application for repairing/authoring wave samples to be shipped in sample sets. It can set and remove metadata for a sample and can be used to diagnose and correct problems with samples before they are released.

### app_autoloop

Is a tool for roughing in loops (and probably releases markers at some point) in samples. I am still in the middle of developing this tool.