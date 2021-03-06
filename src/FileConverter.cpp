/*
 * FileConverter.cpp
 *
 *  Created on: Feb 1, 2017
 *      Author: diepbp
 */

#include "FileConverter.h"

template< typename T >
std::string int_to_hex( T i )
{
	std::stringstream stream;
	stream << "_x" << std::hex << i;
	return stream.str();
}

/*
 * (not (= a b))
 */
std::string refine_not_equality(std::string str){
	assert(str.find("(not (") == 0);

	int num = findCorrespondRightParentheses(5, str);
	std::string tmpStr = str.substr(5, num - 5 + 1);

	std::string retTmp = "";
	unsigned int pos = 0;
	for (pos = 0; pos < tmpStr.length(); ++pos)
		if (tmpStr[pos] == ' ')
			pos++;
		else
			break;

	/* remove all two spaces */
	for (; pos < tmpStr.length(); ++pos) {
		if (tmpStr[pos] == ' ' && tmpStr[pos + 1] == ' ')
			continue;
		else
			retTmp = retTmp + tmpStr[pos];
	}

	std::string ret = "(";
	for (unsigned int i = 1; i < retTmp.length(); ++i){
		if (	i + 1 < retTmp.length() &&
				(retTmp[i - 1] == ')' 		|| retTmp[i - 1] == '(') &&
				retTmp[i] == ' ' &&
				(retTmp[i + 1] == ')'  || retTmp[i + 1] == '(') &&
				retTmp[i - 1] == retTmp[i + 1]){
			continue;
		}
		else if (retTmp[i - 1] == '(' && retTmp[i] == ' '){
			continue;
		}
		else if (i + 1 < retTmp.length() && retTmp[i + 1] == ')' && retTmp[i] == ' '){
			continue;
		}
		else
			ret = ret + retTmp[i];
	}
	__debugPrint(logFile, "%d *** %s ***: %s --> %s --> %s\n", __LINE__, __FUNCTION__, str.c_str(), retTmp.c_str(), ret.c_str());
	return ret;
}

/*
 * For all string variables
 * Change var name: xyz --> len_xyz
 * and change var type: string -> int
 */
std::string redefineStringVar(std::string var){
	return "(declare-const len_" + var + " Int)";
}

std::string redefineOtherVar(std::string var, std::string type){
	return "(declare-const " + var + " " + type + ")";
}

/*
 *
 */
std::vector<std::pair<std::string, int>> replaceTokens(std::vector<std::pair<std::string, int>> tokens,
		int start, int finish, std::string tokenName, int tokenType){
	std::vector<std::pair<std::string, int>> tmp;
	for (int i = 0; i < start; ++i)
		tmp.emplace_back(tokens[i]);
	tmp.push_back(std::make_pair(tokenName, tokenType));
	for (int i = finish + 1; i < (int)tokens.size(); ++i)
		tmp.emplace_back(tokens[i]);

	return tmp;
}

/*
 *
 */
std::vector<std::pair<std::string, int>> replaceTokens(std::vector<std::pair<std::string, int>> tokens,
		int start, int finish, std::vector<std::pair<std::string, int>> addTokens){
	std::vector<std::pair<std::string, int>> tmp;
	for (int i = 0; i < start; ++i)
		tmp.emplace_back(tokens[i]);
	tmp.insert(tmp.end(), addTokens.begin(), addTokens.end());
	for (int i = finish; i < (int)tokens.size(); ++i)
		tmp.emplace_back(tokens[i]);

	return tmp;
}

/*
 *
 */
std::string sumTokens(std::vector<std::pair<std::string, int>> tokens,
		int start, int finish){
	assert(start <= finish);
	std::string ret = tokens[start].first;

	for (int i = start + 1; i <= finish; ++i){
		if (tokens[i].first.compare(")") != 0 && tokens[i - 1].first.compare("(") != 0)
			ret += " ";
		ret += tokens[i].first;
	}

	return ret;
}

/*
 *
 */
int findTokens(std::vector<std::pair<std::string, int>> tokens, int startPos, std::string s, int type){
	for (int i = startPos; i < (int)tokens.size(); ++i)
		if (tokens[i].second == type && tokens[i].first.compare(s) == 0)
			return i;
	return -1;
}

