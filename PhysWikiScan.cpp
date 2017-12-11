#include <atlstr.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

// search all file names of a certain file extension
// path must end with "\\"
vector<CString> GetFileNames(CString path, CString extension)
{
	vector<CString> names;
	CString file, name;
	WIN32_FIND_DATA data;
	HANDLE hfile;
	file = path + _T("*.") + extension;
	hfile = FindFirstFile(file, &data);
	if (data.cFileName[0] == 52428)
		return names;
	names.push_back(data.cFileName);
	//wcout << names.back().GetString() << endl;
	while (true)
	{
		FindNextFile(hfile, &data);
		name = data.cFileName;
		if (!name.Compare(names.back())) //if name == names.back()
			break;
		else
		{
			names.push_back(name);
			//wcout << name.GetString() << endl;
		}
	}
	return names;
}

// convert CString to UTF-8
string to_utf8(CString cstr)
{
	char *buffer = new char[1024 * 1024];
	WideCharToMultiByte(CP_UTF8, NULL, cstr, -1, buffer, 1024 * 1024, NULL, FALSE);

	string str = buffer;
	delete[]buffer;
	return str;
}

// convert UTF-8 to CString
CString from_utf8(string str)
{
	wchar_t *buffer = new wchar_t[1024 * 1024];
	// Convert headers from ASCII to Unicode.
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, 1024 * 1024);

	CString cstr = buffer;
	delete[]buffer;
	return cstr;
}

// read a UTF8 file to CString
CString ReadUTF8(CString path)
{
	ifstream fin;
	fin.open(path, ios::in);
	if (!fin.is_open())
	{
		printf("open input file error\n");
		return _T("error");
	}

	char buffer[100 * 1024] = {};
	fin.read(buffer, 1024 * 1024);
	fin.close();

	return from_utf8(buffer);
}

// write a CString to a UTF8 file
void WriteUTF8(CString text, CString path)
{
	string out_text = to_utf8(text);
	ofstream fout;
	fout.open(path, ios::out | ios::trunc | ios::binary);
	if (!fout.is_open())
		printf("open output file error\n");
	fout.write(out_text.c_str(), out_text.size());
	fout.close();
}

// see if a key appears followed only with only white space
// return the next index if yes, return -1 if no.
int ExpectKey(const CString& str, CString key, int start)
{
	int ind = start;
	int ind0 = 0;
	int L = str.GetLength();
	int L0 = key.GetLength();
	TCHAR c0, c;
	while (true)
	{
		c0 = key.GetAt(ind0);
		c = str.GetAt(ind);
		if (c == c0)
		{
			++ind0;
			if (ind0 == L0)
				return ind + 1;
		}
		else if (c != ' ')
			return -1;
		++ind;
		if (ind == L)
			return -1;
	}
}

// find a scope in str for environment named env
// output: ind is index in pairs
// return number of environments found. return -1 if failed
int FindEnv(vector<int>& ind, const CString& str, CString env)
{
	int ind0 = 0, i, ind1, ind2;
	int N{}; // number of environments found
	ind.resize(0);
	while (true)
	{
		// find "\\begin"
		ind0 = str.Find(_T("\\begin"), ind0);
		if (ind0 < 0)
			return N;
		ind0 += 6;

		// find '{'
		ind0 = ExpectKey(str, _T("{"), ind0);
		if (ind0 < 0)
			return -1;
		// find 'env'
		ind1 = ExpectKey(str, env, ind0);
		if (ind1 < 0)
			continue;
		ind0 = ind1;
		// find '}'
		ind0 = ExpectKey(str, _T("}"), ind0);
		if (ind0 < 0)
			return -1;
		ind.push_back(ind0);

		while (true)
		{
			// find "\\end"
			ind0 = str.Find(_T("\\end"), ind0);
			if (ind0 < 0)
				return -1;
			ind2 = ind0 - 1;
			ind0 += 4;
			// find '{'
			ind0 = ExpectKey(str, _T("{"), ind0);
			if (ind0 < 1)
				return -1;
			// find 'env'
			ind1 = ExpectKey(str, env, ind0);
			if (ind1 < 0)
				continue;
			ind0 = ind1;
			// find '}'
			ind0 = ExpectKey(str, _T("}"), ind0);
			if (ind0 < 0)
				return -1;
			ind.push_back(ind2);
			++N;
			break;
		}
	}
}

