#ifndef FEATURESET_HH
#define FEATURESET_HH

#include <bitset>

enum Features {
    FEATURE_RETAIN_G43,
    FEATURE_OWORD_N_ARGS,
    FEATURE_INI_VARS,
    FEATURE_HAL_PIN_VARS,
    FEATURE_NO_DOWNCASE_OWORD,
    FEATURE_OWORD_WARNONLY,
    FEATURE_LATHE_TOOLCHANGE,
    FEATURE_EXTENDED_ERROR_MESSAGES,
    MAX_FEATURES
};

using FeatureSet = std::bitset<64>;

inline bool has_feature(FeatureSet features, Features f)
{
    switch (f) {
    case FEATURE_RETAIN_G43:
    case FEATURE_OWORD_N_ARGS:
    case FEATURE_INI_VARS:
    case FEATURE_HAL_PIN_VARS:
    case FEATURE_NO_DOWNCASE_OWORD:
    case FEATURE_OWORD_WARNONLY:
    case FEATURE_LATHE_TOOLCHANGE:
    case FEATURE_EXTENDED_ERROR_MESSAGES:
        return features.test((int)f);
    case MAX_FEATURES:
        return false;
    }
    return false;
}
#endif // FEATURESET_HH
