#include "fileloader.h"

#include <xlnt/xlnt.hpp>
#include <filesystem>
#include <fstream>
#include <format>
#include "logging.h"
#include "utils.h"

namespace fs = std::filesystem;

// Function predefinitions
static bool s_CheckFile(const std::string& filename);
std::vector<std::vector<std::string>> s_LoadCSVSheet(const std::string& filename);
static std::vector<std::vector<std::string>> s_LoadExcelSheet(const std::string& filename);
static void s_SaveCSVSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite = false, const std::string& sourcefile = "");
static void s_SaveExcelSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite = false, const std::string& sourcefile = "");

static bool s_CheckFile(const std::string& filename) {
	try {
		// Converting filename to a path
		fs::path path = filename;
		fs::create_directories("sheets");
		xlnt::workbook wb;
		wb.load(path.wstring());
		wb.save("sheets/to_check.xlsx");
		wb.clear();
		wb.load("sheets/to_check.xlsx");
	}
	catch (std::exception& e) {
		logging::logwarning("FILELOADER::s_CheckFile Error Checking File: %s", e.what());
		return false;
	}
	return true;
}

std::vector<std::vector<std::string>> s_LoadCSVSheet(const std::string& filename) {
	std::vector<std::vector<std::string>> sheetData;
	// Converting filename to path
	fs::path path = fs::u8path(filename);
	// Checking if file exists
	if (!fs::exists(path)) {
		logging::logwarning("FILELOADER::s_LoadCSVSheet File does not exist: %s", filename.c_str());
		return sheetData;
	}
	try {
		std::ifstream file(path.wstring(), std::ios::binary);
		if (!file) {
			logging::logerror("FILELOADER:: s_LoadCSVSheet File is corrupted and could not be loaded into filestream: %s", filename.c_str());
			return sheetData;
		}
		std::string line;
		std::string separator = std::string(";");
		// Next two lines are NEEDED!!!! because the separator string does not only contain ';'
		separator.erase(0, separator.find_first_not_of(" \t\r\n"));
		separator.erase(separator.find_last_not_of(" \t\r\n") + 1);
		// Reading csv line by line
		int x = 0;
		while (std::getline(file, line)) {
			RemoveAllSubstrings(line, "\"");
			RemoveAllSubstrings(line, "\n");
			RemoveAllSubstrings(line, "\t");
			RemoveAllSubstrings(line, "\r");
			// first line often containes seperator
			if (x == 0 && line.starts_with("sep=")) {
				separator = Splitlines(line, "=").second;
				// Next two lines are NEEDED!!!! because the separator string does not only contain ';'
				separator.erase(0, separator.find_first_not_of(" \t\r\n"));
				separator.erase(separator.find_last_not_of(" \t\r\n") + 1);
				continue;
			}
			// Now generate the row
			std::vector<std::string> row;
			std::pair<std::string, std::string> values = Splitlines(line, separator);
			while (StrContains(values.second, separator)) {
				row.push_back(values.first);
				values = Splitlines(values.second, separator);
			}
			row.push_back(values.first);
			row.push_back(values.second);
			sheetData.push_back(row);
			x++;
		}
	}
	catch (std::exception& e) {
		logging::logerror("FILELOADER::s_LoadCSVSheet Could not load file: %s\nERROR: %s", filename.c_str(), e.what());
		return std::vector<std::vector<std::string>>();
	}
	return sheetData;
}

static std::vector<std::vector<std::string>> s_LoadExcelSheet(const std::string& filename) {
	std::vector<std::vector<std::string>> sheetData;
	// Converting filename to a path
	fs::path path = fs::u8path(filename);
	const std::string extension = path.filename().extension().string();
	if (extension == ".csv" || extension == ".CSV") {
		return s_LoadCSVSheet(filename);
	}
	// Checking if the file exists
	if (!fs::exists(path)) {
		logging::logwarning("FILELOADER::s_LoadExcelSheet File does not exist: %s", filename.c_str());
		return sheetData;
	}
	if (!s_CheckFile(path.string())) {
		logging::logwarning("FILELOADER::s_LoadExcelSheet Error loading file: %s", filename.c_str());
		return sheetData;
	}
	try {
		std::vector<std::vector<std::string>> testData;
		xlnt::workbook wb;
		fs::path to_load = path.string();
		wb.load(to_load.wstring());
		xlnt::worksheet ws = wb.active_sheet();
		// Loading each row
		for (auto rows : ws.rows(false)) {
			std::vector<std::string> rowdata;
			for (auto cell : rows) {
				std::string value = cell.to_string();
				if (IsNumber(value)) {
					std::replace(value.begin(), value.end(), '.', ',');
				}
				rowdata.push_back(value);
			}
			testData.push_back(rowdata);
		}
		sheetData = testData;
	}
	catch(std::exception & e) {
		logging::logerror("FILELOADER::s_LoadExcelSheet Error loading file: %s", e.what());
		sheetData.clear();
	}
	return sheetData;
}