// find inline equations using $$
// indices of every $ appended to ind.
// return number of $$ environments found
int FindInline0(vector<int>& ind, const CString& str, int begin, int end)
{
	int i, N{};
	TCHAR c, c0 = ' ';
	for (i = begin; i <= end; ++i)
	{
		c = str.GetAt(i);
		if (c == '$' && c0 != '\\')
		{
			ind.push_back(i);
			++N;
		}
		c0 = c;
	}
	if (N % 2 != 0)
		return -1;
	return N / 2;
}

// find inline equations using $$
// need the result from equation environment
// return the number of $$ environments found.
int FindInline(vector<int>& ind, const CString& str, vector<int>& indEq)
{
	ind.resize(0);
	int N{}; // number of $$
	int Neq = indEq.size() / 2; // number of equation environments
	if (Neq == 0)
	{
		N = FindInline0(ind, str, 0, str.GetLength() - 1);
		if (N < 0)
		{
			cout << "error!"; return -1;
		}
	}
	else
	{
		N = FindInline0(ind, str, 0, indEq[0]);
		for (int i = 1; i < Neq * 2 - 1; i += 2)
			N += FindInline0(ind, str, indEq[i], indEq[i + 1]);
		N += FindInline0(ind, str, indEq.back(), str.GetLength() - 1);
	}
	return N;
}

// Pair right brace to left one
// ind is inddex of left brace
// return index of right brace, -1 if failed
int PairBraceR(const CString& str, int ind)
{
	TCHAR c, c0 = ' ';
	int Nleft = 1;
	for (int i{ ind+1 }; i < str.GetLength(); i++)
	{
		c = str.GetAt(i);
		if (c == '{' && c0 != '\\')
			++Nleft;
		else if (c == '}' && c0 != '\\')
		{
			--Nleft;
			if (Nleft == 0)
				return i;
		}
		c0 = c;
	}
	return -1;
}

// Find all \texttt{} environment in str
// index pairs of \texttt scope (ind).
// return number of \texttt{} found, return -1 if failed.
int FindTT(vector<int>& ind, CString& str)
{
	ind.resize(0);
	int ind0{};
	while (true)
	{
		ind0 = str.Find(_T("\texttt"));
		if (ind0 < 0)
			return ind.size() / 2;
		ind0 += 7;
		ind0 = ExpectKey(str, _T("{"), ind0);
		if (ind0 < 0)
			return -1;
		ind.push_back(ind0);
		ind0 = PairBraceR(str, ind0 - 1);
		if (ind0 < 0)
			cout << "error!"; return -1;
		ind.push_back(ind0 - 1);
	}
}

// sort x in descending order while keeping the index
void sort(vector<int>& x, vector<int>& ind)
{
	int i, temp;
	int N = x.size();
	// initialize ind
	ind.resize(0);
	for (i = 0; i < N; ++i)
		ind.push_back(i);

	bool changed{ true };
	while (changed == true)
	{
		changed = false;
		for (i = 0; i < N - 1; i++)
		{
			if (x[i] > x[i + 1])
			{
				temp = x[i];
				x[i] = x[i + 1];
				x[i + 1] = temp;
				temp = ind[i];
				ind[i] = ind[i + 1];
				ind[i + 1] = temp;
				changed = true;
			}
		}
	}
}

// combine scopes ind1 and ind1
// ind1 or ind 2 must be be in order and not overlap with themselves
// ind1 can overlap ind2
int CombineScope(vector<int>& ind, vector<int> ind1, vector<int> ind2)
{
	int i, N1 = ind1.size(), N2 = ind2.size();
	vector<int> start, end, order;
	for (i = 0; i < N1; i += 2)
		start.push_back(ind1[i]);
	for (i = 1; i < N1; i += 2)
		end.push_back(ind1[i]);
	for (i = 0; i < N2; i += 2)
		start.push_back(ind2[i]);
	for (i = 1; i < N2; i += 2)
		end.push_back(ind2[i]);
	int N = start.size();
	sort(start, order);
	vector<int> temp;
	temp.resize(N);
	for (i = 0; i < N; ++i)
		temp[i] = end[order[i]];
	end = temp;
	ind.push_back(start[0]);
	i = 0;
	while(i < N - 1)
	{
		if (end[i] >= start[i + 1])
		{ ++i; continue; }
		else
		{
			ind.push_back(end[i]);
			ind.push_back(start[i+1]);
			++i;
		}
	}
	ind.push_back(end.back());
}

// Find normal text
int FindNormalText(vector<int>& ind, CString& str)
{

}

