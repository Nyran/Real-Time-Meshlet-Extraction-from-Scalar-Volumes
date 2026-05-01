#ifndef FRAMEWORK_UTILS_H
#define FRAMEWORK_UTILS_H

#include "framework_platform.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <string>


	namespace framework::util {
		std::string FRAMEWORK_DLL_EXPORT split_volume_filename(std::string const& string_to_split, std::map<std::string, uint64_t>& tokens);
		std::string FRAMEWORK_DLL_EXPORT strip_whitespace(std::string const& in_string);
	} // util 
// framework

#endif //FRAMEWORK_UTILS_H