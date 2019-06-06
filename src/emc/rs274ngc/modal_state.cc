/********************************************************************
* Description: modal_state.cc
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
#include "modal_state.hh"
#include "interp_base.hh"

ModalState::ModalState(): g_codes(ACTIVE_G_CODES,0),
    m_codes(ACTIVE_M_CODES,0),
    settings(ACTIVE_SETTINGS,0.0),
    work_offset_origin_index(1)
{}
