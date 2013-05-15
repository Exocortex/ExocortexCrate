#include "CommonAlembic.h"
#include "CommonRegex.h"
#include <sstream>
#include <vector>
#include <cstdlib>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

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

namespace EnvVariables
{
	string readEnvVar(const string &str)
	{
		const char *env_value = getenv(str.c_str());
		if (!env_value)
		{
			ESS_LOG_WARNING("Environment variable \"" << str << "\" is invalid and is not replaced");
			return str;
		}
		return std::string(env_value);
	}

	string replace(const string &str)
	{
		vector<std::string> parts;
		boost::split(parts, str, boost::is_any_of("%"));
		if (parts.size() & 0x1 == 0)
		{
			ESS_LOG_ERROR("Need an even number of % in the string to delimit environment variables properly! Nothing is replaced in the string");
			return str;
		}

		// replace every odd-positioned string with the environment variable value!
		size_t finalSize = 2;
		bool isEnvVarRound = false;
		for (std::vector<std::string>::iterator beg = parts.begin(); beg != parts.end(); ++beg, isEnvVarRound=!isEnvVarRound)
		{
			if (isEnvVarRound)
			{
				if (beg->size())
					*beg = readEnvVar(*beg);
				else
					*beg = "%";
			}
			finalSize += beg->size();
		}

		// concat the vector!
		string ret;
		ret.reserve(finalSize);
		for (std::vector<std::string>::iterator beg = parts.begin(); beg != parts.end(); ++beg)
			ret += *beg;
		return ret;
	}
}


