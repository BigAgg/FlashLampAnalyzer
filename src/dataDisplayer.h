#pragma once

#include "fileloader.h"
#include <vector>

#define DEFAULT_INPUT_WIDTH 175.0f

enum DATA_DISPLAY_MODES {
	DATA_DISPLAY_MODE_NONE,
	DATA_DISPLAY_MODE_TOP_TO_BOT,
	DATA_DISPLAY_MODE_
};

void DisplayData(RowInfo &data, const unsigned int identifier, const std::string &mode = "vertical-rightheader", const std::vector<std::string>& hiddenHeaders = std::vector<std::string>());
void DisplayDataset(std::vector<RowInfo>& data, const std::string& mode = "vertical-rightheader", const std::vector<std::string>& hiddenHeaders = std::vector<std::string>());
void DisplayDataWithSettings(RowInfo& data, std::vector<std::pair<std::string, bool>>& settings);