static void s_SaveCSVSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite, const std::string& sourcefile) {
	fs::path path = fs::u8path(filename);
	std::ofstream file(path.wstring(), std::ios::binary);
	if (!file) {
		logging::logwarning("FILELOADER::s_SaveCSVSheet Could not open file: %s", filename.c_str());
		return;
	}
	file << "sep=;" << "\n";
	for (auto&& row : excelSheet) {
		for (int x = 0; x < row.size(); x++) {
			std::string val = row[x];
			if (x == 0) {
				if (IsInteger(val) || IsNumber(val)) {
					file << val;
				}
				else {
					file << '"' << val << '"';
				}
			}
			else {
				file << ';';
				if (IsInteger(val) || IsNumber(val)) {
					file << val;
				}
				else {
					file << '"' << val << '"';
				}
			}
		}
		file << '\n';
	}
}

static void s_SaveExcelSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite, const std::string& sourcefile) {
	fs::path path = fs::u8path(filename);
	const std::string extension = path.filename().extension().string();
	if (extension == ".csv" || extension == ".CSV") {
		s_SaveCSVSheet(filename, excelSheet, overwrite, sourcefile);
		return;
	}
	fs::path sourcepath = fs::u8path(sourcefile);
	xlnt::workbook wb;
	if (!overwrite) {
		if (!s_CheckFile(path.string())) {
			logging::logwarning("FILELOADER::s_SaveExcelSheet filechecking failure for: %s", filename.c_str());
			return;
		}
		wb.load(path.wstring());
	}
	if (sourcefile != "") {
		if (!s_CheckFile(path.string())) {
			logging::logwarning("FILELOADER::s_SaveExcelSheet filecheking failure for sourcefile: %s", sourcefile.c_str());
		}
		else {
			wb.load(sourcepath.wstring());
		}
	}
	xlnt::worksheet ws = wb.active_sheet();
	for (int x = 0; x < excelSheet.size(); x++) {
		for (int y = 0; y < excelSheet[x].size(); y++) {
			xlnt::cell_reference cell_ref = xlnt::cell_reference(y + 1, x + 1);
			auto dest_cell = ws.cell(cell_ref);
			// Check if it is a merged cell
			bool skip = false;
			for (const auto& range : ws.merged_ranges()) {
				if (range.contains(cell_ref) && cell_ref != range.top_left()) {
					skip = true;
					break;
				}
			}
			std::string value = excelSheet[x][y];
			//if (value == dest_cell.to_string())
				//skip = true;
			if (value == "")
				skip = true;
			if (skip)
				continue;
			// Asign cell integer value
			if (IsInteger(value)) {
				int number = std::stoi(value);
				dest_cell.value(number);
				continue;
			}
			// Asign cell float value
			// NOT WORKING CORRECTLY DUE TO PRECISION SHIT!!!
			// Maybe it deosnt matter and you should set precision for cells in excel??? idk
			/*
			if (s_IsNumber(value)) {
				replace(value.begin(), value.end(), ',', '.');
				std::string precision = "";
				float number = s_StringToFloat(value, &precision);
				dest_cell.value(value);
				continue;
			}*/
			// Asign cell value string
			dest_cell.value(value);
		}
	}
	wb.save(filename);
}

void FileInfo::Unload() {
	if (!IsReady())
		return;
	for (auto& rinfo : m_rowinfo) {
		rinfo.Unload();
	}
	m_rowinfo.clear();
	Settings->Unload();
	m_sheetData.clear();
	m_headerinfo.clear();
	m_filename = "";
	m_isready = false;
	m_headeridx = -1;
}

