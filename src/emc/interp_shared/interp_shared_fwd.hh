#ifndef INTERP_SHARED_FWD_HH
#define INTERP_SHARED_FWD_HH

#include <array>
#include <bitset>

namespace interp_shared {

static constexpr const size_t CANON_AXIS_COUNT = 9;
// For now identical to the total count of axes but may be smaller for special applications
static constexpr const size_t MAX_USED_AXES = CANON_AXIS_COUNT;

struct RotateShiftTransform;
struct ShiftTransform;
struct LocalToMachineTransform;
struct UnitsTransform;

using PositionArray = std::array<double, CANON_AXIS_COUNT>;

enum PositionArrayIndex
{
    X,
    Y,
    Z,
    A,
    B,
    C,
    U,
    V,
    W,
};

static constexpr std::array<char, CANON_AXIS_COUNT> AxisNames = {
    'X',
    'Y',
    'Z',
    'A',
    'B',
    'C',
    'U',
    'V',
    'W',
};

using AxisMask = std::bitset<64>;

const constexpr std::array<size_t, 6> linearAxes = {X,Y,Z,U,V,W};
const constexpr std::array<size_t, 3> angularAxes = {A,B,C};
const constexpr AxisMask angularAxisMask = 1 << A |
                                     1 << B |
                                     1 << C;
const constexpr AxisMask linearAxisMask = 1 << X |
                                    1 << Y |
                                    1 << Z |
                                    1 << U |
                                    1 << V |
                                    1 << W;


}

#endif // INTERP_SHARED_FWD_HH