StringOP findStringOP(
		std::vector<std::pair<std::string, int>> tokens,
		std::string name,
		int argsNum,
		int startPos){

//	__debugPrint(logFile, "%d *** %s ***: %s\n", __LINE__, __FUNCTION__, sumTokens(tokens, startPos, tokens.size() - 1).c_str());
	StringOP op(name);

	if (argsNum > 0) {
		/* get the first arg */
		startPos++;
		if (tokens[startPos].second == 92) {
			int tmp = findCorrespondRightParentheses(startPos, tokens);
			op.setArg01(sumTokens(tokens, startPos, tmp));
			startPos = tmp;
		}
		else {
			op.setArg01(tokens[startPos].first);
		}
	}
	else
		return op;

	if (argsNum > 1) {
		/* get the 2nd arg */
		startPos++;
		if (tokens[startPos].second == 92) {
			int tmp = findCorrespondRightParentheses(startPos, tokens);

			/* convert - --> + */
			if  (tokens[startPos + 1].first.compare("-") == 0){
				StringOP opx = findStringOP(tokens, "-", 2, startPos + 1);
				opx.name = "+";
				std::string tmp = "(* (- 1) " + opx.arg02 + ")";
				opx.arg02 = opx.arg01;
				opx.arg01 = tmp;

				op.setArg02(opx.toString());
			}
			else if  (tokens[startPos + 1].first.compare("+") == 0){
				StringOP opx = findStringOP(tokens, "+", 2, startPos + 1);

				/* swap 1 vs 2 */
				if (opx.arg02[0] >= '0' && opx.arg02[0] <= '9' && (!(opx.arg01[0] >= '0' && opx.arg01[0] <= '9'))) {
					std::string tmp = opx.arg02;
					opx.arg02 = opx.arg01;
					opx.arg01 = tmp;
				}

				op.setArg02(opx.toString());
			}
			else
				op.setArg02(sumTokens(tokens, startPos, tmp));
			startPos = tmp;
		}
		else {
			op.setArg02(tokens[startPos].first);
		}
	}
	else
		return op;

	if (argsNum > 2) {
		/* get the 3rd arg */
		startPos++;
		if (tokens[startPos].second == 92) {
			int tmp = findCorrespondRightParentheses(startPos, tokens);
			/* convert - --> + */
			if  (tokens[startPos + 1].first.compare("-") == 0){
				StringOP opx = findStringOP(tokens, "-", 2, startPos + 1);
				opx.name = "+";
				std::string tmp = "(* (- 1) " + opx.arg02 + ")";
				opx.arg02 = opx.arg01;
				opx.arg01 = tmp;

				op.setArg03(opx.toString());
			}
			else if  (tokens[startPos + 1].first.compare("+") == 0){
				StringOP opx = findStringOP(tokens, "+", 2, startPos + 1);

				/* swap 1 vs 2 */
				if (opx.arg02[0] >= '0' && opx.arg02[0] <= '9' && (!(opx.arg01[0] >= '0' && opx.arg01[0] <= '9'))) {
					std::string tmp = opx.arg02;
					opx.arg02 = opx.arg01;
					opx.arg01 = tmp;
				}

				op.setArg03(opx.toString());
			}
			else
				op.setArg03(sumTokens(tokens, startPos, tmp));

			startPos = tmp;
		}
		else {
			op.setArg03(tokens[startPos].first);
		}
	}
	else
		return op;

	return op;
}

/*
 * (implies x) --> (implies false x)
 */
void updateImplies(std::vector<std::pair<std::string, int>> &tokens){
	int found = findTokens(tokens, 0, "implies", 88);
	while (found != -1) {
		int endCond = -1;
		if (tokens[found + 1].second == 92) {
			endCond = findCorrespondRightParentheses(found + 1, tokens);
		}
		else {
			endCond = found + 1;
		}

		tokens = replaceTokens(tokens, found + 1, endCond, "false", 7);

		found = findTokens(tokens, endCond, "implies", 88);
	}
}

/*
 * (RegexIn ...) --> TRUE
 */
void updateRegexIn(std::vector<std::pair<std::string, int>> &tokens){
	int found = -1;
	for (unsigned i = 0; i < tokens.size(); ++i)
		if (tokens[i].second == 88 && tokens[i].first.compare("RegexIn") == 0) {
			found = (int)i;
			break;
		}
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);
//		__debugPrint(logFile, "%d *** %s ***: s = %s\n", __LINE__, __FUNCTION__, s.c_str());

		/* clone & replace */
		tokens = replaceTokens(tokens, found - 1, pos, "true", 15);

		found = -1;
		for (unsigned i = found + 1; i < tokens.size(); ++i)
			if (tokens[i].second == 88 && tokens[i].first.compare("RegexIn") == 0) {
				found = (int)i;
				break;
			}
	}
}

/*
 * (Contains v1 v2) --> TRUE || FALSE
 */
void updateContain(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap){

	int found = findTokens(tokens, 0, "Contains", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);
		__debugPrint(logFile, "%d *** %s ***: s = %s\n", __LINE__, __FUNCTION__, sumTokens(tokens, 0, tokens.size() - 1).c_str());

		StringOP op(findStringOP(tokens, "Contains", 2, found));
		assert(rewriterStrMap.find(op) != rewriterStrMap.end());
		if (rewriterStrMap[op].compare("true") == 0)
			tokens = replaceTokens(tokens, found - 1, pos, "true", 15);
		else
			tokens = replaceTokens(tokens, found - 1, pos, "false", 7);
		__debugPrint(logFile, "--> s = %s \n", sumTokens(tokens, 0, tokens.size() - 1).c_str());

		found = findTokens(tokens, 0, "Contains", 88);
	}
}

/*
 * (Indexof v1 v2) --> ....
 */
void updateIndexOf(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap){
	int found = findTokens(tokens, 0, "Indexof", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);

		StringOP op(findStringOP(tokens, "Indexof", 2, found));
		__debugPrint(logFile, "%d *** %s ***: s = %s\n", __LINE__, __FUNCTION__, op.toString().c_str());
		assert(rewriterStrMap.find(op) != rewriterStrMap.end());

		tokens = replaceTokens(tokens, found - 1, pos, rewriterStrMap[op], 88);
		found = findTokens(tokens, 0, "Indexof", 88);
	}
}

/*
 * (LastIndexof v1 v2) --> ....
 */
void updateLastIndexOf(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap){
	int found = findTokens(tokens, 0, "LastIndexof", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);
//		__debugPrint(logFile, "%d *** %s ***: s = %s\n", __LINE__, __FUNCTION__, s.c_str());

		StringOP op(findStringOP(tokens, "LastIndexof", 2, found));
		assert(rewriterStrMap.find(op) != rewriterStrMap.end());

		tokens = replaceTokens(tokens, found - 1, pos, rewriterStrMap[op], 88);
		found = findTokens(tokens, 0, "LastIndexof", 88);
	}
}

/*
 * (EndsWith v1 v2) --> ....
 */
void updateEndsWith(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap){
	int found = findTokens(tokens, 0, "EndsWith", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);
//		__debugPrint(logFile, "%d *** %s ***: s = %s\n", __LINE__, __FUNCTION__, s.c_str());

		StringOP op(findStringOP(tokens, "EndsWith", 2, found));
		assert(rewriterStrMap.find(op) != rewriterStrMap.end());

		tokens = replaceTokens(tokens, found - 1, pos, rewriterStrMap[op], 88);
		found = findTokens(tokens, 0, "EndsWith", 88);
	}
}