void FileInfo::LoadFile(const std::string& filename) {
	if (IsReady())
		Unload();
	m_sheetData.clear();
	m_rowinfo.clear();
	m_sheetData = s_LoadExcelSheet(filename);
	// Check if there is any data
	if (m_sheetData.size() <= 0)
		return;
	// Get header index
	int headerIndex = -1;
	int idx = 0;
	for (auto& row : m_sheetData) {
		idx++;
		if (row.size() <= 0)
			continue;
		if (row[0] == "DATA") {
			headerIndex = idx - 1;
		}
	}
	// Check if file is valid
	if (headerIndex < 0) {
		logging::logwarning("FILELOADER::FileInfo::LoadFile Loaded file does not contain 'DATA' in the 'A' Column\n Read the Documentation!");
		return;
	}
	// Processing headerinfo
	m_headeridx = headerIndex;
	for (int y = 1; y < m_sheetData[headerIndex].size(); y++) {
		std::pair<int, int> index = std::make_pair(headerIndex, y);
		std::string header = m_sheetData[headerIndex][y];
		header += " ##" + std::to_string(m_headerinfo.size());
		m_headerinfo.push_back(std::make_pair(header, index));
	}
	// Processing RowInfo
	for (int x = headerIndex + 1; x < m_sheetData.size(); x++) {
		const auto& row = m_sheetData[x];
		RowInfo rowinfo;
		bool dataSet = false;
		for (int y = 1; y < row.size(); y++) {
			const std::string& header = m_headerinfo[y-1].first;
			if (header == "")
				continue;
			std::string& value = m_sheetData[x][y];
			if (value != "")
				dataSet = true;
			if (StrContains(header, "Date")
				|| StrContains(header, "Datum")
				|| StrContains(header, "datum")
				|| StrContains(header, "date")) {
				if (IsNumber(value))
					value = ExcelSerialToDate(std::stoi(value));
			}
			rowinfo.AddData(header, value);
		}
		if(dataSet)
			m_rowinfo.push_back(rowinfo);
		else {
			break;
		}
	}
	Settings = new FileSettings();
	Settings->SetParentFile(this);
	m_filename = filename;
	m_isready = true;
}

void FileInfo::SaveFile() {
	s_SaveExcelSheet(m_filename, m_sheetData, true);
}

void FileInfo::SaveFileAs(const std::string& filename){
	s_SaveExcelSheet(filename, m_sheetData);
}

void FileInfo::SaveFileAs(const std::string& sourcefile, const std::string& destfile) {
	if (!IsReady()) {
		logging::logwarning("FILELOADER::FileInfo::SaveFileAs File was never loaded correctly. No Data to save");
		return;
	}
	fs::path source_path = fs::u8path(sourcefile);
	fs::path dest_path = fs::u8path(destfile);

	CreateSheetData();

	s_SaveExcelSheet(destfile, m_sheetData, true, sourcefile);
}

void FileInfo::CreateSheetData() {
	if (m_rowinfo.size() <= 0) {
		return;
	}
	if (m_sheetData.size() <= 0) {
		return;
	}
	size_t header_size = m_sheetData[m_headeridx].size();
	for (int x = 0; x < m_rowinfo.size(); x++) {
		const RowInfo& ri = m_rowinfo[x];
		auto&& rdata = ri.GetData();
		if (x + m_headeridx + 1 < m_sheetData.size()) {
			for (auto& pair : rdata) {
				int header_x = -1, header_y = -1;
				GetHeaderIndex(pair.first, &header_x, &header_y);
				if (header_x != m_headeridx)
					continue;
				if (header_y <= 0)
					continue;
				header_x += x + 1;
				m_sheetData[header_x][header_y] = pair.second;
			}
		}
		else {
			std::vector<std::string> rowinfo(m_headerinfo.size() + 1);
			for (auto& pair : rdata) {
				int header_x = -1, header_y = -1;
				GetHeaderIndex(pair.first, &header_x, &header_y);
				if (header_x != m_headeridx)
					continue;
				if (header_y <= 0)
					continue;
				rowinfo[header_y] = pair.second;
			}
			m_sheetData.push_back(rowinfo);
		}
	}
}

std::string FileInfo::GetFilename() const {
	return m_filename;
}

std::pair<int, int> FileInfo::GetHeaderIndex(const std::string& header) {
	for (auto&& pair : m_headerinfo) {
		if (pair.first == header) {
			return pair.second;
		}
	}
	return std::pair<int, int>(-1, -1);
}

