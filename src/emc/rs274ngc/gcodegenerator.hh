#ifndef GCODEGENERATOR_HH
#define GCODEGENERATOR_HH

#include <vector>
#include <string>
#include <modal_state.hh>
#include <featureset.hh>
#include <positionarray.hh>

using RestoreCommandList = std::vector<std::string>;

struct setup_struct;
struct ShiftTransform;

class GCodeGenerator
{
protected:
    RestoreCommandList cmd_list;
    FeatureSet features;
    AxisMask axis_mask;

    void start_next_line();

    static int formatGCode(GCodes g, std::string &cmd);
    void add_axes(std::ostream &out, PositionArray const & goal, AxisMask active_axes);
public:
    GCodeGenerator(FeatureSet const &features_in, AxisMask machine_axes) :
        cmd_list(),
        features(features_in),
        axis_mask(machine_axes) {}

    RestoreCommandList const &getCommandSequence() const;
    std::string getCommandSequenceAsString() const;

    static void filterAbortedStates(ModalState &current, ModalState &saved);

    void addPreformatted(std::string const &g_code_line, const char *trailing_comment_if_nonempty = nullptr);
    void addWorkOffset(int work_offset_number);
    void addToolOffsetUpdate(int tool_offset_number);
    void addToolChange(int tool_number);
    void addToolLengthOffsetFromTable(int tool_number);
    void addLocalAxisOffsets(ShiftTransform const &local_axis_offset, AxisMask active_axes=-1);
    void clearLocalAxisOffsets(bool clear_params);
    int addModalStateGCodes(ModalState &current, const ModalState & saved);
    int addModalStateMCodes(const ActiveMCodeArray & current, const ActiveMCodeArray & saved);
    int addQueueBuster(const char * comment=nullptr);
    int addLinearFeedMove(PositionArray const &goal, double feed_prog, AxisMask active_axes=-1);
    int addLinearRapidMove(PositionArray const &goal, AxisMask active_axes=-1);

    int addFeedRate(double feed_rate);
    int addFeedMode(GCodes feed_mode, double feed_rate=0.);

    int gen_modalStateRestore(ModalState & current, const ModalState & saved);

    void cancelToolCompensation();
    void addToolCompensation(GCodes cutter_side, CutterCompData const &comp_data_to_restore);
    void addToolCompensationWithRadius(GCodes cutter_side, double cutter_radius);
    void addToolCompensationFromTable(GCodes cutter_side, int tool_number);
    void printMsg(std::string const &&msg);
    void addSpindleMode(GCodes spindle_mode, double css_max_speed, double speed);
    void addSpindleSpeed(double speed);
};

#endif // GCODEGENERATOR_HH
