//
// Copyright 2010 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.  
//

#pragma once

/**
 * This class is for easier and safer descriptions of parameter blocks. 
 * ParamBlockDescUtil is based on ParamBlockDesc2, and is intended to be used created incrementally 
 * using a series of function calls. This is different from the traditional approach of passing 
 * a massive variable length list of arguments to the constructor of ParamBlockDesc2. 
 * The advantages are additional type safety, and explicit support from the IDE (e.g. code completion).
 * 
 * \par This class assumes the parameter block is auto-constructed by 3ds Max (meaning it 
 * assigns the parameter block as a reference to your plug-in) and has an auto-constructed UI. 
 * You would use it normally it by deriving a class which has an empty constructors 
 * and calls the incremental construction functions from its own constructor
 * Once constructed, the parameter block descriptor must never be modified.
 *
 * \note: Pass the flag BEGIN_EDIT_MOTION to the constructor \c flags argument 
 * if using this for a controller.
 */
class ParamBlockDescUtil
    : public ParamBlockDesc2 
{
private:

    BOOL finished;

public:
	///* From the header:
	ParamBlockDescUtil(  ClassDesc2* classDesc  ,
		MCHAR* internalName, 
		StringResID localNameID, 
		BlockID id, 
		int flagMask       )        : ParamBlockDesc2(
		id, 
		internalName, 
		localNameID, 
		classDesc, 
		P_AUTO_CONSTRUCT + P_AUTO_UI,
		// [<auto_construct_block_refno>]
		// [<auto_ui_parammap_specs>]
		flagMask,
		0, // could be APPENDROLL_CLOSED if the roll-up should be closed by default
		dlgProc,
		end),
		finished(FALSE){};  
	//*/
    ParamBlockDescUtil
        (
			BlockID     id              ,
            MCHAR* internalName, 
            StringResID localNameID, 
            ClassDesc2* classDesc       , 
            int         flagMask        , // Set to BEGIN_EDIT_MOTION for controllers.
            // ---------------------
			int         refNumber       ,
            ResID       dialogTemplateID, 
            StringResID panelTitleID, 
            ParamMap2UserDlgProc* dlgProc = NULL
        )
        : ParamBlockDesc2(
            id, 
            internalName, 
            localNameID, 
            classDesc, 
            P_AUTO_CONSTRUCT + P_AUTO_UI,
                // [<auto_construct_block_refno>]
                refNumber,
                // [<auto_ui_parammap_specs>]
                dialogTemplateID,
                panelTitleID,
                flagMask,
                0, // could be APPENDROLL_CLOSED if the roll-up should be closed by default
                dlgProc,
            end),
        finished(FALSE)
    {
    }

    /**
     * After the last parameter is added, call this function. 
     * This is used to assure internally that no one tries to add additional parameters.
     */
    void SetFinished()
    {
        DbgAssert(!finished);
        finished = TRUE;
    }

    /**
     * Adds an angle parameter with an associated spinner controller.
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     * \param  min An argument of type float.
     * \param  max An argument of type float.
     * \param  def An argument of type float.
     * \param  editID An argument of type ResID.
     * \param  spinnerID An argument of type ResID.
     * \param  incAmount An argument of type float.
     */
    void AddAngleSpinnerParam(ParamID id, MCHAR* internalName, StringResID localNameID, float min, float max, float def, ResID editID, ResID spinnerID, float incAmount = SPIN_AUTOSCALE)
    {
        DbgAssert(!finished);
        AddAngleParam(id, internalName, localNameID);
        SetRangeAndDefault(id, min, max, def);
        AddEditAndSpinner(id, editID, spinnerID, incAmount);
    }

    
    /**
     * Adds a float parameter with an associated spinner controller.
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     * \param  min An argument of type float.
     * \param  max An argument of type float.
     * \param  def An argument of type float.
     * \param  editID An argument of type ResID.
     * \param  spinnerID An argument of type ResID.
     * \param  incAmount An argument of type float.
     */
    void AddFloatSpinnerParam(ParamID id, MCHAR* internalName, StringResID localNameID, float min, float max, float def, ResID editID, ResID spinnerID, float incAmount = SPIN_AUTOSCALE)
    {
        DbgAssert(!finished);
        AddFloatParam(id, internalName, localNameID);
        SetRangeAndDefault(id, min, max, def);
        AddEditAndSpinner(id, editID, spinnerID, incAmount);
    }
        
    /**
     * Adds a node parameter, with a node pick button. 
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     * \param  buttonID An argument of type ResID.
     */
    void AddNodePickParam(ParamID id, MCHAR* internalName, StringResID localNameID, ResID buttonID)
    {
        DbgAssert(!finished);
        AddParam(id, internalName, TYPE_INODE, 0, localNameID, end);
        ParamOption(id, p_ui, TYPE_PICKNODEBUTTON, buttonID, end);
    }

    /**
     * Sets the range and default values of the specified float parameter.
     * 
     * \param  id An argument of type ParamID.
     * \param  min An argument of type float.
     * \param  max An argument of type float.
     * \param  def An argument of type float.
     */
    void SetRangeAndDefault(ParamID id, float min, float max, float def)
    {
        DbgAssert(!finished);
        DbgAssert(min <= max);
        DbgAssert(def >= min);
        DbgAssert(def <= max);
        ParamOption(id, p_range, min, max, end);
        ParamOption(id, p_default, def, end);
    }

    /**
     * Adds an angle parameter. 
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     */
    void AddAngleParam(ParamID id, MCHAR* internalName, StringResID localNameID, int flags = P_ANIMATABLE )
    {
        DbgAssert(!finished);
        AddParam(id, internalName, TYPE_ANGLE, flags, localNameID, end);        
    }

   /**
     * Adds a int parameter.
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     */
    void AddIntParam(ParamID id, MCHAR* internalName, StringResID localNameID, int flags = P_ANIMATABLE)
    {
        DbgAssert(!finished);
        AddParam(id, internalName, TYPE_INT, flags, localNameID, end);        
    }

     /**
     * Adds a filename parameter.
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     */
    void AddFilenameParam(ParamID id, MCHAR* internalName, StringResID localNameID, int flags = 0 )
    {
        DbgAssert(!finished);
        AddParam(id, internalName, TYPE_FILENAME, flags, localNameID, end);        
    }
	
	/**
     * Adds a float parameter.
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     */
    void AddFloatParam(ParamID id, MCHAR* internalName, StringResID localNameID, int flags = P_ANIMATABLE)
    {
        DbgAssert(!finished);
        AddParam(id, internalName, TYPE_FLOAT, flags, localNameID, end);        
    }

	
    /**
     * Adds a bool parameter.
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     */
    void AddBooleanParam(ParamID id, MCHAR* internalName, StringResID localNameID, int flags = P_ANIMATABLE)
    {
        DbgAssert(!finished);
        AddParam(id, internalName, TYPE_BOOL, flags, localNameID, end);        
    }

    /**
     * Adds a bool parameter.
     * 
     * \param  id An argument of type ParamID.
     * \param  internalName An argument of type char *.
     * \param  localNameID An argument of type StringResID.
     */
    void AddStringParam(ParamID id, MCHAR* internalName, StringResID localNameID)
    {
        DbgAssert(!finished);
        AddParam(id, internalName, TYPE_STRING, 0, localNameID, end);        
    }

	/**
     * Adds an edit control and and a spinner control to the specified parameter. 
     * 
     * \param  id An argument of type ParamID.
     * \param  editID An argument of type ResID.
     * \param  spinnerID An argument of type ResID.
     * \param  incAmount An argument of type float.
     */
    void AddEditAndSpinner(ParamID id, ResID editID, ResID spinnerID, float incAmount = SPIN_AUTOSCALE)
    {
        DbgAssert(!finished);
        ParamOption(id, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, editID, spinnerID, incAmount);
    }

    
    /**
     * Associates a button with a parameter, which become a pick-node button.
     * This means when the user clicks on it, 3ds Max goes into node picking mode.
     * 
     * \param  id An argument of type ParamID.
     * \param  buttonID An argument of type ResID.
     */
    void AddPickNodeButton(ParamID id, ResID buttonID)
    {
        DbgAssert(!finished);
        ParamOption(id, p_ui, TYPE_PICKNODEBUTTON, buttonID, end);
    }
};