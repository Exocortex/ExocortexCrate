#ifndef __ALEMBIC_LICENSING_H
#define __ALEMBIC_LICENSING_H
#include <maya/MGlobal.h>

// TODO: this has to be implemented properly
inline bool HasFullLicense() { return true; }

inline void EC_LOG_WARNING(MString message)
{
   MGlobal::displayWarning(message);
}

#endif // __ALEMBIC_LICENSING_H
