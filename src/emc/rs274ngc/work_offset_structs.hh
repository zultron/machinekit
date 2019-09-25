#ifndef WORK_OFFSET_STRUCTS_HH
#define WORK_OFFSET_STRUCTS_HH

#include <array>
#include <coordinatetransform.hh>
#include <interp_parameter_def.hh>
#include <map>

using WorkOffsetTable = std::array<RotateShiftTransform, interp_param_global::MAX_EXTENDED_WORK_CSYS+1>;
using WorkOffsetMap = std::map<int, RotateShiftTransform>;
#endif // WORK_OFFSET_STRUCTS_HH
