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
#include "interp_base.hh"
#include "modal_state.hh"

ModalState::ModalState(): g_codes(ACTIVE_G_CODES,0),
    m_codes(ACTIVE_M_CODES,0),
    settings(ACTIVE_SETTINGS,0.0)
{}

//FIXME hard-code the initial vector size for now
ModalStateHistory::ModalStateHistory(): history(2000)
{}