/*
 * (StartsWith v1 v2) --> ....
 */
void updateStartsWith(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap){
	int found = findTokens(tokens, 0, "StartsWith", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);
//		__debugPrint(logFile, "%d *** %s ***: s = %s\n", __LINE__, __FUNCTION__, s.c_str());

		StringOP op(findStringOP(tokens, "StartsWith", 2, found));
		assert(rewriterStrMap.find(op) != rewriterStrMap.end());

		tokens = replaceTokens(tokens, found - 1, pos, rewriterStrMap[op], 88);
		found = findTokens(tokens, 0, "StartsWith", 88);
	}
}

/*
 * = x (Replace ...) --> true
 */
void updateReplace(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap){
	int found = findTokens(tokens, 0, "Replace", 88);
	while (found != -1){
		while (found >= 0) {
			if (tokens[found].first.compare("(") == 0 && tokens[found + 1].first.compare("=") == 0){
				int pos = findCorrespondRightParentheses(found, tokens);
				tokens = replaceTokens(tokens, found, pos, "true", 15);
				break;
			}
			else
				found = found - 1;
		}
		found = findTokens(tokens, found, "Replace", 88);
	}
}

/*
 * = x (ReplaceAll ...) --> true
 */
void updateReplaceAll(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap){
	int found = findTokens(tokens, 0, "ReplaceAll", 88);
	while (found != -1){
		while (found >= 0) {
			if (tokens[found].first.compare("(") == 0 && tokens[found + 1].first.compare("=") == 0){
				int pos = findCorrespondRightParentheses(found, tokens);
				tokens = replaceTokens(tokens, found, pos, "true", 15);
				break;
			}
			else
				found = found - 1;
		}
		found = findTokens(tokens, found, "ReplaceAll", 88);
	}
}

/*
 * (Substring a b c) --> c
 */
void updateSubstring(
		std::vector<std::pair<std::string, int>> &tokens,
		std::map<StringOP, std::string> rewriterStrMap) {

	int found = findTokens(tokens, 0, "Substring", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);
		__debugPrint(logFile, "%d *** %s ***: s = %s\n", __LINE__, __FUNCTION__, sumTokens(tokens, 0, tokens.size() - 1).c_str());

		int startAssignment = found - 2;
		while (startAssignment >= 0) {
			if (tokens[startAssignment].first.compare("(") == 0 && tokens[startAssignment + 1].first.compare("=") == 0){
				break;
			}
			else
				startAssignment--;
		}

		StringOP op(findStringOP(tokens, "Substring", 3, found));
		__debugPrint(logFile, "%d op = %s\n", __LINE__, op.toString().c_str());
		assert(rewriterStrMap.find(op) != rewriterStrMap.end());

		assert(op.arg03.length() > 0);
		if (op.arg03[0] >= '0' && op.arg03[0] >= '9') {
			tokens = replaceTokens(tokens, found - 1, pos, op.arg03, 82);
		}
		else {
			tokens = replaceTokens(tokens, found - 1, pos, op.arg03, 88);
		}

		__debugPrint(logFile, "%d *** after replace ***: s = %s\n", __LINE__, sumTokens(tokens, 0, tokens.size() - 1).c_str());
		std::vector<std::pair<std::string, int>> tmp;
		for (int i = 0; i < startAssignment; ++i)
			tmp.emplace_back(tokens[i]);

		tmp.push_back(std::make_pair("(", 92));
		tmp.push_back(std::make_pair("and", 88));
		tmp.push_back(std::make_pair(rewriterStrMap[op], 88));

		for (int i = startAssignment; i <= startAssignment + 4; ++i) {
			if (i == found - 1){
				std::vector<std::pair<std::string, int>> tmpx = parseTerm(tokens[i].first);
				tmp.insert(tmp.end(), tmpx.begin(), tmpx.end());
			}
			else
				tmp.emplace_back(tokens[i]);
		}

		tmp.push_back(std::make_pair(")", 93));

		__debugPrint(logFile, "%d *** added rewrite ***: s = %s\n", __LINE__, sumTokens(tmp, 0, tmp.size() - 1).c_str());


		for (int i = startAssignment + 5; i < (int)tokens.size(); ++i)
			tmp.emplace_back(tokens[i]);

		__debugPrint(logFile, "%d *** FINAL ***: s = %s\n", __LINE__, sumTokens(tmp, 0, tmp.size() - 1).c_str());
		tokens.clear();
		tokens = tmp;

		found = findTokens(tokens, pos, "Substring", 88);
	}
}

/*
 * ToUpper --> len = len
 */
void updateToUpper(std::vector<std::pair<std::string, int>> &tokens) {
	int found = findTokens(tokens, 0, "ToUpper", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);

		StringOP op(findStringOP(tokens, "ToUpper", 1, found));

		tokens = replaceTokens(tokens, found - 1, pos, op.arg01, 88);
		found = findTokens(tokens, pos, "ToUpper", 88);
	}
}

/*
 * ToLower --> len = len
 */
void updateToLower(std::vector<std::pair<std::string, int>> &tokens) {
	int found = findTokens(tokens, 0, "ToLower", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);

		StringOP op(findStringOP(tokens, "ToLower", 1, found));
		tokens = replaceTokens(tokens, found - 1, pos, op.arg01, 88);

		found = findTokens(tokens, pos, "ToLower", 88);
	}
}


/*
 * Concat --> +
 */
void updateConcat(std::vector<std::pair<std::string, int>> &tokens) {
	// replace concat --> +
	int found = findTokens(tokens, 0, "Concat", 88);
	while (found != -1) {
		tokens[found] = std::make_pair("+", 88);
		found = findTokens(tokens, found, "Concat", 88);
	}
}

/*
 * Length --> ""
 */
