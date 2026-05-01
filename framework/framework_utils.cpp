#include "framework_utils.hpp"

#include <glm/glm.hpp>

#include <iostream>
#include <regex>
#include <sstream>
#include <vector>
#include <cstring>


    namespace framework::util {

        std::string split_volume_filename(std::string const& string_to_split, std::map<std::string, uint64_t>& tokens) {
            size_t file_starting_pos = string_to_split.find_last_of("/") + 1;
            std::string file_name = string_to_split.substr(file_starting_pos);
            // expects something like volume_w256_h256_d256_c1_b16.raw
            std::string extension_removed_string = file_name.substr(0, file_name.find(".raw"));
            
            // split the numbers and rest of file name into capture groups for easy conversion
            // klacansky - open sci-vis dataset format:
            // volume_256x256x256_uint16.raw
            std::regex klacansky_re("(.*)_([[:digit:]]+)x([[:digit:]]+)x([[:digit:]]+)_(uint|float)([[:digit:]]+)");
            std::regex non_negative_number_regex("[[:digit:]]+");
            std::smatch matches;
            // check if file has klacansky naming (https://klacansky.com/open-scivis-datasets/)
            // and rewrite to the framework's `_w_h_d_b` token naming on the fly
            if (std::regex_match(extension_removed_string, matches, klacansky_re)) {
                std::string const datatype = "_b";
                extension_removed_string = matches[1].str() + "_w" + matches[2].str() + "_h" + matches[3].str() + "_d" + matches[4].str() + datatype + matches[6].str();
            }

            std::stringstream test(extension_removed_string);
            std::string segment;

            std::vector<std::string> registered_tokens;

            // tokens to look for in the volume string
            registered_tokens.emplace_back("w");
            registered_tokens.emplace_back("h");;
            registered_tokens.emplace_back("d");
            registered_tokens.emplace_back("c");
            registered_tokens.emplace_back("b");
            registered_tokens.emplace_back("t");
            //registered_tokens.push_back("f");

            std::size_t first_valid_volume_descriptor_token_position = std::numeric_limits<std::size_t>::max();

            while (std::getline(test, segment, '_')) {
                for (auto const& potentially_matching_token : registered_tokens) {
                    if (segment.find(potentially_matching_token) == 0) {
                        uint64_t length_of_token = potentially_matching_token.size();
                        std::string const remaining_string = segment.substr(length_of_token);

                        bool regex_match_result = std::regex_match(remaining_string, non_negative_number_regex);
                        if (regex_match_result) {
                            tokens[potentially_matching_token] = std::atoi(remaining_string.c_str());

                            std::size_t const token_pos = file_name.find("_" + potentially_matching_token);
                            first_valid_volume_descriptor_token_position = std::min(first_valid_volume_descriptor_token_position, token_pos);
                            break;
                        }
                    }
                }
            }

            std::string base_volume_file_path = file_name.substr(0, first_valid_volume_descriptor_token_position);

            return base_volume_file_path;
        }

        std::string
            strip_whitespace(std::string const& in_string) {
            return std::regex_replace(in_string, std::regex("^ +| +$|( ) +"), "$1");
        }

        //checks for prefix AND removes it (+ whitespace) if it is found; 
        //returns true, if prefix was found; else false
        bool
            parse_prefix(std::string& in_string, std::string const& prefix) {

            size_t num_prefix_characters = prefix.size();

            bool prefix_found
                = (!(in_string.size() < num_prefix_characters)
                    && strncmp(in_string.c_str(), prefix.c_str(), num_prefix_characters) == 0);

            if (prefix_found) {
                in_string = in_string.substr(num_prefix_characters);
                in_string = strip_whitespace(in_string);
            }

            return prefix_found;
        }

    } // util
// framework