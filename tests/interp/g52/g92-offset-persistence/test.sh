#!/bin/bash
#
# Set up .var file with made-up values for #5210-#5219
#
cp g92_offsets.var.template g92_offsets.var
#
# Check PERSISTENT_G92_OFFSET = true
# Should reproduce params in .var file for #5210-#5219
#
rs274 -g -v g92_offsets.var -i persistent_g92_offset_true.ini \
    g92_offsets.ngc
#
# Check PERSISTENT_G92_OFFSET = false
# Should produce 0.0000 values for #5210-#5219
#
rs274 -g -v g92_offsets.var -i persistent_g92_offset_false.ini \
    g92_offsets.ngc
#
# Check PERSISTENT_G92_OFFSET unset
# Should produce 0.0000 values for #5210-#5219
#
rs274 -g -v g92_offsets.var g92_offsets.ngc
