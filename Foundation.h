#ifndef _FOUNDATION_H_
#define _FOUNDATION_H_

#ifdef NOMINMAX
    #undef NOMINMAX
#endif

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <utility>
#include <limits>
#include <set>
#include <vector>
#include <map>
#include <list>
#include <stdexcept>
#include <exception>
#include <string>
#include <sstream>

#include <boost/cstdint.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>


typedef std::map<std::string,std::string> stringMap;
typedef std::map<std::string,std::string>::iterator stringMapIt;
typedef std::pair<std::string,std::string> stringPair;

#endif  // _FOUNDATION_H_