void FileInfo::GetHeaderIndex(const std::string& header, int* x, int* y) {
	for (auto&& pair : m_headerinfo) {
		if (pair.first == header) {
			*x = pair.second.first;
			*y = pair.second.second;
			return;
		}
	}
}

std::vector<std::string> FileInfo::GetHeaderNames() const {
	std::vector<std::string> headernames;
	for (auto& pair : m_headerinfo) {
		headernames.push_back(pair.first);
	}
	return headernames;
}

RowInfo FileInfo::GetRowdata(const int rowIdx){
	if(rowIdx >= m_rowinfo.size())
		return RowInfo();
	return m_rowinfo[rowIdx];
}

std::vector<RowInfo> FileInfo::GetData() {
	return m_rowinfo;
}

void RowInfo::Unload() {
	m_rowinfo.clear();
	m_changed = false;
}

void FileInfo::SetRowData(const RowInfo& rowinfo, const int rowIdx){
	if (rowIdx >= m_rowinfo.size())
		return;
	m_rowinfo[rowIdx] = rowinfo;
}

void FileInfo::AddRowData(const RowInfo& rowinfo){
	m_rowinfo.push_back(rowinfo);
}

bool FileInfo::IsReady() const{
	return m_isready;
}

void RowInfo::AddData(const std::string& header, const std::string& value){
	auto it = std::find_if(m_rowinfo.begin(), m_rowinfo.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_rowinfo.end()) {
		m_rowinfo.push_back(std::make_pair(header, value));
		return;
	}
	it->second = value;
}

void RowInfo::UpdateData(const std::string& header, const std::string& newValue){
	auto it = std::find_if(m_rowinfo.begin(), m_rowinfo.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_rowinfo.end()) {
		return;
	}
	it->second = newValue;
	m_changed = true;
}

std::string RowInfo::GetData(const std::string& header) const{
	for (auto& p : m_rowinfo) {
		if (p.first == header)
			return p.second;
	}
	return "";
}

std::vector<std::pair<std::string, std::string>> RowInfo::GetData() const{
	return m_rowinfo;
}

void RowInfo::SetData(const std::vector<std::pair<std::string, std::string>>& data){
	m_rowinfo = data;
}

bool RowInfo::Changed() {
	bool tmp = m_changed;
	m_changed = false;
	return tmp;
}

void RowInfo::ResetChanged() {
	m_changed = false;
}

void FileSettings::Unload() {
	m_parentFile = nullptr;
	if(m_mergefileSet)
		m_mergefile.Unload();
	m_mergefileSet = false;
	m_mergeheaders.clear();
	m_mergeif = {};
	if (m_mergefolderfile.IsReady())
		m_mergefolderfile.Unload();
	m_mergefolder = "";
	m_mergefolderSet = false;
	m_mergefolderpaths.clear();
}

void FileSettings::SetMergeFile(const FileInfo otherFile) {
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFile m_parentFile is currently not set!");
		return;
	}
	if (!otherFile.IsReady()) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFile Given File is not a valid file.\n%s", otherFile.GetFilename());
		return;
	}
	m_mergefile = otherFile;
	m_mergefileSet = true;
}

FileInfo FileSettings::GetMergeFile() const {
	return m_mergefile;
}

void FileSettings::SetParentFile(FileInfo* parentFile) {
	if (!parentFile) {
		logging::logwarning("FILELOADER::FileSettings::SetParentFile Given File is a nullptr!");
		return;
	}
	m_parentFile = parentFile;
}

bool FileSettings::IsMergeFileSet() const{
	return m_mergefileSet;
}

void FileSettings::AddHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader) {
	if (destHeader != "" && !IsMergeFileSet()) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_mergefile is not set yet!");
		return;
	}
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_parentFile is not set yet!");
		return;
	}
	for (auto& pair : m_mergeheaders) {
		if (pair.first == sourceHeader) {
			pair.second = destHeader;
			return;
		}
	}
	m_mergeheaders.push_back(std::make_pair(sourceHeader, destHeader));
}

void FileSettings::SetMergeHeaderIf(const std::string& sourceHeader, const std::string& destHeader) {
	m_mergeif = std::make_pair(sourceHeader, destHeader);
}

void FileSettings::RemoveHeaderToMerge(const std::string& header) {
	auto it = std::find_if(m_mergeheaders.begin(), m_mergeheaders.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_mergeheaders.end()) {
		return;
	}
	m_mergeheaders.erase(it);
}

