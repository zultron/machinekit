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
#include <vector>

struct ModalState {
    ModalState();
    std::vector<int> g_codes;       // array of active G codes
    std::vector<int> m_codes;      // array of active M codes
    std::vector<double> settings;     // array of feed, speed, etc.
    int work_offset_origin_index;
};
#endif
