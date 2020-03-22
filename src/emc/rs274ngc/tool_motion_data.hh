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
    SEEK,
    PREP,
    RUN,
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

struct ToolMotion
{
    int line_number;
    PositionArray p0;
    PositionArray p1;
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
