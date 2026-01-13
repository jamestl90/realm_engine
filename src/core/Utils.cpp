#include "../../include/core/Utils.hpp"

namespace core {

std::optional<std::pair<int, int>> parseResolutionString(const std::string& str)
{
	auto xPos = str.find('x');
	if (xPos != std::string::npos)
	{
		auto widthString = str.substr(0, xPos);
		auto heightString = str.substr(xPos + 1, str.length());

		auto newEnd = std::remove(widthString.begin(), widthString.end(), ' ');
		widthString.erase(newEnd, widthString.end());

		newEnd = std::remove(heightString.begin(), heightString.end(), ' ');
		heightString.erase(newEnd, heightString.end());

		int width = std::stoi(widthString);
		int height = std::stoi(heightString);

		return std::make_optional(std::pair{ width, height });
	}
	return std::nullopt;
}

}