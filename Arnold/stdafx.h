#ifndef _ARNOLD_ALEMBIC_FOUNDATION_H_
#define _ARNOLD_ALEMBIC_FOUNDATION_H_


   #include <ai.h>
   #include "CommonAlembic.h"

   #ifndef AtULong

      typedef void               AtVoid;
      typedef char               AtChar;
      typedef char               AtBoolean;

      typedef unsigned short     AtUShort;
      typedef short              AtShort;
      typedef unsigned long      AtULong;
      typedef long               AtLong;

      typedef signed long long   AtLLong;
      typedef unsigned long long AtULLong;


      typedef int                AtInt;
      typedef unsigned int       AtUInt;

      typedef float              AtFloat;
      typedef double             AtDouble;

      typedef const AtChar*      AtConstCharPtr;
      typedef AtVoid*            AtVoidPtr;

   #endif


#endif 