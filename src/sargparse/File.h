#pragma once

#include "Parameter.h"

namespace sargp {

enum class File { Single, Multi };
inline auto completeFile(std::string extension = "", File file = File::Single) {
	return [file, extension](std::vector<std::string> const& c) -> std::pair<bool, std::set<std::string>> {
		if ((file == File::Single and c.size() > 1) or c.empty()) {
			return {true, {}};
		}
		return {false, {" -f " + extension}};
	};
}
inline auto completeDirectory(File file = File::Single) {
	return [file](std::vector<std::string> const& c) -> std::pair<bool, std::set<std::string>> {
		if ((file == File::Single and c.size() > 1) or c.empty()) {
			return {true, {}};
		}

		return {false, {" -d "}};
	};
}

}