std::pair<std::string, std::string> FileSettings::GetMergeIf() const{
	return m_mergeif;
}


std::vector<std::pair<std::string, std::string>> FileSettings::GetMergeHeaders() const{
	return m_mergeheaders;
}

void FileSettings::MergeFiles() {
	if (IsMergeFolderSet() && IsMergeFolderTemplate()) {
		logging::loginfo("FILELOADER::FileSettings::MergeFiles merging all files from folder: %s", m_mergefolder.c_str());
		std::string cache = m_mergefolder + "/.cache";
		fs::path cachepath = fs::u8path(cache);
		std::ofstream cachefile(cachepath.wstring(), std::ios::binary);
		if (!cachefile) {
			logging::logwarning("FILELOADER::FileSettings::MergeFiles cannot cache filedata!\n%s", cache);
		}
		for (auto& path : m_mergefolderpaths) {
			FileInfo file;
			file.LoadFile(path);
			if (!file.IsReady()) {
				continue;
			}
			if (cachefile) {
				fs::path filep = fs::u8path(path);
				cachefile << cache << " : " << GetLastWriteTime(filep) << "\n";
			}
			std::vector<RowInfo>&& data = m_parentFile->GetData();
			std::vector<RowInfo>&& mergeData = file.GetData();
			if (m_mergefolderif.first == "") {
				for (auto& row : mergeData) {
					// Setting up a new row
					RowInfo newrow = data.back();
					for (auto&& header : m_parentFile->GetHeaderNames()) {
						newrow.UpdateData(header, "");
					}
					// Getting and updating
					bool dataset = false;
					for (auto& mergeheader : m_mergeheadersfolder) {
						const std::string value = row.GetData(mergeheader.second);
						if (value != "") {
							newrow.UpdateData(mergeheader.first, value);
							dataset = true;
						}
					}
					if (dataset) {
						m_parentFile->AddRowData(newrow);
					}
				}
			}
			else {
				int idx = -1;
				for (auto& row : data) {
					idx++;
					std::string value = row.GetData(m_mergefolderif.first);
					if (value == "")
						continue;
					for (auto& merge_row : mergeData) {
						std::string merge_value = merge_row.GetData(m_mergefolderif.second);
						if (merge_value == "")
							continue;
						if (merge_value != value)
							continue;
						for (auto& pair : m_mergeheadersfolder) {
							std::string new_val = merge_row.GetData(pair.second);
							if (new_val != "") {
								row.UpdateData(pair.first, new_val);
							}
						}
						break;
					}
					if (row.Changed()) {
						m_parentFile->SetRowData(row, idx);
					}
				}
			}
			file.Unload();
		}
	}
	if (!m_mergefile.IsReady())
		return;
	logging::loginfo("FILELOADER::FileSettings::MergeFiles Merging files\n\t%s\n\t%s\n\t And Searching for header: %s to fill with %s", m_parentFile->GetFilename().c_str(), m_mergefile.GetFilename().c_str(), m_mergeif.first.c_str(), m_mergeif.second.c_str());
	std::vector<RowInfo> &&data = m_parentFile->GetData();
	std::vector<RowInfo> &&mergeData = m_mergefile.GetData();
	int idx = -1;
	for (auto& row : data) {
		idx++;
		std::string value = row.GetData(m_mergeif.first);
		if (value == "")
			continue;
		for (auto& merge_row : mergeData) {
			std::string merge_value = merge_row.GetData(m_mergeif.second);
			if (merge_value == "")
				continue;
			if (merge_value != value)
				continue;
			for (auto& pair : m_mergeheaders) {
				std::string new_val = merge_row.GetData(pair.second);
				if (new_val != "") {
					row.UpdateData(pair.first, new_val);
				}
			}
			break;
		}
		if (row.Changed()) {
			m_parentFile->SetRowData(row, idx);
		}
	}
}

