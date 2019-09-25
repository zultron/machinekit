#ifndef WORK_OFFSET_STRUCTS_HH
#define WORK_OFFSET_STRUCTS_HH

#include <array>
#include <coordinatetransform.hh>
#include <interp_parameter_def.hh>
#include <map>

template< typename ContainerT, typename PredicateT >
void erase_if( ContainerT& items, const PredicateT& predicate ) {
    for( auto it = items.begin(); it != items.end(); ) {
        if( predicate(*it) ) it = items.erase(it);
        else ++it;
    }
}

using WorkOffsetTable = std::array<RotateShiftTransform, interp_param_global::MAX_EXTENDED_WORK_CSYS+1>;
using WorkOffsetMap = std::map<int, RotateShiftTransform>;
#endif // WORK_OFFSET_STRUCTS_HH
