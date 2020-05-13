#ifndef NML_PRETTYPRINT_HH
#define NML_PRETTYPRINT_HH

#include <sstream>
#include <emc_nml.hh>
#include <emc.hh>
struct EmcPose;

#include <positionarray.hh>
#include <coordinatetransform.hh>

// For generic classes used in NML and elsewhere
std::ostream &operator<<(std::ostream &out, EmcPose const &p);
std::ostream &operator<<(std::ostream &out, PositionArray const &p);
std::ostream &operator<<(std::ostream &out, ShiftTransform const &p);
std::ostream &operator<<(std::ostream &out, RotateShiftTransform const &p);

// For specific NML messages
std::ostream &to_stream(std::ostream &out, EMC_TRAJ_SET_G92 const &msg);
std::ostream &to_stream(std::ostream &out, EMC_TRAJ_SET_G5X_TRANSFORM const &msg);
std::ostream &to_stream(std::ostream &out, EMC_TRAJ_SET_TOOL_OFFSET const &msg);

template <typename NMLMessageType>
void log_processed_message(NMLMessageType const &msg, CanonMessageAlarmLevel lvl, const char *log_prefix="")
{
    std::ostringstream oss;
    oss << log_prefix;
    to_stream(oss, msg);
    emcOperatorText(lvl, "%s", oss.str().c_str());
}

#endif // NML_PRETTYPRINT_HH
