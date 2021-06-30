# PurpleDrop STM32 Software Releases

## 0.5.1 (2021-06-30)

- Support low gain for active capacitance measurements.
  - This includes adding a flag for gain setting to active capacitance
    message, breaking messaging compatibility
- Increase HV regulator integrator limit
  - On some boards, this limit prevents the controller from reaching target at
    higher voltages
- Add per-electrode capacitance offset calibration
- Feedback controller uses real capacitance units (pF) instead of count

## 0.4.1 (2021-04-28)

- Adds feedback control mode for droplet splitting
- Adds support for up to 5 capacitance scan groups for high rate measurement of
arbitrary sets of electrodes.
- Adds support for two drive groups and varying PWM duty cycle for each.
- Adds support for inverting opto-isolators as AppConfig option.
  - Allows accomodating alternative parts when the designed part is out of stock.
- Adds integrator feedback to high voltage control.
  - Previously, it was a constant output calibrated for a particular load. This
  allows for accurate voltage control under different loads, important now because
  new board revision has different resistor values in the voltage sense divider,
  which substantially alters the load on the HV supply.

NOTE: This release alters the structure of several messages and so breaks
USB API compatibility in order to support drive groups and scan groups. The
purpledrop-driver software must be updated along with this update.

## 0.3.1 (2020-03-11)

- Add controllable GPIO outputs
- Disable timeout on PWM channels 4-15
- Update VID/PID to 0x1209/0xCCAA

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

