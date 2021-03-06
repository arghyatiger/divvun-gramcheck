/*
* Copyright (C) 2015-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#pragma once
#ifndef fe64e9a18486d375_SUGGEST_H
#define fe64e9a18486d375_SUGGEST_H

#include <algorithm>
#include <exception>
#include <locale>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

// divvun-gramcheck:
#include "util.hpp"
#include "hfst_util.hpp"
#include "json.hpp"
#include "checkertypes.hpp"
// xml:
#ifdef HAVE_LIBPUGIXML
#include <pugixml.hpp>
#endif
// hfst:
#include <hfst/HfstInputStream.h>
#include <hfst/HfstTransducer.h>
// variants:
#include "mapbox/variant.hpp"

namespace divvun {

using mapbox::util::variant;
using std::string;
using std::stringstream;
using std::u16string;
using std::pair;
using std::vector;

using UStringVector = vector<u16string>;

using MsgMap = std::unordered_map<Lang, pair<ToggleIds, ToggleRes> >;	// msgs[Lang] = make_pair(ToggleIds, ToggleRes)
using SortedMsgLangs = vector<Lang>; // sorted with preferred language first

inline string xml_raw_cdata(const pugi::xml_node& label) {
	std::ostringstream os;
	for(const auto& cc: label.children())
	{
		cc.print(os, "", pugi::format_raw);
	}
	return os.str();
}

enum RunState {
	flushing,
	eof
};

using rel_id = size_t;
using relations = std::unordered_map<string, rel_id>;

enum Added { NotAdded, AddedAfterBlank, AddedBeforeBlank };

struct Reading {
	bool suggest = false;
	string ana;	   // for generating suggestions from this reading
	u16string errtype; // the error tag (without leading ampersand)
	UStringVector sforms;
	relations rels;	// rels[relname] = target.id
	rel_id id = 0; // id is 0 if unset, otherwise the relation id of this word
	string wf;
	bool suggestwf = false;
	bool link = false; // cohorts that are not the "core" of the underline never become Err's; message template offsets refer to the cohort of the Err
	Added added = NotAdded;
};

struct Cohort {
	u16string form;
	size_t pos;
	rel_id id;
	vector<Reading> readings;
	std::set<u16string> errtypes;
	Added added;
};

using CohortMap = std::unordered_map<rel_id, size_t>;

struct Sentence {
	vector<Cohort> cohorts;
	CohortMap ids_cohorts;
	// TODO: can we make this an encoded stringstream? would avoid a lot of from/to_bytes calls
	// std::basic_ostringstream<char16_t> text;
	std::ostringstream text;
	RunState runstate;
};

class Suggest {
	public:
		Suggest(const hfst::HfstTransducer* generator, divvun::MsgMap msgs, const string& locale, bool verbose);
		Suggest(const string& gen_path, const string& msg_path, const string& locale, bool verbose);
		Suggest(const string& gen_path, const string& locale, bool verbose);
		~Suggest() = default;

		void run(std::istream& is, std::ostream& os, bool json);

		vector<Err> run_errs(std::istream& is);
		void setIgnores(const std::set<ErrId>& ignores);

		static const MsgMap readMessages(const string& file);
		static const MsgMap readMessages(const char* buff, const size_t size);

		const MsgMap msgs;
		const string locale;
	private:
		const SortedMsgLangs sortedmsglangs; // invariant: contains all and only the keys of msgs
		RunState run_json(std::istream& is, std::ostream& os);
		std::unique_ptr<const hfst::HfstTransducer> generator;
		std::set<ErrId> ignores;
		variant<Nothing, Err> cohort_errs(const ErrId& ErrId, const Cohort& c, const Sentence& sentence, const u16string& text);
		vector<Err> mk_errs(const Sentence &sentence);
};

}

#endif
