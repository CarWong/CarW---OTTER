#include "Utils/StringUtils.h"

std::string StringTools::SanitizeClassName(const std::string& name)
{
	const std::string classTag = "class ";
	const std::string structTag = "struct ";
	if (strncmp(name.c_str(), classTag.c_str(), classTag.size())) {
		return name.substr(6);
	}
	if (strncmp(name.c_str(), structTag.c_str(), structTag.size())) {
		return name.substr(7);
	}
	return name;
}

std::vector<std::string> StringTools::Split(const std::string& s, const std::string& splitOn) {
	std::vector<std::string> result;
	Split(s, result, splitOn);
	return result;
}

void StringTools::Split(const std::string& s, std::vector<std::string>& results, const std::string& splitOn)
{
	size_t splitLen = splitOn.size();
	size_t seek = s.find(splitOn, 0);
	size_t lastPos = 0;

	while (seek != std::string::npos) {
		results.push_back(s.substr(lastPos, seek - lastPos));
		lastPos = seek + splitLen;

		seek = s.find(splitOn, lastPos);
	}
	results.push_back(s.substr(lastPos, seek));
}