// match braces
// return -1 means failure, otherwise return number of {} paired
// output ind_left, ind_right, ind_RmatchL
int MatchBraces(vector<int>& ind_left, vector<int>& ind_right,
	vector<int>& ind_RmatchL, CString& str, int start, int end)
{
	ind_left.resize(0); ind_right.resize(0); ind_RmatchL.resize(0);
	TCHAR c, c_last = ' ';
	bool continuous{ false };
	int Nleft = 0, Nright = 0;
	vector<bool> Lmatched;
	bool matched;
	for (int i = start; i <= end; ++i)
	{
		c = str.GetAt(i);
		if (c == '{' && c_last != '\\')
		{
			++Nleft;
			ind_left.push_back(i);
			Lmatched.push_back(false);
		}
		else if (c == '}' && c_last != '\\')
		{
			++Nright;
			ind_right.push_back(i);
			matched = false;
			for (int j = Nleft - 1; j >= 0; --j)
				if (!Lmatched[j])
				{
					ind_RmatchL.push_back(j);
					Lmatched[j] = true;
					matched = true;
					break;
				}
			if (!matched)
				return -1; // unbalanced braces
		}
		c_last = c;
	}
	if (Nleft != Nright)
		return -1;
	return Nleft;
}

// detect and unnecessary braces and add "ɾ�����"
// return the number of braces pairs removed
int RemoveBraces(vector<int>& ind_left, vector<int>& ind_right,
	vector<int>& ind_RmatchL, CString& str)
{
	int i, N{};
	//bool continuous{ false };
	vector<int> ind; // redundent right brace index
	for (i = 1; i < ind_right.size(); ++i)
		// there must be no space between redundent {} and neiboring braces.
		if (ind_right[i] == ind_right[i - 1] + 1 &&
			ind_left[ind_RmatchL[i]] == ind_left[ind_RmatchL[i - 1]] - 1)
		{
			ind.push_back(ind_right[i]);
			ind.push_back(ind_left[ind_RmatchL[i]]);
			++N;
		}

	sort(ind.begin(), ind.end());
	for (i = ind.size() - 1; i >= 0; --i)
	{
		str.Insert(ind[i], _T("ɾ�����"));
	}
	return N;
}

// remove extra {} from equation environment and rewrite file
// return total {} deleted
int OneFile1(CString path)
{
	CString str = ReadUTF8(path); // read file

								  // get all the equation environment scopes

	vector<int> eqscope;
	if (FindEnv(eqscope, str, _T("equation")) < 0)
	{
		cout << "error!"; return 0;
	}
	// match  and remove braces
	vector<int> ind_left, ind_right, ind_RmatchL;
	int N{}; // total {} removed
	for (int i = eqscope.size() - 1; i > 0; i -= 2)
	{
		int Npair = MatchBraces(ind_left, ind_right, ind_RmatchL,
			str, eqscope[i - 1], eqscope[i]);
		if (Npair > 0)
			N += RemoveBraces(ind_left, ind_right, ind_RmatchL, str);
	}
	if (N > 0)
		WriteUTF8(str, path); // write file
	return N;
}

// remove extra {} from $$ environment and rewrite file
// return total number of {} pairs removed
int OneFile2(CString path)
{
	CString str = ReadUTF8(path); // read file

								  // get all the equation environment scopes
	vector<int> indEq;
	if (FindEnv(indEq, str, _T("equation")) < 0)
	{
		cout << "error!"; return 0;
	}
	vector<int> ind; // indices for all the $.
	if (FindInline(ind, str, indEq) == 0)
		return 0;

	// match  and remove braces
	vector<int> ind_left, ind_right, ind_RmatchL;
	int Npair{}; // total {} pairs found
	int N{}; // total {} removed
	for (int i = ind.size() - 1; i > 0; i -= 2)
	{
		int Npair = MatchBraces(ind_left, ind_right, ind_RmatchL,
			str, ind[i - 1], ind[i]);
		if (Npair > 0)
			N += RemoveBraces(ind_left, ind_right, ind_RmatchL, str);
	}
	// write file
	if (N > 0)
		WriteUTF8(str, path);
	return N;
}

void main()
{
	CString path0 = _T("C:\\Users\\addis\\Documents\\GitHub\\PhysWiki\\contents\\");
	//_T("C:\\Users\\addis\\Documents\\GitHub\\PhysWiki\\contents\\");
	//_T("C:\\Users\\addis\\Desktop\\");
	vector<CString> names = GetFileNames(path0, _T("tex"));
	int N;
	for (int i{}; i < names.size(); ++i)
	{
		wcout << names[i].GetString() << _T("...");
		N = OneFile2(path0 + names[i]);
		cout << N << endl;
	}
}
