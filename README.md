A xovi extension for the rmpp marker plus that lets you set the tool for the eraser end of your marker.

> This tool is currently in beta as I wanted to add more features but it is usable at the current state.

## How to use?
It currently just switches the tool to eraser the tab when you use the back end, meaning you just need to select the eraser mode you want to use.
Note: the eraser tool may get broken after a screen clear, just press the tool you wanted again if that happens (will fix in the future).

## Dependencies
This extension depends on [fileman](https://github.com/asivery/rmpp-xovi-extensions/tree/master/fileman) and [qt-resource-rebuilder](https://github.com/asivery/rmpp-xovi-extensions/tree/master/qt-resource-rebuilder).

## Compiling
Rename `template.env` to `.env` and set the variables for the remarkable toolchain and xovi home directories.
Then run `bash make.sh` to build.

## TODOs:
  - [ ] Add a toggle to enable and a button to set the tool to anything.
  - [ ] Set up an autobuild.
  - [ ] Remove unnecessary logs and clean up the code.
