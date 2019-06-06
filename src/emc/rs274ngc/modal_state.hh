/********************************************************************
* Description: modal_state.hh
*
* State storage class for interpreter
*
* Author: Robert W. Ellenberg
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2014 All rights reserved.
*
********************************************************************/

#ifndef MODAL_STATE_HH
#define MODAL_STATE_HH
#include <array>
#include <interp_fwd.hh>
#include <interp_enums.hh>

struct ModalState {
    ModalState();
    ActiveGCodeArray g_codes;       // array of active G codes
    ActiveMCodeArray m_codes;      // array of active M codes
    ActiveSettingsArray settings;     // array of feed, speed, etc.
    int work_offset_origin_index;
};
#endif
