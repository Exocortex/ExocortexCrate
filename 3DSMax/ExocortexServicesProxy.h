#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H



#define ESS_CALLBACK_START(NodeName_CallbackName, ParamType )						\
	XSIPLUGINCALLBACK CStatus NodeName_CallbackName( ParamType in_ctxt ) {			\
		class Body {																\
		public:																		\
			static CStatus execute( ParamType in_ctxt ) {							\
				ESS_CPP_EXCEPTION_REPORTING_START

#define ESS_CALLBACK_END															\
				ESS_CPP_EXCEPTION_REPORTING_END										\
			}																		\
		};																			\
		ESS_STRUCTURED_EXCEPTION_REPORTING_START									\
			return Body::execute( in_ctxt );										\
		ESS_STRUCTURED_EXCEPTION_REPORTING_END										\
		return CStatus::Abort;														\
	}



#endif // __EXOCORTEX_SERVICES_PROXY_H