void updateLength(std::vector<std::pair<std::string, int>> &tokens) {
	// replace Length --> ""
	int found = findTokens(tokens, 0, "Length", 88);
	while (found != -1) {
		std::vector<std::pair<std::string, int>> tmp;
		for (int i = 0; i < found; ++i)
			tmp.emplace_back(tokens[i]);
		tmp.push_back(std::make_pair("+", 88));
		tmp.push_back(std::make_pair("0", 82));
		for (int i = found + 1; i < (int)tokens.size(); ++i)
			tmp.emplace_back(tokens[i]);
		tokens.clear();
		tokens = tmp;
		found = findTokens(tokens, found, "Length", 88);
	}
}

/*
 * "abcdef" --> 6
 */
void updateConst(std::vector<std::pair<std::string, int>> &tokens) {
	/* replace const --> its length */

	for (unsigned i = 0; i < tokens.size(); ++i){
		if (tokens[i].second == 86){
			tokens[i].first = std::to_string(tokens[i].first.length() - 2);
			tokens[i].second = 82;
		}
	}
}

/*
 * (Str2Reg x)--> x
 */
void updateStr2Regex(std::vector<std::pair<std::string, int>> &tokens){
	int found = findTokens(tokens, 0, "Str2Reg", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);

		StringOP op(findStringOP(tokens, "Str2Reg", 1, found));
		tokens = replaceTokens(tokens, found - 1, pos, op.arg01, 88);

		found = findTokens(tokens, pos, "Str2Reg", 88);
	}
}

/*
 *
 */
void updateRegexStar(std::vector<std::pair<std::string, int>> &tokens, int &regexCnt){
	std::string regexPrefix = "__regex_";

	int found = findTokens(tokens, 0, "RegexStar", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);

		StringOP op(findStringOP(tokens, "RegexStar", 1, found));

		std::vector<std::pair<std::string, int>> addingTokens;
		addingTokens.push_back(std::make_pair("*", 88));
		for (int i = found + 1; i < pos; ++i)
			addingTokens.emplace_back(tokens[i]);
		addingTokens.push_back(std::make_pair(regexPrefix + std::to_string(regexCnt++), 88));

		tokens = replaceTokens(tokens, found, pos - 1, addingTokens);
		found = findTokens(tokens, pos, "RegexStar", 88);
	}
}

/*
 *
 */
void updateRegexPlus(
		std::vector<std::pair<std::string, int>> &tokens,
		int &regexCnt){
	std::string regexPrefix = "__regex_";

	int found = findTokens(tokens, 0, "RegexPlus", 88);
	while (found != -1) {
		int pos = findCorrespondRightParentheses(found - 1, tokens);

		StringOP op(findStringOP(tokens, "RegexPlus", 1, found));

		std::vector<std::pair<std::string, int>> addingTokens;
		addingTokens.push_back(std::make_pair("*", 88));
		for (int i = found + 1; i < pos; ++i)
			addingTokens.emplace_back(tokens[i]);
		addingTokens.push_back(std::make_pair(regexPrefix + std::to_string(regexCnt++), 88));

		tokens = replaceTokens(tokens, found, pos - 1, addingTokens);
		found = findTokens(tokens, pos, "RegexPlus", 88);
	}
}

/*
 * xyz --> len_xyz
 */
void updateVariables(
		std::vector<std::pair<std::string, int>> &tokens,
		std::vector<std::string> strVars) {
	for (unsigned int i = 0; i < tokens.size(); ++i) {
		if (tokens[i].second == 88 && std::find(strVars.begin(), strVars.end(), tokens[i].first) != strVars.end()) {
			tokens[i].first = "len_" + tokens[i].first;
		}
	}
}

/*
 * contain a string variable
 */
bool strContaintStringVar(std::string notStr, std::vector<std::string> strVars) {
	if (notStr.find("Length") != std::string::npos)
		return false;
	for (const auto& s : strVars) {
		if (notStr.find(s) != std::string::npos)
			return true;
	}
	return false;
}

/*
 *
 */
std::vector<std::string> _collectAlternativeComponents(std::string str){
	std::vector<std::string> result;
	int counter = 0;
	unsigned int startPos = 0;
	for (unsigned int j = 0; j < str.length(); ++j) {
		if (str[j] == ')'){
			counter--;
		}
		else if (str[j] == '('){
			counter++;
		}
		else if ((str[j] == '|' || str[j] == '~') && counter == 0) {
			result.emplace_back(str.substr(startPos, j - startPos));
			startPos = j + 1;
		}
	}
	if (startPos != 0)
		result.emplace_back(str.substr(startPos, str.length() - startPos));
	return result;
}

/*
 *
 */
int _findCorrespondRightParentheses(int leftParentheses, std::string str){
	assert (str[leftParentheses] == '(');
	int counter = 1;
	for (unsigned int j = leftParentheses + 1; j < str.length(); ++j) {
		if (str[j] == ')'){
			counter--;
			if (counter == 0){
				return j;
			}
		}
		else if (str[j] == '('){
			counter++;
		}
	}
	return -1;
}

/*
 *
 */
