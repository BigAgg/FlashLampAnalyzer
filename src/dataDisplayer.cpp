#include "dataDisplayer.h"
#include <imgui.h>
#include "utils.h"
#include "ui_helper.h"


void DisplayData(RowInfo &data, const unsigned int identifier, const std::string &mode, const std::vector<std::string> &hiddenHeaders) {
	// Drawing vertical with headers on right side
	if (mode == "vertical-rightheader") {
		int headerfix = 0;
		for (auto& rdata : data.GetData()) {
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				if (*it == rdata.first)
					continue;
			}
			std::string label = rdata.first + " ## " + std::to_string(identifier) + std::to_string(headerfix);
			std::string value = rdata.second;
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if(ImGui::InputStringWithHint(value, label, headersplit.c_str()))
				data.UpdateData(rdata.first, value);
			ImGui::SetItemTooltip(headersplit.c_str());
			headerfix++;
		}
		ImGui::Separator();
	}
	// Drawing vertical with headers on left side
	else if (mode == "vertical-leftheader") {
		int headerfix = 0;
		for (auto& rdata : data.GetData()) {
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				if (*it == rdata.first)
					continue;
			}
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			std::string label = "## " + headersplit + std::to_string(identifier) + std::to_string(headerfix);
			std::string value = rdata.second;
			ImGui::Text("%s", headersplit.c_str());
			ImGui::SameLine();
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			label = "## " + label;
			if (ImGui::InputStringWithHint(value, label, headersplit.c_str()))
				data.UpdateData(rdata.first, value);
			ImGui::SetItemTooltip(headersplit.c_str());
			headerfix++;
		}
		ImGui::Separator();
	}
	// Drawing horizontal with headers above
	else if (mode == "horizontal-aboveheader") {
		auto&& dataset = data.GetData();
		int headerfix = 0;
		//ImGui::BeginChild("horizontal-aboveheader-dataDisplayer", { dataset.size() * DEFAULT_INPUT_WIDTH + DEFAULT_INPUT_WIDTH, dataset.size() * 50.0f });
		for (auto& rdata : dataset) {
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				continue;
			}
			if (rdata != dataset.front() && headerfix > 0)
				ImGui::SameLine();
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			std::string label = "## " + headersplit + std::to_string(identifier) + std::to_string(headerfix);
			std::string childname = label + "_child" + std::to_string(identifier);
			std::string value = rdata.second;
			ImGui::BeginChild(childname.c_str(), {DEFAULT_INPUT_WIDTH, 50.0f});
			ImGui::Text("%s", headersplit.c_str());
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if (ImGui::InputStringWithHint(value, label, headersplit.c_str())) {
				data.UpdateData(rdata.first, value);
			}
			ImGui::SetItemTooltip(headersplit.c_str());
			ImGui::EndChild();
			headerfix++;
		}
		ImGui::Separator();
		//ImGui::EndChild();
	}
	// Drawing horizontal without headers (Should be drawn before this)
	else if (mode == "horizontal-noheader") {
		auto&& dataset = data.GetData();
		int headerfix = 0;
		//ImGui::BeginChild("horizontal-aboveheader-dataDisplayer", { dataset.size() * DEFAULT_INPUT_WIDTH + DEFAULT_INPUT_WIDTH, dataset.size() * 50.0f });
		for (auto& rdata : dataset) {
			auto it = std::find(hiddenHeaders.begin(), hiddenHeaders.end(), rdata.first);
			if (it != hiddenHeaders.end()) {
				continue;
			}
			if (rdata != dataset.front() && headerfix > 0)
				ImGui::SameLine();
			std::string headersplit = Splitlines(rdata.first, " ##").first;
			std::string label = "## " + headersplit + std::to_string(identifier) + std::to_string(headerfix);
			std::string childname = label + "_child" + std::to_string(identifier);
			std::string value = rdata.second;
			ImGui::BeginChild(childname.c_str(), {DEFAULT_INPUT_WIDTH, 25.0f});
			ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
			if (ImGui::InputStringWithHint(value, label, headersplit.c_str())) {
				data.UpdateData(rdata.first, value);
			}
			ImGui::SetItemTooltip(headersplit.c_str());
			ImGui::EndChild();
			headerfix++;
		}
		ImGui::Separator();
	}

}

void DisplayDataset(std::vector<RowInfo>& data, const std::string& mode, const std::vector<std::string>& hiddenHeaders) {
	int idx = 0;
	for (auto& rowinfo : data) {
		DisplayData(rowinfo, idx, mode, hiddenHeaders);
		idx++;
	}
}