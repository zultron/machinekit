#include "previewinterp.hh"
#include "interp_parameter_def.hh"

using namespace interp_param_global;

int PreviewInterp::write_and_update_state_tag(block_pointer block, int line_number)
{
    // Pass through the line number but don't write a tag because they're not needed in the preview context
    return line_number ?: get_toplevel_line_number();
}

int PreviewInterp::restore_parameters(const char * filename)
{
  _setup.parameters.fill(0);
  FILE *infile = nullptr;
  char line[256];

  // it's OK if the parameter file doesn't exist yet
  // it'll be created in due course with some default values
  if(access(filename, F_OK) == -1)
    return INTERP_OK;
  // open original for reading
  infile = fopen(filename, "r");
  CHKS((infile == nullptr), _("Unable to open parameter file: '%s'"), filename);

  while (feof(infile) == 0) {
    if (!fgets(line, 256, infile)) {
      break;
    }

    int variable;
    double value;
    // Skip the line if it doesn't match the pattern
    if (sscanf(line, "%d %lf", &variable, &value) < 2) {
      continue;
    }

    if (variable <= 0) {
        fclose(infile);
        ERS(_("Parameter number out of range: %d"), variable);
    }

    // KLUDGE handle work offsets stored as fake parameters
    if (variable >=  EXTENDED_WORK_OFFSET_START && variable < EXTENDED_WORK_OFFSET_END) {
      // Work offsets
      int offset_number = (variable - EXTENDED_WORK_OFFSET_START ) / 20;
      int offset_base = EXTENDED_WORK_OFFSET_START + 20 * offset_number;
      // zero-indexed displacement from the initial value
      int index = variable - offset_base - 1;
      switch (index) {
      case X:
      case Y:
      case Z:
      case A:
      case B:
      case C:
      case U:
      case V:
      case W:
        _setup.work_offsets_user[offset_number].shift.tf[index] = value;
        break;
      case 9:
        _setup.work_offsets_user[offset_number].setXYRotationDeg(value);
        break;
      }
    } else if (variable < RS274NGC_MAX_PARAMETERS) {
        _setup.parameters[variable] = value;
    }
  }
  fclose(infile);
  for (int origin = 1; origin <= 9; ++origin) {
    find_work_offset_from_parameters(&_setup, origin, _setup.work_offsets_user[origin]);
  }

  auto pred = [](WorkOffsetMap::value_type const &el)
  {
      return !isNonzero(el.second);
  };
  erase_if(_setup.work_offsets_user, pred);
  return INTERP_OK;

}

int PreviewInterp::write_modal_state(const block_pointer block, setup_pointer settings)
{
    // KLUDGE this is a no-op because the preview interpreter never exports modal state to task, so there's no reason to bother updating it for export.
    // WARNING: other overloads ARE used (e.g. for state restore, seek-start-line). Eventually this will be disambiguated properly with better names.
    return INTERP_OK;
}

int PreviewInterp::synch_work_offsets_from_task(setup_pointer settings) const
{
    // Everything is immediate in preview, no disparity to recover from
    return INTERP_OK;
}

int PreviewInterp::check_startline_leadin_feedrate(double &feed_rate, const char * move_type)
{
    if (feed_rate < 1e-10) {
        setWarning(_("Start-line %s lead-in move needs a positive feed rate (start-line lead-in uses current machine feed)"), move_type);
        // KLUDGE force this to be non-zero so that it at least draws the path correctly
        feed_rate = 1e-10;
    }
    return INTERP_OK;
}
