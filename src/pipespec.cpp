/*
 * Copyright (C) 2017-2018, Kevin Brubeck Unhammer <unhammer@fsfe.org>
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

#include <cstdlib>

#include "pipespec.hpp"

namespace divvun {

PipeSpec::PipeSpec(const string& file) {
	pugi::xml_parse_result result = doc.load_file(file.c_str());
	if (result) {
		language = doc.child("pipespec").attribute("language").value();
		if(language == "") {
			language = "se"; // reasonable default
		}
		for (pugi::xml_node pipeline: doc.child("pipespec").children("pipeline")) {
			const u16string& pipename = fromUtf8(pipeline.attribute("name").value());
			if(default_pipe.empty()) {
				// The first one is the default:
				default_pipe = pipename;
			}
			auto pr = std::make_pair(pipename, pipeline);
			pnodes[pipename] = pipeline;
		}
	}
	else {
		std::cerr << file << ":" << result.offset << " ERROR: " << result.description() << "\n";
		throw std::runtime_error("libdivvun: ERROR: Couldn't load the pipespec xml \"" + file + "\"");
	}
}


PipeSpec::PipeSpec(pugi::char_t* buff, size_t size) {
	pugi::xml_parse_result result = doc.load_buffer(buff, size);
	if (result) {
		language = doc.child("pipespec").attribute("language").value();
		if(language == "") {
			language = "se"; // reasonable default
		}
		for (pugi::xml_node pipeline: doc.child("pipespec").children("pipeline")) {
			const u16string& pipename = fromUtf8(pipeline.attribute("name").value());
			if(default_pipe.empty()) {
				// The first one is the default:
				default_pipe = pipename;
			}
			auto pr = std::make_pair(pipename, pipeline);
			pnodes[pipename] = pipeline;
		}
	}
	else {
		std::cerr << "pipespec.xml:" << result.offset << " ERROR: " << result.description() << "\n";
		throw std::runtime_error("libdivvun: ERROR: Couldn't load the pipespec.xml from archive");
	}
}

// TODO: return optional string message instead?
void validatePipespecCmd(const pugi::xml_node& cmd, const std::unordered_map<string, string>& args) {
	const string& name = cmd.name();
	if(name == "tokenise" || name == "tokenize") {
		if(args.size() != 1 || args.find("tokenizer") == args.end()) {
			throw std::runtime_error("Wrong arguments to <tokenize> command (expected <tokenizer>), at byte offset " + std::to_string(cmd.offset_debug()));
		}
	}
	else if(name == "cg") {
		if(args.size() != 1 || args.find("grammar") == args.end()) {
			throw std::runtime_error("Wrong arguments to <cg> command (expected <grammar>), at byte offset " + std::to_string(cmd.offset_debug()));
		}
	}
	else if(name == "cgspell") {
		if(args.size() != 2 || args.find("lexicon") == args.end() || args.find("errmodel") == args.end()) {
			throw std::runtime_error("Wrong arguments to <cgspell> command (expected <lexicon> and <errmodel>), at byte offset " + std::to_string(cmd.offset_debug()));
		}
	}
	else if(name == "mwesplit") {
		if(args.size() != 0) {
			throw std::runtime_error("Wrong arguments to <mwesplit> command (expected none), at byte offset " + std::to_string(cmd.offset_debug()));
		}
	}
	else if(name == "blanktag") {
		if(args.size() != 1 || args.find("blanktagger") == args.end()) {
			throw std::runtime_error("Wrong arguments to <blanktag> command (expected <blanktagger>), at byte offset " + std::to_string(cmd.offset_debug()));
		}
	}
	else if(name == "suggest") {
		if(args.size() != 2 || args.find("generator") == args.end() || args.find("messages") == args.end()) {
			throw std::runtime_error("Wrong arguments to <suggest> command (expected <generator> and <messages>), at byte offset " + std::to_string(cmd.offset_debug()));
		}
	}
	else if(name == "sh") {
		throw std::runtime_error("<sh> command not implemented yet!");
		// const auto& prog = fromUtf8(cmd.attribute("prog").value());
	}
	else if(name == "prefs") {
		// pass
	}
	else {
		throw std::runtime_error("Unknown command '" + name + "'");
	}
}

string makeDebugSuff(string name, std::unordered_map<string, string> args) {
	if(name == "cg" && args.find("grammar") != args.end()) {
		if(args["grammar"].substr(0, 14) == "grammarchecker") {
			return "gc";
		}
		if(args["grammar"].substr(0, 7) == "mwe-dis") {
			return "mwe-dis";
		}
		if(args["grammar"].substr(0, 7) == "valency") {
			return "val";
		}
		if(args["grammar"].substr(0, 13) == "disambiguator") {
			return "disam";
		}
	}
	if(name == "tokenize" || name == "tokenise") {
		return "morph";
	}
	if(name == "suggest") {
		return "suggest";
	}
	if(name == "mwesplit") {
		return "mwe-split";
	}
	if(name == "blanktag") {
		return "blanktag";
	}
	if(name == "cgspell") {
		return "spell";
	}
	return name;
}

string abspath(const string& path) {
	// TODO: would love to use <experimental/filesystem> here, but doesn't exist on Mac :(
	// return std::experimental::filesystem::absolute(specfile).remove_filename();
        char resolved_path[PATH_MAX];
	if(realpath(path.c_str(), resolved_path) == NULL) {
		throw std::runtime_error("libdivvun: ERROR: realpath(3) failed to resolve '" + path + "', got: " + std::strerror(errno));
	}
	return resolved_path;
}

string pathconcat(const string& path1, const string& path2) {
	// TODO: would love to use <experimental/filesystem> here, but doesn't exist on Mac :(
	// return path1 / path2;
	if(path2.length() > 0 && path2[0] == '/') {
		return path2;
	}
	if(path1.length() > 0 && path1[path1.length() - 1] == '/') {
		return path1 + path2;
	}
	else {
		// TODO: \\ on Windows
		return path1 + "/" + path2;
	}
}

string argprepare(const string& dir, string file) {
	// Wrap the whole thing in single-quotes, but put existing single-quotes in double-quotes
	replaceAll(file, "'", "'\"'\"'");
	return " '" + pathconcat(dir, file) + "'";
}

vector<std::pair<string,string>> toPipeSpecShVector(const string& dir, const PipeSpec& spec, const u16string& pipename, bool trace, bool json) {
	vector<std::pair<string, string>> cmds = {};
	for (const pugi::xml_node& cmd: spec.pnodes.at(pipename).children()) {
		const auto& name = string(cmd.name());
		string prog;
		std::unordered_map<string, string> args;
		for (const pugi::xml_node& arg: cmd.children()) {
			args[arg.name()] = arg.attribute("n").value();
		}
		validatePipespecCmd(cmd, args);
		if(name == "tokenise" || name == "tokenize") {
			prog = "hfst-tokenise -g";
			if(json) {
				prog += " -S";
			}
			prog += argprepare(dir, args["tokenizer"]);
		}
		else if(name == "cg") {
			prog = "vislcg3";
			if(trace) {
				prog += " --trace";
			}
			if(json) {
				prog += " --quiet";
			}
			prog += " -g" + argprepare(dir, args["grammar"]);
		}
		else if(name == "cgspell") {
			prog = "divvun-cgspell -n 25";
			prog += " -l" + argprepare(dir, args["lexicon"]);
			prog += " -m" +  argprepare(dir, args["errmodel"]);
		}
		else if(name == "mwesplit") {
			prog = "cg-mwesplit";
		}
		else if(name == "blanktag") {
			prog = "divvun-blanktag" + argprepare(dir, args["blanktagger"]);
		}
		else if(name == "suggest") {
			prog = "divvun-suggest";
			if(json) {
				prog += " --json";
			}
			prog += " -g" + argprepare(dir, args["generator"]);
			prog += " -m" + argprepare(dir, args["messages"]);
			prog += " -l " + spec.language;
		}
		else if(name == "sh") {
			prog = cmd.attribute("prog").value();
			for(auto& a : args) {
				prog += argprepare(dir, a.second);
			}
		}
		else if(name == "prefs") {
			// TODO: Do prefs make sense here?
		}
		else {
			throw std::runtime_error("Unknown command '" + name + "'");
		}
		if(!prog.empty()) {
			cmds.push_back(std::make_pair(prog,
						      makeDebugSuff(name, args)));
		}
	}
	return cmds;
}

void writePipeSpecSh(const string& specfile, const u16string& pipename, bool json, std::ostream& os) {
	const std::unique_ptr<PipeSpec> spec(new PipeSpec(specfile));
	const auto dir = dirname(abspath(specfile));
	const auto cmds = toPipeSpecShVector(dir, *spec, pipename, false, json);
	bool first = true;
	os << "#!/bin/sh" << std::endl << std::endl;
	for (const auto& cmd: cmds) {
		if(!first) {
			os << " \\" << std::endl << " | ";
		}
		first = false;
		os << cmd.first;
	}
}

void chmod777(const string& path) {
	mode_t mode = S_IRWXU|S_IRWXG|S_IRWXO;
	if (chmod(path.c_str(), mode) < 0) {
		throw std::runtime_error("libdivvun: ERROR: failed to chmod 777 " + path + std::strerror(errno));
	}
}

void writePipeSpecShDirOne(const vector<std::pair<string, string>> cmds, const string& pipename, const string& modesdir, bool nodebug) {
	// TODO: (modesdir / …) when we get <experimental/filesystem>
	size_t i = 0;
	if(nodebug) {
		i = cmds.size() - 1;
	}
	for (; i < cmds.size(); ++i) {
		string debug_suff = "";
		if(i < cmds.size() - 1) {
			debug_suff = std::to_string(i) + "-" + cmds[i].second;
		}
		// TODO: \\ on Windows
		const auto path = modesdir + "/" + pipename + debug_suff + ".mode";
		std::cout << path << std::endl;
		std::ofstream ofs;
		ofs.open(path, std::ofstream::out | std::ofstream::trunc);
		if(!ofs) {
			throw std::runtime_error("libdivvun: ERROR: Couldn't open " + path + " for writing! " + std::strerror(errno));
		}
		bool first = true;
		ofs << "#!/bin/sh" << std::endl << std::endl;
		for (size_t j = 0; j <= i; ++j) {
			const auto& cmd = cmds[j];
			if(!first) {
				ofs << " \\" << std::endl << " | ";
			}
			first = false;
			ofs << cmd.first;
		}
		ofs.close();
		chmod777(path);
	}
}

void writePipeSpecShDir(const string& specfile, bool json, const string& modesdir, bool nodebug) {
	const std::unique_ptr<PipeSpec> spec(new PipeSpec(specfile));
	const auto dir = dirname(abspath(specfile));
	for(const auto& p : spec->pnodes) {
		const auto& pipename = toUtf8(p.first);
		writePipeSpecShDirOne(toPipeSpecShVector(dir, *spec, p.first, false, json),
				      pipename,
				      modesdir,
				      nodebug);
		if(!nodebug) {
			writePipeSpecShDirOne(toPipeSpecShVector(dir, *spec, p.first, true, json),
					      "trace-" + pipename,
					      modesdir,
					      nodebug);
		}
	}
}
}
