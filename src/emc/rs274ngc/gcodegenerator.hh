#ifndef GCODEGENERATOR_HH
#define GCODEGENERATOR_HH

#include <vector>
#include <string>
#include <modal_state.hh>

using RestoreCommandList = std::vector<std::string>;

struct setup_struct;

class GCodeGenerator
{
protected:
    RestoreCommandList cmd_list;
    struct setup_struct const &settings;

    void start_next_line();

    static int formatGCode(GCodes g, std::string &cmd);
public:
    GCodeGenerator(struct setup_struct const &settings_in) :
        cmd_list(),
        settings(settings_in) {}

    RestoreCommandList const &getCommandSequence() const;
    std::string getCommandSequenceAsString() const;

    static int filterAbortedStates(ModalState &current, ModalState &saved);

    void addWorkOffset(int work_offset_number);
    int addModalStateGCodes(ModalState &current, const ModalState & saved);
    int addModalStateMCodes(const ActiveMCodeArray & current, const ActiveMCodeArray & saved);

    int gen_modalStateRestore(ModalState & current, const ModalState & saved);

};

#endif // GCODEGENERATOR_HH
