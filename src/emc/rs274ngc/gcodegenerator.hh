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

    void start_next_line();

    static int formatGCode(GCodes g, std::string &cmd);
    void add_axes(std::ostream &out, PositionArray const & goal, AxisMask active_axes);
public:
    GCodeGenerator(FeatureSet const &features_in) :
        cmd_list(),
        features(features_in) {}

    RestoreCommandList const &getCommandSequence() const;
    std::string getCommandSequenceAsString() const;

    static int filterAbortedStates(ModalState &current, ModalState &saved);

    void addPreformatted(std::string const &g_code_line);
    void addWorkOffset(int work_offset_number);
    void addToolOffset(int tool_number);
    void addToolChange(int tool_number);
    void addLocalAxisOffsets(ShiftTransform const &local_axis_offset, AxisMask active_axes=-1);
    void clearLocalAxisOffsets(bool clear_params);
    int addModalStateGCodes(ModalState &current, const ModalState & saved);
    int addModalStateMCodes(const ActiveMCodeArray & current, const ActiveMCodeArray & saved);
    int addQueueBuster(const char * comment=nullptr);
    int addLinearFeedMove(PositionArray const &goal, double feed_prog, AxisMask active_axes=-1);
    int addLinearRapidMove(PositionArray const &goal, AxisMask active_axes=-1);

    int gen_modalStateRestore(ModalState & current, const ModalState & saved);

    void addToolCompensation(GCodes cutter_side, CutterCompData const &comp_data_to_restore);
    void addToolCompensationWithRadius(GCodes cutter_side, double cutter_radius);
    void addToolCompensationFromTable(GCodes cutter_side, int tool_number);
};

#endif // GCODEGENERATOR_HH
