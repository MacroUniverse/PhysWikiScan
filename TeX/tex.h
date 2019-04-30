﻿// tex parser utilities
// always remove comments first
#pragma once
#include "../SLISC/parser.h"
#include <cassert>

namespace slisc {

// find text command '\name', return the index of '\'
// output the index of "name.back()"
inline Long find_command(Str32_I str, Str32_I name, Long_I start)
{
	Long ind0 = start;
	while (true) {
		ind0 = str.find(U"\\" + name, ind0);
		if (ind0 < 0)
			return -1;
		// check right
		Char32 c = str[ind0 + name.size() + 1];
		if (c == U'{' || c == U' ' || c == U'\n' || is_num(c))
			return ind0;
		++ind0;
	}
}

// skipt command with 'Narg' arguments (command name can only have letters)
// arguments must be in braces '{}' for now
// input the index of '\'
// return one index after command name if 'Narg = 0'
// return one index after '}' if 'Narg > 0'
// return -1 if failed
inline Long skip_command(Str32_I str, Long_I ind, Long_I Narg = 0)
{
	Long i, ind0;
	for (i = ind + 1; i < str.size(); ++i) {
		if (!is_letter(str[i]))
			break;
	}
	if (i >= str.size() - 1)
		return -1;
	if (Narg > 0)
		ind0 = skip_scope(str, i, Narg);
}

// get the i-th command argument
// return the next index of the i-th '}'
// when "option" is 't', trim white spaces on both sides of "arg"
inline Long command_arg(Str32_O arg, Str32_I str, Long_I ind, Long_I i, Char_I option = 't')
{
	Long ind0, ind1;
	ind0 = skip_command(str, ind0);
	if (ind0 < 0) return -1;
	ind0 = skip_scope(str, i);
	if (ind0 < 0) return -1;
	ind0 = expect(str, U"{", ind0);
	if (ind0 < 0) return -1;
	ind1 = pair_brace(str, ind0 - 1);
	if (ind1 < 0) return -1;
	arg = str.substr(ind0, ind1 - ind0 - 1);
	trim(arg);
	return ind1;
}

// find command with a specific 1st arguments
// i.e. \begin{equation}
// return the index of '\', return -1 if not found
inline Long find_command_spec(Str32_I str, Str32_I name, Str32_I arg1, Long_I start)
{
	Long ind0 = start;
	Str32 arg1_;
	while (true) {
		ind0 = find_command(str, name, ind0);
		if (ind0 < 0)
			return -1;
		command_arg(arg1_, str, ind0, 0);
		if (arg1_ == arg1)
			return ind0;
		++ind0;
	}
}

// find the intervals of all commands with 1 argument
// intervals are from '\' to '}'
// return the number of commands found, return -1 if failed
inline Long find_all_command_intv(Intvs_O intv, Str32_I name, Str32_I str)
{
	Long ind0 = 0, N = 0;
	intv.clear();
	while (true) {
		ind0 = find_command(str, name, ind0);
		if (ind0 < 0)
			return intv.size();
		intv.pushL(ind0);
		ind0 = skip_command(str, ind0, 1);
		if (ind0 < 0)
			return -1;
		intv.pushR(ind0-1);
	}
}

// find all FindComBrace()
inline Long FindAllComBrace(Intvs_O intv, Str32_I key, Str32_I str, Char option = 'i')
{
	return find_scopes(intv, U"\\"+key, str, option);
}

// find a scope in str for environment named env
// return number of environments found. return -1 if failed
// if option = 'i', range starts from the next index of \begin{} and previous index of \end{}
// if option = 'o', range starts from '\' of \begin{} and '}' of \end{}
inline Long FindEnv(Intvs_O intv, Str32_I str, Str32_I env, Char option = 'i')
{
	Long ind0{}, ind1{}, ind2{}, ind3{};
	Intvs intvComm; // result from FindComment
	Long N{}; // number of environments found
	intv.clear();
	// find comments including the ones in lstlisting (doesn't matter)
	find_comments(intvComm, str, U"%");
	while (true) {
		// find "\\begin"
		ind3 = str.find(U"\\begin", ind0);
		if (is_in(ind3, intvComm)) { ind0 = ind3 + 6; continue; }
		if (ind3 < 0)
			return N;
		ind0 = ind3 + 6;

		// expect '{'
		ind0 = expect(str, U"{", ind0);
		if (ind0 < 0)
			return -1;
		// expect 'env'
		ind1 = expect(str, env, ind0);
		if (ind1 < 0 || !is_whole_word(str, ind1-env.size(), env.size()))
			continue;
		ind0 = ind1;
		// expect '}'
		ind0 = expect(str, U"}", ind0);
		if (ind0 < 0)
			return -1;
		intv.pushL(option == 'i' ? ind0 : ind3);

		while (true)
		{
			// find "\\end"
			ind0 = str.find(U"\\end", ind0);
			if (is_in(ind0, intvComm)) { ind0 += 4; continue; }
			if (ind0 < 0)
				return -1;
			ind2 = ind0 - 1;
			ind0 += 4;
			// expect '{'
			ind0 = expect(str, U"{", ind0);
			if (ind0 < 1)
				return -1;
			// expect 'env'
			ind1 = expect(str, env, ind0);
			if (ind1 < 0)
				continue;
			ind0 = ind1;
			// expect '}'
			ind0 = expect(str, U"}", ind0);
			if (ind0 < 0)
				return -1;
			intv.pushR(option == 'i' ? ind2 : ind0 - 1);
			++N;
			break;
		}
	}
}

// see if an index ind is in any of the evironments \begin{names[j]}...\end{names[j]}
// output iname of name[iname], -1 if return false
// TODO: check if this function works.
inline Bool IndexInEnv(Long& iname, Long ind, const vector<Str32>& names, Str32_I str)
{
	Intvs intv;
	for (Long i = 0; i < names.size(); ++i) {
		while (FindEnv(intv, str, names[i]) < 0) {
			Input().Bool("failed! retry?");
		}
		if (is_in(ind, intv)) {
			iname = i;
			return true;
		}
	}
	iname = -1;
	return false;
}

// find latex comments
// similar to FindComment0
// does not include the ones in lstlisting environment
inline Long FindComment(Intvs_O intv, Str32_I str)
{
	find_comments(intv, str, U"%");
	Intvs intvLst;
	FindEnv(intvLst, str, U"lstlisting", 'o');
	for (Long i = intv.size() - 1; i >= 0; --i) {
		if (is_in(intv.L(i), intvLst))
			intv.erase(i, 1);
	}
	return intv.size();
}

// find the range of inline equations using $$
// if option = 'i', intervals does not include $, if 'o', it does.
// return the number of $$ environments found.
inline Long FindInline(Intvs_O intv, Str32_I str, Char option = 'i')
{
	intv.clear();
	Long N{}; // number of $$
	Long ind0{};
	Intvs intvComm; // result from FindComment
	FindComment(intvComm, str);
	while (true) {
		ind0 = str.find(U"$", ind0);
		if (ind0 < 0) {
			break;
		} // did not find
		if (ind0 > 0 && str.at(ind0 - 1) == '\\') { // escaped
			++ind0; continue;
		}
		if (is_in(ind0, intvComm)) { // in comment
			++ind0; continue;
		}
		intv.push_back(ind0);
		++ind0; ++N;
	}
	if (N % 2 != 0) {
		SLS_ERR("odd number of $ found!"); // breakpoint here
		return -1;
	}
	N /= 2;
	if (option == 'i' && N > 0)
		for (Long i = 0; i < N; ++i) {
			++intv.L(i); --intv.R(i);
		}
	return N;
}

// TODO: find one instead of all
// TODO: find one using FindComBrace
// Long FindAllBegin

// Find "\begin{env}" or "\begin{env}{}" (option = '2')
// output interval from '\' to '}'
// return number found, return -1 if failed
// use FindAllBegin
inline Long FindAllBegin(Intvs_O intv, Str32_I env, Str32_I str, Char option)
{
	intv.clear();
	Long N{}, ind0{}, ind1;
	while (true) {
		ind1 = str.find(U"\\begin", ind0);
		if (ind1 < 0)
			return N;
		ind0 = expect(str, U"{", ind1 + 6);
		if (expect(str, env, ind0) < 0)
			continue;
		++N; intv.pushL(ind1);
		ind0 = pair_brace(str, ind0 - 1);
		if (option == '1')
			intv.pushR(ind0);
		ind0 = expect(str, U"{", ind0 + 1);
		if (ind0 < 0) {
			SLS_ERR("expecting {}{}!"); return -1;  // break point here
		}
		ind0 = pair_brace(str, ind0 - 1);
		intv.pushR(ind0);
	}
}

// Find "\end{env}"
// output ranges to intv, from '\' to '}'
// return number found, return -1 if failed
inline Long FindEnd(Intvs_O intv, Str32_I env, Str32_I str)
{
	intv.clear();
	Long N{}, ind0{}, ind1{};
	while (true) {
		ind1 = str.find(U"\\end", ind0);
		if (ind1 < 0)
			return N;
		ind0 = expect(str, U"{", ind1 + 4);
		if (expect(str, env, ind0) < 0)
			continue;
		++N; intv.pushL(ind1);
		ind0 = pair_brace(str, ind0 - 1);
		intv.pushR(ind0);
	}
}

// Find normal text range
// return -1 if failed
inline Long FindNormalText(Intvs_O indNorm, Str32_I str)
{
	Intvs intv, intv1;
	// comments
	FindComment(intv, str);
	// inline equation environments
	FindInline(intv1, str, 'o');
	if (combine(intv, intv1) < 0) return -1;
	// equation environments
	FindEnv(intv1, str, U"equation", 'o');
	if (combine(intv, intv1) < 0) return -1;
	// command environments
	FindEnv(intv1, str, U"Command", 'o');
	if (combine(intv, intv1) < 0) return -1;
	// gather environments
	FindEnv(intv1, str, U"gather", 'o');
	if (combine(intv, intv1) < 0) return -1;
	// align environments (not "aligned")
	FindEnv(intv1, str, U"align", 'o');
	if (combine(intv, intv1) < 0) return -1;
	// texttt command
	find_all_command_intv(intv1, U"texttt", str);
	if (combine(intv, intv1) < 0) return -1;
	// input command
	find_all_command_intv(intv1, U"input", str);
	if (combine(intv, intv1) < 0) return -1;
	// Figure environments
	FindEnv(intv1, str, U"figure", 'o');
	if (combine(intv, intv1) < 0) return -1;
	// Table environments
	FindEnv(intv1, str, U"table", 'o');
	if (combine(intv, intv1) < 0) return -1;
	// subsubsection command
	find_all_command_intv(intv1, U"subsubsection", str);
	if (combine(intv, intv1) < 0) return -1;
	//  \begin{exam}{} and \end{exam}
	FindAllBegin(intv1, U"exam", str, '2');
	if (combine(intv, intv1) < 0) return -1;
	FindEnd(intv1, U"exam", str);
	if (combine(intv, intv1) < 0) return -1;
	//  exer\begin{exer}{} and \end{exer}
	FindAllBegin(intv1, U"exer", str, '2');
	if (combine(intv, intv1) < 0) return -1;
	FindEnd(intv1, U"exer", str);
	if (combine(intv, intv1) < 0) return -1;
	// invert range
	return invert(indNorm, intv, str.size());
}

// detect unnecessary braces and add "删除标记"
// return the number of braces pairs removed
inline Long RemoveBraces(vector<Long>& ind_left, vector<Long>& ind_right,
	vector<Long>& ind_RmatchL, Str32_IO str)
{
	unsigned i, N{};
	vector<Long> ind; // redundent right brace index
	for (i = 1; i < ind_right.size(); ++i)
		// there must be no space between redundent {} and neiboring braces.
		if (ind_right[i] == ind_right[i - 1] + 1 &&
			ind_left[ind_RmatchL[i]] == ind_left[ind_RmatchL[i - 1]] - 1)
		{
			ind.push_back(ind_right[i]);
			ind.push_back(ind_left[ind_RmatchL[i]]);
			++N;
		}

	if (N > 0) {
		sort(ind.begin(), ind.end());
		for (Long i = ind.size() - 1; i >= 0; --i) {
			str.insert(ind[i], U"删除标记");
		}
	}
	return N;
}

// replace \nameComm{...} with strLeft...strRight
// {} cannot be omitted
// must remove comments first
inline Long Command2Tag(Str32_I nameComm, Str32_I strLeft, Str32_I strRight, Str32_IO str)
{
	Long N{}, ind0{}, ind1{}, ind2{};
	while (true) {
		ind0 = str.find(U"\\" + nameComm, ind0);
		if (ind0 < 0) break;
		ind1 = ind0 + nameComm.size() + 1;
		ind1 = expect(str, U"{", ind1); --ind1;
		if (ind1 < 0) {
			++ind0; continue;
		}
		ind2 = pair_brace(str, ind1);
		str.erase(ind2, 1);
		str.insert(ind2, strRight);
		str.erase(ind0, ind1 - ind0 + 1);
		str.insert(ind0, strLeft);
		++N;
	}
	return N;
}

// replace nameEnv environment with strLeft...strRight
// must remove comments first
inline Long Env2Tag(Str32_I nameEnv, Str32_I strLeft, Str32_I strRight, Str32_IO str)
{
	Long i{}, N{}, Nenv;
	Intvs intvEnvOut, intvEnvIn;
	Nenv = FindEnv(intvEnvIn, str, nameEnv, 'i');
	Nenv = FindEnv(intvEnvOut, str, nameEnv, 'o');
	for (i = Nenv - 1; i >= 0; --i) {
		str.erase(intvEnvIn.R(i) + 1, intvEnvOut.R(i) - intvEnvIn.R(i));
		str.insert(intvEnvIn.R(i) + 1, strRight);
		str.erase(intvEnvOut.L(i), intvEnvIn.L(i) - intvEnvOut.L(i));
		str.insert(intvEnvOut.L(i), strLeft);
		++N;
	}
	return N;
}

} // namespace slisc
