#ifndef PREVIEWINTERP_HH
#define PREVIEWINTERP_HH

#include "rs274ngc_interp.hh"

class PreviewInterp : public Interp
{
public:
    PreviewInterp() : Interp() {}
     virtual int write_and_update_state_tag(block_pointer block=nullptr, int line_number=0);
    virtual int restore_parameters(const char *filename);
    virtual bool is_preview() const {return true;}
    virtual bool need_step_increment(const setup_struct *) const {return true;}
    virtual int write_modal_state(block_pointer const block, setup_pointer settings);
    virtual int synch_work_offsets_from_task(setup_pointer settings) const;
    virtual int check_startline_leadin_feedrate(double & feed_rate, const char * move_type);
};

#endif // PREVIEWINTERP_HH