std::vector<std::vector<std::string>> _parseRegexComponents(std::string str){
//	printf("%d parsing: \"%s\"\n", __LINE__, str.c_str());
	if (str.length() == 0)
		return {};

	std::vector<std::vector<std::string>> result;

	std::vector<std::string> alternativeRegex = _collectAlternativeComponents(str);
	if (alternativeRegex.size() != 0){
		for (unsigned int i = 0; i < alternativeRegex.size(); ++i) {
			std::vector<std::vector<std::string>> tmp = _parseRegexComponents(alternativeRegex[i]);
			assert(tmp.size() <= 1);
			if (tmp.size() == 1)
				result.emplace_back(tmp[0]);
		}
		return result;
	}

	size_t leftParentheses = str.find('(');
//	if (leftParentheses == std::string::npos || str[str.length() - 1] == '*' || str[str.length() - 1] == '+')
	if (leftParentheses == std::string::npos)
		return {{str}};

	/* abc(def)* */
	if (leftParentheses != 0) {
		std::string header = str.substr(0, leftParentheses);
		std::vector<std::vector<std::string>> rightComponents = _parseRegexComponents(str.substr(leftParentheses));
		for (unsigned int i = 0; i < rightComponents.size(); ++i) {
			std::vector<std::string> tmp = {header};
			tmp.insert(tmp.end(), rightComponents[i].begin(), rightComponents[i].end());
			result.emplace_back(tmp);
		}
		return result;
	}

	int rightParentheses = _findCorrespondRightParentheses(leftParentheses, str);
	if (rightParentheses < 0) {
		assert (false);
	}
	else if (rightParentheses == (int)str.length() - 1){
		/* (a) */
		return _parseRegexComponents(str.substr(1, str.length() - 2));
	}
	else if (rightParentheses == (int)str.length() - 2 && (str[str.length() - 1] == '*' || str[str.length() - 1] == '+')){
		/* (a)* */
		return {{str}};
	}

	else {
		int pos = rightParentheses;
		std::string left, right;
		if (str[rightParentheses + 1] == '*' || str[rightParentheses + 1] == '+'){
			pos++;
			left = str.substr(leftParentheses, pos - leftParentheses + 1);
			right = str.substr(pos + 1);
		}
		else if (str[pos] != '|' || str[pos] != '~') {
			left = str.substr(leftParentheses + 1, pos - leftParentheses - 1);
			right = str.substr(pos + 1);
		}
		else {
			assert (false);
			/* several options ab | cd | ef */
		}

		if (str[pos] != '|' || str[pos] != '~') {
			std::vector<std::vector<std::string>> leftComponents = _parseRegexComponents(left);
			std::vector<std::vector<std::string>> rightComponents = _parseRegexComponents(right);
			if (leftComponents.size() > 0) {
				if (rightComponents.size() > 0) {
					for (unsigned int i = 0; i < leftComponents.size(); ++i)
						for (unsigned int j = 0; j < rightComponents.size(); ++j) {
							std::vector<std::string> tmp;
							tmp.insert(tmp.end(), leftComponents[i].begin(), leftComponents[i].end());
							tmp.insert(tmp.end(), rightComponents[j].begin(), rightComponents[j].end());
							result.emplace_back(tmp);
						}
				}
				else {
					result.insert(result.end(), leftComponents.begin(), leftComponents.end());
				}
			}
			else {
				if (rightComponents.size() > 0) {
					result.insert(result.end(), rightComponents.begin(), rightComponents.end());
				}
			}

			return result;
		}
	}
	return {};
}

/*
 * AutomataDef to const
 */
std::string extractConst(std::string str) {
	if (str[0] == '(' && str[str.length() - 1] == ')') { /*(..)*/
		str = str.substr(1, str.length() - 2);
		/* find space */
		std::size_t found = str.find(' ');
		assert (found != std::string::npos);
		found = found + 1;
		/* find $$ */
		if (found >= str.length())
			return "";
		if (str[found] == '$') {
			if (str[found + 1] == '$') {
				/* find !! */
				found = str.find("!!");
				assert (found != std::string::npos);
				found = found + 2;
				str = "\"" + str.substr(found) + "\"";
			}
		}
		else {
			str = str.substr(found);
			if (str.length() > 2)
				if (str[0] == '|') {
					str = str.substr(1, str.length() - 2);
					if (str[0] == '\"')
						str = str.substr(1, str.length() - 2);
				}

			str = "\"" + str + "\"";
		}
	}
	else {
		assert (str[0] == '\"');
		if (str[str.length() - 1] == '\"')
			str = str.substr(1, str.length() - 2);
		else {
			/* "abc"_number */
			for (unsigned int i = str.length() - 1; i >= 0; --i)
				if (str[i] == '_') {
					assert (str[i - 1] == '\"');
					str = str.substr(1, i - 2);
					break;
				}
		}
		if (str[0] == '$') {
			if (str[1] == '$') {
				/* find !! */
				std::size_t found = str.find("!!");
				assert (found != std::string::npos);
				found = found + 2;
				str = str.substr(found);
			}
		}
		str = "\"" + str + "\"";
	}
	__debugPrint(logFile, "%d extractConst: --%s--\n", __LINE__, str.c_str());
	return str;
}

/*
 * check whether the list does not have variables
 */
bool hasNoVar(std::vector<std::string> list){
	for (unsigned int i = 0; i < list.size(); ++i)
		if (list[i][0] != '\"')
			return false;
	return true;
}

/*
 * Get all chars in consts
 */
std::set<char> getUsedChars(std::string str){
	std::set<char> result;

	int textState = 0; /* 1 -> "; 2 -> ""; 3 -> \; */

	for (unsigned int i = 0; i < str.length(); ++i) {
		if (str[i] == '"') {
			switch (textState) {
				case 1:
					textState = 2;
					break;
				case 3:
					textState = 1;
					result.emplace(str[i]);
					break;
				default:
					textState = 1;
					break;
			}
		}
		else if (str[i] == '\\') {
			switch (textState) {
				case 3:
					result.emplace(str[i]);
					textState = 1;
					break;
				default:
					textState = 3;
					break;
			}
		}
		else if (textState == 1 || textState == 3) {

			if (str[i] == 't' && textState == 3)
				result.emplace('\t');
			else
				result.emplace(str[i]);

			textState = 1;
		}
	}

	return result;
}

/*
 *
 */
