#ifndef TOOL_MOTION_DATA_HH
#define TOOL_MOTION_DATA_HH

#include <map>
#include <vector>
#include <tuple>

#include "canon_enums.hh"
#include "positionarray.hh"
#include "coordinatetransform.hh"

enum class MotionType
{
    STRAIGHT_FEED,
    RAPID,
    ARC_FEED,
    RIGID_TAP,
    PROBE,
};

enum class ParseExecMode {
    SEEK, // Before the start line, motions are processed but not executed (shown in a faded color in the preview)
    PREP, // Preparatory / lead-in motions at the start line
    RUN, // Normal program run (i.e. everything after the start line is reached)
};

struct ToolMotionKey
{
    MotionType motion_type;
    ParseExecMode exec_mode;
    int tool_pocket_no;

};
bool operator<(ToolMotionKey const &a, ToolMotionKey const &b) {
    return std::tie(a.motion_type, a.exec_mode, a.tool_pocket_no) < std::tie(b.motion_type, b.exec_mode, b.tool_pocket_no);
}

inline const char * motion_type_as_str(MotionType motion_type) {
    switch (motion_type){
    case MotionType::STRAIGHT_FEED:
        return "straight_feed";
    case MotionType::RAPID:
        return "traverse";
    case MotionType::PROBE:
        //TODO unique color?
        return "probe";
    case MotionType::RIGID_TAP:
        //TODO unique color?
        return "rigid_tap";
    case MotionType::ARC_FEED:
        return "arc_feed";
    }
    return "unknown";
}

inline MotionType motion_type_from_str(const char *motion_str) {
    static const std::map<std::string, MotionType> str_lookup{
        {"straight_feed", MotionType::STRAIGHT_FEED},
        {"traverse", MotionType::RAPID},
        {"arc_feed", MotionType::ARC_FEED},
        {"probe", MotionType::PROBE},
        {"rigid_tap", MotionType::RIGID_TAP},
        };
    auto it = str_lookup.find(motion_str);
    if (it != str_lookup.end()) {
        return it->second;
    } else {
        // KLUDGE deal with this later
        return MotionType::STRAIGHT_FEED;
    }
}


struct ToolMotion
{
    int line_number;
    PositionArray p_start;
    PositionArray p_end;
    double feed_rate;
};

struct ExtentBox
{
    PositionArray min;
    PositionArray max;
};

/**
 * Stores tool motion data similar to what's in python now.
 */
using ToolMotionMap = std::map<ToolMotionKey, std::vector<ToolMotion> >;

#endif // TOOL_MOTION_DATA_HH