void FileSettings::SetMergeFolder(const std::string& folder, const bool ignoreCache) {
	fs::path path = fs::u8path(folder);

	try {
		if (!fs::exists(path)) {
			logging::loginfo("FILELOADER::FileSettings::SetMergeFolder Directory is not valid: %s", folder);
			return;
		}
		std::string cache = folder + "/.cache";
		fs::path cachepath = fs::u8path(cache);
		std::ifstream cachefile(cachepath.wstring(), std::ios::binary);
		std::vector<std::pair<std::string, std::string>> cachedData;
		std::vector<std::string> newcache;
		if (!ignoreCache && cachefile) {
			std::string line;
			while (std::getline(cachefile, line)) {
				RemoveAllSubstrings(line, "\n");
				std::pair<std::string, std::string> values = Splitlines(line, " : ");
				cachedData.push_back(values);
			}
		}
		for (const auto& entry : fs::directory_iterator(path)) {
			if (entry.is_regular_file()
				&& (entry.path().extension().string() == ".csv"
					|| entry.path().extension().string() == ".CSV"
					|| entry.path().extension().string() == ".xlsx"
					|| entry.path().extension().string() == ".XLSX")) {
				std::u8string u8str = entry.path().u8string();
				std::string strpath = std::string(u8str.begin(), u8str.end());
				for (auto& c : strpath) {
					if (c == '\\')
						c = '/';
				}
				bool add = true;
				for (auto& pair : cachedData) {
					if (pair.first == strpath && pair.second == GetLastWriteTime(entry.path())) {
						add = false;
						break;
					}
				}
				if (!add)
					continue;
				m_mergefolderpaths.push_back(strpath);
				logging::loginfo("FILELOADER::FileSettings::SetMergeFolder %s", m_mergefolderpaths.back().c_str());
			}
		}
	}
	catch (const fs::filesystem_error& e) {
		logging::logerror("FIELELOADER::FileSettings::SetMergeFolder Filesystem error: \n%s", e.what());
		return;
	}
	catch (const std::exception& e) {
		std::cerr << "Other error: " << e.what() << '\n';
		logging::logerror("FILELOADER::FileSettings::SetMergeFolder Error: \n%s", e.what());
		return;
	}
	m_mergefolder = folder;
	m_mergefolderSet = true;
}

std::string FileSettings::GetMergeFolder() const{
	return m_mergefolder;
}

bool FileSettings::IsMergeFolderSet() const{
	return m_mergefolderSet;
}

std::vector<std::string> FileSettings::GetMergeFolderPaths() const {
	return m_mergefolderpaths;
}

void FileSettings::SetMergeFolderTemplate(const std::string& filepath) {
	logging::loginfo("FILELOADER::FileSettings::SetMergeFolderTemplate %s", filepath.c_str());
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFolderTemplate m_parentFile is not set yet!");
	}
	if (m_mergefolderfile.IsReady()) {
		m_mergefolderif = {  };
		m_mergeheadersfolder.clear();
		m_mergefolderfile.Unload();
	}
	m_mergefolderfile.LoadFile(filepath);
	if (!m_mergefolderfile.IsReady()) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFolderTemplate could not load file properly: %s", filepath.c_str());
		m_mergefolderfile.Unload();
		m_mergefolderfile = FileInfo();
		return;
	}
	m_mergefolderfileSet = true;
}

FileInfo FileSettings::GetMergeFolderTemplate() const{
	return m_mergefolderfile;
}

bool FileSettings::IsMergeFolderTemplate() const{
	return m_mergefolderfileSet;
}

void FileSettings::AddFolderHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader) {
	if (destHeader != "" && !IsMergeFolderTemplate()) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_mergefoldertemplate is not set yet!");
		return;
	}
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_parentFile is not set yet!");
		return;
	}
	for (auto& pair : m_mergeheadersfolder) {
		if (pair.first == sourceHeader) {
			pair.second = destHeader;
			return;
		}
	}
	m_mergeheadersfolder.push_back(std::make_pair(sourceHeader, destHeader));
}

void FileSettings::SetMergeFolderHeaderIf(const std::string& sourceHeader, const std::string& destHeader) {
	m_mergefolderif = std::make_pair(sourceHeader, destHeader);
}

void FileSettings::RemoveFolderHeaderToMerge(const std::string& header) {
	auto it = std::find_if(m_mergeheadersfolder.begin(), m_mergeheadersfolder.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_mergeheadersfolder.end()) {
		return;
	}
	m_mergeheadersfolder.erase(it);
}

std::pair<std::string, std::string> FileSettings::GetMergeFolderIf() const {
	return m_mergefolderif;
}
std::vector<std::pair<std::string, std::string>> FileSettings::GetMergeFolderHeaders() const {
	return m_mergeheadersfolder;
}