void prepareEncoderDecoderMap(std::string fileName){
	FILE* in = fopen(fileName.c_str(), "r");
	if (!in) {
		printf("%d %s", __LINE__, fileName.c_str());
		throw std::runtime_error("Cannot open input file!");
	}

	std::set<char> tobeEncoded = {'?', '\\', '|', '"', '(', ')', '~', '&', '\t', '\'', '+', '%', '#', '*'};
	std::set<char> encoded;
	char buffer[5000];
	bool used[255];
	memset(used, sizeof used, false);
	while (!feof(in)) {
		/* read a line */
		if (fgets(buffer, 5000, in) != NULL){
			std::set<char> tmp = getUsedChars(buffer);
			for (const auto& ch : tmp) {
				used[(int)ch] = true;
				if (tobeEncoded.find(ch) != tobeEncoded.end())
					encoded.emplace(ch);
			}
		}
	}
	pclose(in);

	std::vector<char> unused;
	for (unsigned i = '0'; i <= '9'; ++i)
		if (used[i] == false)
			unused.emplace_back(i);

	for (unsigned i = 'a'; i <= 'z'; ++i)
		if (used[i] == false)
			unused.emplace_back(i);

	for (unsigned i = 'A'; i <= 'Z'; ++i)
			if (used[i] == false)
				unused.emplace_back(i);

	__debugPrint(logFile, "%d *** %s ***: unused = %ld, encoded = %ld\n", __LINE__, __FUNCTION__, unused.size(), encoded.size());
	assert(unused.size() >= encoded.size());


	for (const auto& ch : unused)
		__debugPrint(logFile, "%c ", ch);
	__debugPrint(logFile, "\n");

	unsigned cnt = 0;
	for (const char& ch : encoded) {
		ENCODEMAP[ch] = unused[cnt];
		DECODEMAP[unused[cnt]] = ch;
		cnt++;
	}
}

/*
 * "GrammarIn" -->
 * it is the rewriteGRM callee
 */
void rewriteGRM(std::string s,
		std::map<std::string, std::vector<std::vector<std::string>>> equalitiesMap,
		std::map<std::string, std::string> constMap,
		std::vector<std::string> &definitions,
		std::vector<std::string> &constraints) {

	__debugPrint(logFile, "%d CFG constraint: %s\n", __LINE__, s.c_str());
	/* step 1: collect var that is the next token after GrammarIn */
	unsigned int pos = s.find("GrammarIn");
	assert(pos != std::string::npos);

	assert(s[pos + 9] == ' ');
	pos = pos + 9;
	while (s[pos] == ' ' && pos < s.length())
		pos++;

	std::string varName = "";
	while (s[pos] != ' ' && pos < s.length()) {
		varName = varName + s[pos];
		pos++;
	}

	__debugPrint(logFile, "%d CFG var: %s\n", __LINE__, varName.c_str());

	//assert(equalitiesMap[varName].size() > 0);

	/* step 2: collect the regex value of varName*/
	std::string result = "";
	for (unsigned int i = 0; i < equalitiesMap[varName].size(); ++i) {
		if (hasNoVar(equalitiesMap[varName][i])) {

			std::vector<std::string> components = equalitiesMap[varName][i];

			displayListString(components, "zxxxxxxxxx");

			/* create concat for each pair */
			for (unsigned int j = 0; j < components.size(); ++j) {
				std::string content = components[j].substr(1, components[j].length() - 2);
				if (components[j].find('*') != std::string::npos) {
					unsigned int leftParentheses = components[j].find('(');
					unsigned int rightParentheses = components[j].find(')');

					std::string tmp = components[j].substr(leftParentheses + 1, rightParentheses - leftParentheses - 1);
					__debugPrint(logFile, "%d: lhs = %d, rhs = %d, str = %s --> %s (%s) \n", __LINE__, leftParentheses, rightParentheses, components[j].c_str(), tmp.c_str(), constMap[content].c_str());

					definitions.emplace_back("(declare-fun " + constMap[content] + "_100 () String)\n");
					constraints.emplace_back("(assert (RegexIn " + constMap[content] + "_100 (RegexStar (Str2Reg \"" + tmp + "\"))))\n");

					if (result.length() > 0)
						result = "(Concat " + result + " " + constMap[content]+ "_100)";
					else
						result = constMap[content] + "_100";
				}
				else if (components[j].find('+') != std::string::npos) {
					unsigned int leftParentheses = components[j].find('(');
					unsigned int rightParentheses = components[j].find(')');
					std::string tmp = components[j].substr(leftParentheses + 1, rightParentheses - leftParentheses - 1);

					definitions.emplace_back("(declare-fun " + constMap[content] + "_100 () String)\n");
					constraints.emplace_back("(assert (RegexIn " + constMap[content] + "_100 (RegexPlus (Str2Reg \"" + tmp + "\"))))\n");
					if (result.length() > 0)
						result = "(Concat " + result + " " + constMap[content]+ "_100)";
					else
						result = constMap[content] + "_100";
				}
				else {
					if (result.length() > 0)
						result = "(Concat " + result + " " + components[j] + ")";
					else
						result = components[j];
				}
			}
		}
		else {
			__debugPrint(logFile, "%d rewriteGRM something here: equalMap size = %ld\n", __LINE__, equalitiesMap[varName][i].size());
			// displayListString(_equalMap[varName][i], "\t>> ");
		}
	}

	//assert(result.length() > 0);

	result = "(assert (= " + varName + " " + result + "))\n";
	constraints.emplace_back(result);
	__debugPrint(logFile, "%d >> %s\n", __LINE__, result.c_str());
}

/*
 * replace the CFG constraint by the regex constraints
 * it is the rewriteGRM caller
 */
