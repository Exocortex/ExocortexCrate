

class AlembicCustomAttributesEx
{
   typedef std::map<XSI::CString, Abc::OArrayProperty> propMap;
   propMap customProps;

   
public:


   XSI::CStatus defineCustomAttributes(XSI::Geometry geo, Abc::OCompoundProperty& argGeomParams, const AbcA::MetaData& metadata, unsigned int animatedTs);
   XSI::CStatus exportCustomAttributes(XSI::Geometry geo);

};

