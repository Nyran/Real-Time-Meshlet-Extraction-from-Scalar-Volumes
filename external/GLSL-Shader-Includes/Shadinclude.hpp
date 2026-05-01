#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

//	===========
//	Shadinclude
//	===========
/*
LICENCE
MIT License

Copyright (c) [2018] [Tahar Meijs]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

INTRODUCTION
The sole purpose of this class is to load a file and extract the text that is in it.
In theory, this class could be used for a variety of text-processing purposes, but
it was initially designed to be used to load shader source code.

USING THIS CLASS
Since the entire class is a static class, you only have to add this single line of
code to your project:

--------------------------------------------------------------------------------------
std::string shaderSource = Shadinclude::load("./path/to/shader.extension");
--------------------------------------------------------------------------------------

This will (recursively) extract the source code from the first shader file.
Now, you might be wondering, what is the point of using your code for something
so trivial as to loading a file and calling the "std::getline()" function on it?

Well, besides loading the shader source code from a single file, the loader also
supports custom keywords that allow you to include external files inside your
shader source code!

PARAMETERS OF THE LOAD FUNCTION
- std::string	path				path to the "main" shader file
- std::string	includeIdentifier		keyword to look for when scanning for files

MISCELLANEOUS
- Author	:	Tahar Meijs
- Date		:	10th - 12th of April 2018
- Language	:	C++ (can easily be converted into other languages)
*/





class Shadinclude
{
public:
	// Return the source code of the complete shader
	static std::string load(std::string path, std::filesystem::file_time_type& freshest_file_time, std::string includeIdentifier = "#include ")
	{
		//includeIdentifier += ' ';
		static bool isRecursiveCall = false;

		std::string fullSourceCode = "";

		if (std::filesystem::exists(path)) {
			auto const current_file_write_time = std::filesystem::last_write_time(path);
			freshest_file_time = std::max(freshest_file_time, current_file_write_time);
		}

		std::ifstream file(path);



		if (!file.is_open())
		{
			std::cerr << "ERROR: could not open the shader at: " << path << "\n" << std::endl;
			return fullSourceCode;
		}

		std::string lineBuffer;
		while (std::getline(file, lineBuffer))
		{

			std::string line_to_test = lineBuffer;
			trim(line_to_test);
			// Look for the new shader include identifier
			if (line_to_test.find(includeIdentifier) != line_to_test.npos)
			{
				// Remove the include identifier, this will cause the path to remain
				line_to_test.erase(0, includeIdentifier.size());


				// Remove quotation marks from the include-string, in case there are any
				line_to_test.erase(std::remove(line_to_test.begin(), line_to_test.end(), '\"'), line_to_test.end());

				// The include path is relative to the current shader file path
				std::string pathOfThisFile;
				getFilePath(path, pathOfThisFile);
				line_to_test.insert(0, pathOfThisFile);

				// By using recursion, the new include file can be extracted
				// and inserted at this location in the shader source code
				isRecursiveCall = true;
				fullSourceCode += load(line_to_test, freshest_file_time);

				// Do not add this line to the shader source code, as the include
				// path would generate a compilation issue in the final source code
				continue;
			}

			fullSourceCode += lineBuffer + '\n';	
		}

		// Only add the null terminator at the end of the complete file,
		// essentially skipping recursive function calls this way
		if (!isRecursiveCall)
			fullSourceCode += '\0';

		file.close();

		return fullSourceCode;
	}

private:
	static void getFilePath(const std::string & fullPath, std::string & pathWithoutFileName)
	{
		// Remove the file name and store the path to this folder
		size_t found = fullPath.find_last_of("/\\");
		pathWithoutFileName = fullPath.substr(0, found + 1);
	}

	static bool isWhitespace(char ch) {
		return std::isspace(static_cast<unsigned char>(ch)) ||
			ch == '\u00A0' || // non-breaking space
			ch == '\u2007' || // figure space
			ch == '\u202F';   // narrow no-break space
	}

	// Trim from start (in place)
	static void ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			[](unsigned char ch) { return !isWhitespace(ch); }));
	}

	// Trim from end (in place)
	static void rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(),
			[](unsigned char ch) { return !isWhitespace(ch); }).base(), s.end());
	}

	// Trim from both ends (in place)
	static void trim(std::string& s) {
		ltrim(s);
		rtrim(s);
	}

};