void toNonGRMFile(
		std::string inputFile,
		std::string outFile,
		std::map<std::string, std::vector<std::vector<std::string>>> equalitiesMap,
		std::map<std::string, std::string> constMap) {
	__debugPrint(logFile, "%d *** %s ***\n", __LINE__, __FUNCTION__);

	FILE* in = fopen(inputFile.c_str(), "r");
	std::ofstream out;
	out.open(outFile.c_str(), std::ios::out);

	if (!in){
		printf("%d %s", __LINE__, inputFile.c_str());
		throw std::runtime_error("Cannot open input file!");
	}

	std::vector<std::string> definitions;
	std::vector<std::string> constraints;

	char buffer[5000];
	std::vector<std::string> strVars;

	while (!feof(in)){
		/* read a line */
		if (fgets(buffer, 5000, in) != NULL){

			if (strcmp("(check-sat)", buffer) == 0 || strcmp("(check-sat)\n", buffer) == 0) {
				break;
			}
			else {
				std::string tmp = buffer;
				if (tmp.find("GrammarIn") != std::string::npos) {
					rewriteGRM(tmp, equalitiesMap, constMap, definitions, constraints);
				}
				else
					constraints.emplace_back(tmp);
			}
		}
	}

	/* write everything to the file */
	for (unsigned int i = 0; i < definitions.size(); ++i) {
		out << definitions[i];
	}

	for (unsigned int i = 0; i < constraints.size(); ++i) {
		out << constraints[i];
	}

	out << "(check-sat)\n(get-model)\n";
	out.close();
	pclose(in);
}

/*
 * "abc123" 			--> 6
 * Concat abc def --> + len_abc len_def
 * Length abc 		--> len_abc
 */
void toLengthLine(
		std::vector<std::pair<std::string, int>> tokens,
		std::vector<std::string> &strVars,
		bool handleNotOp,
		std::map<StringOP, std::string> rewriterStrMap,
		int &regexCnt,
		std::vector<std::string> &smtVarDefinition,
		std::vector<std::string> &smtLenConstraints){

	std::set<std::string> constList;

	bool declare = false;
	for (const auto& token : tokens)
		if (token.second == 64 || token.second == 65) {
			declare = true;
			break;
		}

	if (declare == true) {
		bool stringVarDef = false;
		std::string newStr = "";
		for (const auto& token : tokens)
			if (token.second == 88 && token.first.compare("String") == 0){
				stringVarDef = true;
				break;
			}

		if (stringVarDef == true) {
			smtLenConstraints.emplace_back("(assert (>= len_" + tokens[2].first + " 0))\n");
			strVars.emplace_back(tokens[2].first); /* list of string variables */
			if (tokens[2].first.find("const_") != 0)
				newStr = redefineStringVar(tokens[2].first);
			else
				newStr = "";
		}
		else {
			newStr = redefineOtherVar(tokens[2].first, tokens[tokens.size() - 2].first);
		}

		if (newStr.length() > 0)
			smtVarDefinition.emplace_back(newStr);

		return;
	}

	for (unsigned i = 0; i < tokens.size(); ++i)
		if (tokens[i].second == 86){
			std::string s = "";
			for (unsigned j = 0; j < tokens[i].first.length(); ++j)
				if (tokens[i].first[j] == '\\'){
					if (j + 2 < tokens[i].first.length() && tokens[i].first[j + 1] == 't') {
						s += '\t';
						++j;
					}
					else if (j + 2 < tokens[i].first.length()){
						s += tokens[i].first[j + 1];
						++j;
					}
				}
				else
					s += tokens[i].first[j];
			tokens[i].first = s;
		}

	updateImplies(tokens);
	__debugPrint(logFile, "%d *** %s ***: %s\n", __LINE__, __FUNCTION__, sumTokens(tokens, 0, tokens.size() - 1).c_str());
	updateRegexIn(tokens);
	updateContain(tokens, rewriterStrMap);
	updateLastIndexOf(tokens, rewriterStrMap);
	updateIndexOf(tokens, rewriterStrMap);
	updateEndsWith(tokens, rewriterStrMap);
	updateStartsWith(tokens, rewriterStrMap);
	updateReplace(tokens, rewriterStrMap);
	updateReplaceAll(tokens, rewriterStrMap);

	updateToUpper(tokens);
	updateToLower(tokens);
	updateSubstring(tokens, rewriterStrMap);

	updateConst(tokens); /* "abcdef" --> 6 */
	updateStr2Regex(tokens);
	updateRegexStar(tokens, regexCnt);
	updateRegexPlus(tokens, regexCnt);



	updateConcat(tokens); /* Concat --> + */
	updateLength(tokens); /* Length --> "" */
	updateVariables(tokens, strVars); /* xyz --> len_xyz */

	__debugPrint(logFile, "%d *** %s ***: %s\n", __LINE__, __FUNCTION__, sumTokens(tokens, 0, tokens.size() - 1).c_str());
	smtLenConstraints.emplace_back(sumTokens(tokens, 0, tokens.size() - 1) + "\n");
}

/*
 *
 */
std::string encodeSpecialChars(std::string constStr){
	std::string strTmp = "";

	for (unsigned i = 1 ; i < constStr.length() - 1; ++i){
		if (constStr[i] == '\\') {
			if (i < constStr.length() - 1) {
				if (constStr[i + 1] == 't'){
					strTmp += ENCODEMAP['\t'];
					i++;
				}
				else if (constStr[i + 1] == '"') {
					strTmp += ENCODEMAP['"'];
					i++;
				}
				else if (constStr[i + 1] == '\'') {
					strTmp += ENCODEMAP['\''];
					i++;
				}
				else {
					strTmp += constStr[i];
					strTmp += constStr[i];
				}
			}
			else {
				strTmp += constStr[i];
			}
		}
		else if (ENCODEMAP.find(constStr[i]) != ENCODEMAP.end())
				strTmp += ENCODEMAP[constStr[i]];
		else
			strTmp += constStr[i];
	}

	return '"' + strTmp + '"';
}

/*
 * read SMT file
 */
