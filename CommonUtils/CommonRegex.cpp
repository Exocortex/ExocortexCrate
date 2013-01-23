#include "CommonRegex.h"
#include <sstream>

using namespace std;
namespace SearchReplace
{
	ReplacePtr defaultNoReplace(new NoReplace());

	string NoReplace::replace(const string &str) const
	{
		return str;
	}

	RegexReplace::RegexReplace(const string &_exp, const string &_form): NoReplace(), exp(_exp), formatter(_form)
	{
	}

	string RegexReplace::replace(const string &str) const
	{
		ostringstream t(std::ios::out);
		ostream_iterator<char, char> oi(t);
		boost::regex_replace(oi, str.begin(), str.end(), exp, formatter, boost::match_default | boost::format_all);
		return t.str();
	}

	ReplacePtr createReplacer(const std::string *exp, const std::string *form)
	{
		return (exp == 0 || form == 0) ? defaultNoReplace : createReplacer(*exp, *form);
	}

	ReplacePtr createReplacer(const std::string &exp, const std::string &form)
	{
		if (exp.length() == 0 || form.length() == 0)
			return defaultNoReplace;
		return ReplacePtr(new RegexReplace(exp, form));
	}
}

