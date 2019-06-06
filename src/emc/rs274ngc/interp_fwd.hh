/**
 * @file interp_fwd.hh
 * 
 * Forward declarations for interp_internal.hh.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * Copyright (c) 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#ifndef INTERP_FWD_HH
#define INTERP_FWD_HH

struct block_struct;
typedef struct block_struct *block_pointer;
typedef struct block_struct block;

struct setup_struct;
typedef struct setup_struct *setup_pointer;
typedef struct setup_struct setup;

struct remap_struct;
typedef struct remap_struct *remap_pointer;
typedef struct remap_struct remap;

struct context_struct;
typedef struct context_struct *context_pointer;
typedef struct context_struct context;

struct parameter_value_struct;
typedef parameter_value_struct *parameter_pointer;
typedef parameter_value_struct parameter_value;

struct offset_struct;
typedef offset_struct *offset_pointer;
typedef offset_struct offset;

// feed_mode
enum FEED_MODE {
    UNITS_PER_MINUTE=0,
    INVERSE_TIME=1,
    UNITS_PER_REVOLUTION=2
};

// cutter radius compensation mode, 0 or false means none
// not using CANON_SIDE since interpreter handles cutter radius comp
enum CUTTER_COMP_DIRECTION {
    RIGHT = 1,
    LEFT = 2,
};

// spindle control modes
enum SPINDLE_MODE {
    CONSTANT_RPM,
    CONSTANT_SURFACE
};

// unary operations
// These are not enums because the "&" operator is used in
// reading the operation names and is illegal with an enum

enum UnaryOperations
{
    ABS = 1,
    ACOS = 2,
    ASIN = 3,
    ATAN = 4,
    COS = 5,
    EXP = 6,
    FIX = 7,
    FUP = 8,
    LN = 9,
    ROUND = 10,
    SIN = 11,
    SQRT = 12,
    TAN = 13,
    EXISTS = 14,
};


// binary operations
enum BinaryOperations
{
    NO_OPERATION = 0,
    DIVIDED_BY = 1,
    MODULO = 2,
    POWER = 3,
    TIMES = 4,
    AND2 = 5,
    EXCLUSIVE_OR = 6,
    MINUS = 7,
    NON_EXCLUSIVE_OR = 8,
    PLUS = 9,
    RIGHT_BRACKET = 10,
    /* relational operators (are binary operators)*/
    LT = 11,
    EQ = 12,
    NE = 13,
    LE = 14,
    GE = 15,
    GT = 16,
    RELATIONAL_OP_FIRST = 11,
    RELATIONAL_OP_LAST = 16,
};

// O code
enum OCodes
{
    O_none = 0,
    O_sub = 1,
    O_endsub = 2,
    O_call = 3,
    O_do = 4,
    O_while = 5,
    O_if = 6,
    O_elseif = 7,
    O_else = 8,
    O_endif = 9,
    O_break = 10,
    O_continue = 11,
    O_endwhile = 12,
    O_return = 13,
    O_repeat = 14,
    O_endrepeat = 15,
    M_98 = 16,
    M_99 = 17,
    O_ = 18,
};

// G Codes are symbolic to be dialect-independent in source code
enum GCodes
{
    G_0 = 0,
    G_1 = 10,
    G_2 = 20,
    G_3 = 30,
    G_4 = 40,
    G_5 = 50,
    G_5_1 = 51,
    G_5_2 = 52,
    G_5_3 = 53,
    G_7 = 70,
    G_8 = 80,
    G_10 = 100,
    G_17 = 170,
    G_17_1 = 171,
    G_18 = 180,
    G_18_1 = 181,
    G_19 = 190,
    G_19_1 = 191,
    G_20 = 200,
    G_21 = 210,
    G_28 = 280,
    G_28_1 = 281,
    G_30 = 300,
    G_30_1 = 301,
    G_33 = 330,
    G_33_1 = 331,
    G_38_2 = 382,
    G_38_3 = 383,
    G_38_4 = 384,
    G_38_5 = 385,
    G_38_6 = 386,
    G_40 = 400,
    G_41 = 410,
    G_41_1 = 411,
    G_42 = 420,
    G_42_1 = 421,
    G_43 = 430,
    G_43_1 = 431,
    G_43_2 = 432,
    G_49 = 490,
    G_50 = 500,
    G_51 = 510,
    G_52 = 520,
    G_53 = 530,
    G_54 = 540,
    G_54_1 = 541,
    G_55 = 550,
    G_56 = 560,
    G_57 = 570,
    G_58 = 580,
    G_59 = 590,
    G_59_1 = 591,
    G_59_2 = 592,
    G_59_3 = 593,
    G_61 = 610,
    G_61_1 = 611,
    G_64 = 640,
    G_73 = 730,
    G_76 = 760,
    G_80 = 800,
    G_81 = 810,
    G_82 = 820,
    G_83 = 830,
    G_84 = 840,
    G_85 = 850,
    G_86 = 860,
    G_87 = 870,
    G_88 = 880,
    G_89 = 890,
    G_90 = 900,
    G_90_1 = 901,
    G_91 = 910,
    G_91_1 = 911,
    G_92 = 920,
    G_92_1 = 921,
    G_92_2 = 922,
    G_92_3 = 923,
    G_93 = 930,
    G_94 = 940,
    G_95 = 950,
    G_96 = 960,
    G_97 = 970,
    G_98 = 980,
    G_99 = 990,
};
#endif // INTERP_FWD_HH
