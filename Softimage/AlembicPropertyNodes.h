#ifndef _ALEMBIC_PROPERTY_NODES_H_
#define _ALEMBIC_PROPERTY_NODES_H_

//#include <xsi_pluginregistrar.h>

XSI::CStatus Register_alembic_string_array( XSI::PluginRegistrar& in_reg );
XSI::CStatus Register_alembic_float_array( XSI::PluginRegistrar& in_reg );
XSI::CStatus Register_alembic_vec2f_array( XSI::PluginRegistrar& in_reg );
XSI::CStatus Register_alembic_vec3f_array( XSI::PluginRegistrar& in_reg );
XSI::CStatus Register_alembic_vec4f_array( XSI::PluginRegistrar& in_reg );
XSI::CStatus Register_alembic_int_array( XSI::PluginRegistrar& in_reg );


#endif