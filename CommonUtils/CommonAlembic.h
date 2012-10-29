#ifndef __COMMON_ALEMBIC_H
#define __COMMON_ALEMBIC_H


#include <utility>
#include <limits>

#include <set>
#include <vector>
#include <map>
#include <list>
#include <deque>
#include <cmath>
#include <algorithm>

#include <stdexcept>
#include <exception>

#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include <iostream>

#include <time.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <boost/algorithm/string.hpp>
#include <boost/cstdint.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>
#include <boost/exception/all.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>

namespace fs = boost::filesystem;

#include <ImathMatrixAlgo.h>

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/Util/Murmur3.h>

namespace Alembic {
	namespace Abc {
		namespace ALEMBIC_VERSION_NS {
			using Imath::V4s;
			using Imath::V4i;
			using Imath::V4f;
			using Imath::V4d;
		}
	}
}

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace Abc = ::Alembic::Abc::ALEMBIC_VERSION_NS;
namespace AbcG = ::Alembic::AbcGeom::ALEMBIC_VERSION_NS;
namespace AbcU = ::Alembic::Util::ALEMBIC_VERSION_NS;

#include "CommonOS.h"
#include "CommonLog.h"
#include "CommonProfiler.h"

#endif // __COMMON_ALEMBIC_H
