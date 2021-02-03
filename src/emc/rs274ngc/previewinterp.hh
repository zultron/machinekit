#ifndef PREVIEWINTERP_HH
#define PREVIEWINTERP_HH

#include "rs274ngc_interp.hh"

class PreviewInterp : public Interp
{
public:
    PreviewInterp() : Interp() {}
    virtual int restore_parameters(const char *filename);
    virtual bool is_preview() const {return true;}
    virtual bool need_step_increment(const setup_struct *) const {return true;}
    virtual int write_modal_state(block_pointer const block, setup_pointer settings);
    virtual int synch_work_offsets_from_task(setup_pointer settings) const;
    virtual int check_startline_leadin_feedrate(double & feed_rate, const char * move_type);
    virtual int check_leadin_feedmode(const ModalState & initial, const ModalState & to_restore, int start_line, double leadin_feedrate) const;
    virtual StateTag make_state_tag(setup_struct const &s, block_pointer block, int toplevel_line=0) const;

protected:
    virtual const char * getStateRestoreSubroutineName() const {
        return "_preview_restore_start_line_state";
    }
};

#endif // PREVIEWINTERP_HH