void encodeSpecialChars(std::string inputFile, std::string outFile){
	std::vector<std::vector<std::pair<std::string, int>>> fileTokens = parseFile(inputFile);
	std::vector<std::vector<std::string>> newtokens;
	std::set<std::string> constStr;

	for (const auto& tokens : fileTokens) {
		bool add = true;
		std::vector<std::string> listTokens;
		for (const auto& token : tokens) {
			if (token.second == 81) { /* get model */
				add = false;
				break;
			}
			else if (token.second == 86) /* string */{
				std::string tmp = encodeSpecialChars(token.first);
				constStr.emplace(tmp);
				listTokens.emplace_back(tmp);
			}
			else {
				listTokens.emplace_back(token.first);
			}
		}
		if (add == true)
			newtokens.emplace_back(listTokens);
	}

	std::ofstream out;
	out.open(outFile.c_str(), std::ios::out);

	for (const auto& tokens : newtokens) {
		for (const auto &token : tokens) {
			out << token << " ";
			out.flush();
		}
		out << "\n";
	}
	out.flush();
	out.close();
}



/*
 *
 */
std::string encodeHex(std::string constStr){
	std::string newStr = "__cOnStStR_";
	for (unsigned i = 1 ; i < constStr.length() - 1; ++i) {
		newStr = newStr + int_to_hex((int)constStr[i]);
		if (constStr[i] == '\\' && constStr[i + 1] == '\\')
			++i;
	}
	return newStr;
}

/*
 * read SMT file
 */
void encodeHex(std::string inputFile, std::string outFile){

	std::vector<std::vector<std::pair<std::string, int>>> fileTokens = parseFile(inputFile);
	std::vector<std::vector<std::string>> newtokens;
	std::set<std::string> constStr;
	for (const auto& tokens : fileTokens) {
		std::vector<std::string> listTokens;
		for (const auto &token : tokens) {
			if (token.second == 86) /* string */{
				std::string tmp = encodeHex(token.first);
				constStr.emplace(tmp);
				listTokens.emplace_back(tmp);
			}
			else {
				listTokens.emplace_back(token.first);
			}
		}
		newtokens.emplace_back(listTokens);
	}

	std::ofstream out;
	out.open(outFile.c_str(), std::ios::out);

	for (const auto& s : constStr) {
		out << "(declare-const " << s << " String)\n";
		out.flush();
	}

	for (const auto& tokens : newtokens) {
		for (const auto &token : tokens) {
			out << token << " ";
			out.flush();
		}
		out << "\n";
	}

	out.flush();
	out.close();
}

/*
 * read SMT file
 * convert the file to length file & store it
 */
void toLengthFile(
		std::string inputFile, bool handleNotOp,
		std::map<StringOP, std::string> rewriterStrMap,
		int &regexCnt,
		std::vector<std::string> &smtVarDefinition,
		std::vector<std::string> &smtLenConstraints){
	smtVarDefinition.clear();
	smtLenConstraints.clear();

	std::vector<std::vector<std::string>> newtokens;
	std::vector<std::vector<std::pair<std::string, int>>> fileTokens = parseFile(inputFile);
	std::vector<std::string> strVars;
	std::set<std::string> constStr;
	for (const auto& tokens : fileTokens) {
		toLengthLine(tokens, strVars, handleNotOp, rewriterStrMap, regexCnt, smtVarDefinition, smtLenConstraints);
	}

	__debugPrint(logFile, "Print smtLength: %d \n", __LINE__);
	displayListString(smtLenConstraints, "");
}

/*
 *
 */
std::string decodeStr(std::string s){
	__debugPrint(logFile, "%d *** %s ***: %s: ", __LINE__, __FUNCTION__, s.c_str());
	std::string tmp = "";
	for (unsigned i = 0 ; i < s.size(); ++i) {
		tmp += s[i];
		if (s[i] == '\\' && i != s.size() - 1 && s[i + 1] == '\\')
			++i;
	}
	s = tmp;
	tmp = "";

	for (unsigned int i = 0; i < s.length(); ++i){
		if (DECODEMAP.find(s[i]) != DECODEMAP.end()){
			if ((char)DECODEMAP[s[i]] != '\t')
				tmp += (char)DECODEMAP[s[i]];
			else
				tmp += "\\t";
		}
		else
			tmp += s[i];

	}
	__debugPrint(logFile, " %s\n", tmp.c_str());
	return tmp;
}


/*
 * read SMT file
 * add length constraints and write it
 * rewrite CFG
 */
void addConstraintsToSMTFile(
		std::string inputFile, /* nongrm file */
		std::map<std::string, std::vector<std::vector<std::string>>> _equalMap,
		std::vector<std::string> lengthConstraints,
		std::string outFile){
	FILE* in = fopen(inputFile.c_str(), "r");
	if (!in) {
		printf("%d %s", __LINE__, inputFile.c_str());
		throw std::runtime_error("Cannot open input file!");
	}
	std::ofstream out;
	out.open(outFile.c_str(), std::ios::out);

	std::vector<std::string> constraints;

	char buffer[5000];

	while (!feof(in)) {
		/* read a line */
		if (fgets(buffer, 5000, in) != NULL){
			if (strcmp("(check-sat)", buffer) == 0 || strcmp("(check-sat)\n", buffer) == 0) {
				break;
			}
			else {
				std::string tmp = buffer;
				/* rewrite CFG */
				if (tmp.find("GrammarIn") != std::string::npos) {
					assert(false);
					// rewriteGRM(tmp, _equalMap, newVars, definitions, constraints);
				}
				else {
					constraints.emplace_back(tmp);
				}
			}
		}
	}

	/* write everything to the file */
	for (unsigned int i = 0; i < constraints.size(); ++i)
		out << constraints[i];

	for (unsigned int i = 0 ; i < lengthConstraints.size(); ++i) {
		/* add length constraints */
		out << lengthConstraints[i];
		out.flush();
	}

	out << "(check-sat)\n(get-model)\n";

	pclose(in);

	out.flush();
	out.close();
}
