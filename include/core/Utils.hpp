#pragma once

#include <string>
#include <optional>

namespace core {

	std::optional<std::pair<int, int>> parseResolutionString(const std::string& str);
}