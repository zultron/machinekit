#ifndef MOCK_XYZA_CONFIG_HH
#define MOCK_XYZA_CONFIG_HH

void setup_xyza_config();
void setup_9axis_config();

void set_axis_position_limit(int axis_num, double max_limit, double min_limit);

#endif // MOCK_XYZA_CONFIG_HH
