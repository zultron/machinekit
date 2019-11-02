#ifndef GCODEGENERATOR_HH
#define GCODEGENERATOR_HH

#include <vector>
#include <string>
#include <modal_state.hh>
#include <featureset.hh>
#include <positionarray.hh>

using RestoreCommandList = std::vector<std::string>;

struct setup_struct;

class GCodeGenerator
{
protected:
    RestoreCommandList cmd_list;
    FeatureSet features;

    void start_next_line();

    static int formatGCode(GCodes g, std::string &cmd);
public:
    GCodeGenerator(FeatureSet const &features_in) :
        cmd_list(),
        features(features_in) {}

    RestoreCommandList const &getCommandSequence() const;
    std::string getCommandSequenceAsString() const;

    static int filterAbortedStates(ModalState &current, ModalState &saved);

    void addWorkOffset(int work_offset_number);
    void addToolOffset(int tool_number);
    void addToolChange(int tool_number);
    int addModalStateGCodes(ModalState &current, const ModalState & saved);
    int addModalStateMCodes(const ActiveMCodeArray & current, const ActiveMCodeArray & saved);
    int addLinearFeedMove(PositionArray const &goal, double feed);

    int gen_modalStateRestore(ModalState & current, const ModalState & saved);

};

#endif // GCODEGENERATOR_HH
