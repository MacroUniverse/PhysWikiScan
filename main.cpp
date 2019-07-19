﻿#include "lib/PhysWikiScan.h"

// get arguments
void get_args(vector_O<Str32> args, Int_I argc, Char *argv[])
{
	args.clear();
	if (argc > 1) {
		// convert argv to args
		Str temp;
		for (Int i = 1; i < argc; ++i) {
			temp = argv[i];
			args.push_back(utf8to32(temp));
		}
	}
	else {
		// input args
		cout << u8"#===========================#" << endl;
		cout << u8"#     PhysWikiScan          #" << endl;
		cout << u8"#===========================#\n" << endl;

		cout << u8"请输入 arguments" << endl;
		Str temp; getline(std::cin, temp);
		Long ind0, ind1 = 0;
		ind0 = temp.find_first_not_of(' ', ind1);
		for (Int i = 0; i < 100; ++i) {
			ind1 = temp.find(' ', ind0);
			if (ind1 < 0) {
				if (Size(temp) > ind0)
					args.push_back(utf8to32(temp.substr(ind0)));
				break;
			}

			args.push_back(utf8to32(temp.substr(ind0, ind1 - ind0)));
			ind0 = temp.find_first_not_of(' ', ind1);
		}
	}
}

// get path and remove --path options from args
Long get_path(Str32_O path_in, Str32_O path_out, vector_IO<Str32> args)
{
	Str32 temp, line;
	if (!file_exist("set_path.txt")) {
		err_msg = U"内部错误： set_path.txt 不存在!";
		return -1;
	}
	read_file(temp, "set_path.txt");
	CRLF_to_LF(temp);
	vector<Str32> paths_in, paths_out;
	Long ind0 = 0;
	for (Long i = 0; i < 100; ++i) {
		ind0 = skip_line(temp, ind0);
		if (ind0 < 0) {
			err_msg = U"内部错误： path.txt 格式";
			return -1;
		}
		ind0 = get_line(line, temp, ind0);
		if (ind0 < 0) {
			err_msg = U"内部错误： path.txt 格式";
			return -1;
		}
		paths_in.push_back(line); trim(paths_in.back());
		ind0 = skip_line(temp, ind0);
		if (ind0 < 0) {
			err_msg = U"内部错误： path.txt 格式";
			return -1;
		}
		ind0 = get_line(line, temp, ind0);
		paths_out.push_back(line); trim(paths_out.back());
		if (ind0 < 0) {
			break;
		}
	}

	Long N = args.size();
	if (args.size() > 1 && args[N - 2] == U"--path") {
		if (args[N - 1] == U"0") {
			path_in = paths_in[0];
			path_out = paths_out[0];
		}
		else if (args[N - 1] == U"1") {
			path_in = paths_in[1];
			path_out = paths_out[1];
		}
		else if (args[N - 1] == U"2") {
			path_in = paths_in[2];
			path_out = paths_out[2];
		}
		else {
			err_msg = U"illegal --path argument!";
			return -1;
		}
		args.pop_back(); args.pop_back();
	}
	else { // default path
		path_in = paths_in[0];
		path_out = paths_out[0];
	}

	return paths_in.size();
}

int main(int argc, char *argv[]) {
	using namespace slisc;

	vector<Str32> args;
	get_args(args, argc, argv);

	// input folder, put tex files and code files here
	// same directory structure with PhysWiki
	Str32 path_in;
	// output folder, put png, svg in here
	// html will be output to here
	Str32 path_out;
	if (get_path(path_in, path_out, args) < 0) {
		cerr << err_msg << endl;
		return 0;
	}

	// === parse arguments ===

	if (args[0] == U"." && args.size() == 1) {
		// interactive full run (ask to try again in error)
		PhysWikiOnline(path_in, path_out);
	}
	else if (args[0] == U"--titles") {
		// update entries.txt and titles.txt
		vector<Str32> titles, entries;
		if (entries_titles(titles, entries, path_in) < 0) {
			cerr << err_msg << endl;
			return 0;
		}
		write_vec_str(titles, U"data/titles.txt");
		write_vec_str(entries, U"data/entries.txt");
	}
	else if (args[0] == U"--toc" && args.size() == 1) {
		// table of contents
		// read entries.txt and titles.txt, then generate index.html from PhysWiki.tex
		vector<Str32> titles, entries;
		if (file_exist(U"data/titles.txt"))
			read_vec_str(titles, U"data/titles.txt");
		if (file_exist(U"data/entries.txt"))
			read_vec_str(entries, U"data/entries.txt");
		if (titles.size() != entries.size()) {
			err_msg = U"内部错误： titles.txt 和 entries.txt 行数不同!";
			cerr << err_msg << endl;
			return 0;
		}
		if (table_of_contents(titles, entries, path_in, path_out) < 0) {
			cerr << err_msg << endl;
			return 0;
		}
	}
	else if (args[0] == U"--toc-changed" && args.size() == 1) {
		// table of contents
		// read entries.txt and titles.txt, then generate changed.html from changed.txt
		vector<Str32> titles, entries;
		if (file_exist(U"data/titles.txt"))
			read_vec_str(titles, U"data/titles.txt");
		if (file_exist(U"data/entries.txt"))
			read_vec_str(entries, U"data/entries.txt");
		if (titles.size() != entries.size()) {
			err_msg = U"内部错误： titles.txt 和 entries.txt 行数不同!";
			cerr << err_msg << endl;
			return 0;
		}
		if (table_of_changed(titles, entries, path_in, path_out) < 0) {
			cerr << err_msg << endl;
			return 0;
		}
	}
	else if (args[0] == U"--autoref" && args.size() == 4) {
		// check a label, add one if necessary
		vector<Str32> labels, ids;
		if (file_exist(U"data/labels.txt"))
			read_vec_str(labels, U"data/labels.txt");
		if (file_exist(U"data/ids.txt"))
			read_vec_str(ids, U"data/ids.txt");
		Str32 label;
		Long ret = check_add_label(label, args[1], args[2],
			atoi(utf32to8(args[3]).c_str()), labels, ids, path_in);
		vector<Str32> output;
		if (ret == -1) { // error
			cerr << err_msg << endl;
			return 0;
		}
		else if (ret == 0) { // added
			labels.push_back(label);
			ids.push_back(args[2] + args[3]);
			write_vec_str(labels, U"data/labels.txt");
			write_vec_str(ids, U"data/ids.txt");
			output = { label, U"added" };
		}
		else // ret == 1, already exist
			output = {label, U"exist"};
		cout << output[0] << endl;
		cout << output[1] << endl;
		write_vec_str(output, U"data/autoref.txt");
	}
	else if (args[0] == U"--entry" && args.size() > 1) {
		// process a single entry
		vector<Str32> entryN;
		Str32 temp;
		for (Int i = 1; i < args.size(); ++i) {
			temp = args[i];
			if (temp[0] == '-' && temp[1] == '-')
				break;
			entryN.push_back(temp);
		}
		if (PhysWikiOnlineN(entryN, path_in, path_out) < 0) {
			cerr << err_msg << endl;
			return 0;
		}
	}
	else {
		err_msg = U"内部错误： 命令不合法";
		cerr << err_msg << endl;
		return 0;
	}
	
	// PhysWikiCheck(U"../PhysWiki/contents/");

	cout << "done!" << endl;
	if (argc <= 1) {
		cout << "按任意键退出..." << endl;
		getchar();
	}
		
	return 0;
}
