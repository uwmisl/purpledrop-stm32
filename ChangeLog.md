# PurpleDrop STM32 Software Releases

## 0.3.0 (2020-09-18)

- Update parameter persistence so that new parameters get properly initialized with defaults
- Add separate sample delay for low gain (i.e. large) electrodes
- Add command to recalibrate the capacitance offset
- Fix bug where polarity was left in the wrong state after scan

NOTE: Loading this and later versions will overwrite old saved parameters with new defaults. 

## 0.2.0 (2020-07-29)

- Fix failure mode after running for a long time due to overflow on periodic timer
- Add support for driving low-side augment FET on top plate
- Support setting low gain on select electrodes during capacitance scan
- Adds DFU programming file generation for USB firmware update

## 0.1.0 (2020-06-12)

First release

