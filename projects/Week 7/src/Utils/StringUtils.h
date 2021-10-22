#pragma once
#include <string>
#include <algorithm>
#include <vector>

// Borrowed from https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
int constexpr const_strlen(const char* str) {
	return *str ? 1 + const_strlen(str + 1) : 0;
}
	
class StringTools {
public:
	static std::string SanitizeClassName(const std::string& name);
	// trim from start (in place)
	static inline void LTrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
		}));
	}

	// trim from end (in place)
	static inline void RTrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void Trim(std::string& s) {
		LTrim(s);
		RTrim(s);
	}

	// trim from start (in place)
	static inline void LTrim(std::string& s, char toTrim) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [=](int ch) {
			return ch != toTrim;
		}));
	}

	// trim from end (in place)
	static inline void RTrim(std::string& s, char toTrim) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [=](int ch) {
			return ch != toTrim;;
		}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void Trim(std::string& s, char toTrim) {
		LTrim(s, toTrim);
		RTrim(s, toTrim);
	}

	static inline void ToLower(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
			return std::tolower(c);
		});
	}

	static inline void ToUpper(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
			return std::toupper(c);
		});
	}

	static std::vector<std::string> Split(const std::string& s, const std::string& splitOn = "");
	static void Split(const std::string& s, std::vector<std::string>& results, const std::string& splitOn = "");
};