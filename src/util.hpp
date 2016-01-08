/*
* Copyright (C) 2015-2016, Kevin Brubeck Unhammer <unhammer@fsfe.org>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


// Simple string convenience functions


#pragma once
#ifndef a309fa88d5af43e9_UTIL_H
#define a309fa88d5af43e9_UTIL_H


#include <sstream>
#include <iterator>

#include <vector>
#include <string>
#include <algorithm>

namespace gtd {

typedef std::vector<std::string> StringVec;

inline const std::string join_quoted(const StringVec vec, const std::string delim=" ") {
	std::ostringstream os;
	std::for_each(vec.begin(), vec.end(), [&](const std::string& s){ os << "\"" << s << "\","; });
	const auto& str = os.str();
	return str.substr(0,
			  str.size() - delim.size());
}

inline const std::string join(const StringVec vec, const std::string delim=" ") {
	std::ostringstream os;
	std::copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(os, delim.c_str()));
	const auto& str = os.str();
	return str.substr(0,
			  str.size() - delim.size());
}

inline const StringVec split(const std::string str, const char delim=' ')
{
	std::string buf;
	std::stringstream ss(str);
	StringVec tokens;
	while (std::getline(ss, buf, delim)) {
		if(!buf.empty()) {
			tokens.push_back(buf);
		}
	}
	return tokens;
}

inline int startswith(std::string big, std::string start)
{
	return big.compare(0, start.size(), start) == 0;
}


}

#endif
