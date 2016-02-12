#ifndef __COMMON_ALEMBIC_H
#define __COMMON_ALEMBIC_H

#include <limits>
#include <utility>

#include <algorithm>
#include <cmath>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

#include <exception>
#include <stdexcept>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <boost/algorithm/string.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/variant.hpp>

namespace fs = boost::filesystem;

#include <ImathMatrixAlgo.h>

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcMaterial/All.h>
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
namespace AbcF = ::Alembic::AbcCoreFactory::ALEMBIC_VERSION_NS;
namespace Abc = ::Alembic::Abc::ALEMBIC_VERSION_NS;
namespace AbcG = ::Alembic::AbcGeom::ALEMBIC_VERSION_NS;
namespace AbcU = ::Alembic::Util::ALEMBIC_VERSION_NS;
namespace AbcM = ::Alembic::AbcMaterial::ALEMBIC_VERSION_NS;

#include "CommonLog.h"
#include "CommonOS.h"
#include "CommonProfiler.h"

#endif  // __COMMON_ALEMBIC_H
