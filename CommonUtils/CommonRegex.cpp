#include "CommonRegex.h"
#include <sstream>
#include <boost/regex.hpp>

using namespace std;
namespace SearchReplace
{
	class RegexReplace: public NoReplace
	{
	private:
		boost::regex exp;
		std::string formatter;
	public:
		RegexReplace(const std::string &exp, const std::string &form);

		virtual std::string replace(const std::string &str) const;
	};

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
		return (exp.length() == 0 || form.length() == 0) ? defaultNoReplace : ReplacePtr(new RegexReplace(exp, form));
	}
}

