# Test for movement after stop button

Tests for [issue #865][1], where the stop button is pressed while a
long `.ngc` file is still being loaded by interp.  A bug caused an
additional line to be appended to the interp list after the interp
list was cleared in response to the stop command.

[1]:  https://github.com/LinuxCNC/linuxcnc/issues/865
