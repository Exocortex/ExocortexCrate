
function CreateAlembicMetadataModifier selectedObject = (

	if selectedObject != undefined then (
		--only allow one modifier
		if selectedObject.modifiers["Alembic Metadata"] == undefined then (

			AlembicMetaDataModifier = EmptyModifier()
			AlembicMetaDataModifier.name = "Alembic Metadata"
			addmodifier selectedObject AlembicMetaDataModifier

			AlembicMetadataCA = attributes AlembicMetadata
			(
				parameters AlembicMetadataPRM rollout:AlembicMetadataRLT
				(
					name0 type:#string ui:eName0 default:""
					value0 type:#string ui:eValue0 default:""
					name1 type:#string ui:eName1 default:""
					value1 type:#string ui:eValue1 default:""
					name2 type:#string ui:eName2 default:""
					value2 type:#string ui:eValue2 default:""
					name3 type:#string ui:eName3 default:""
					value3 type:#string ui:eValue3 default:""
					name4 type:#string ui:eName4 default:""
					value4 type:#string ui:eValue4 default:""
					name5 type:#string ui:eName5 default:""
					value5 type:#string ui:eValue5 default:""
					name6 type:#string ui:eName6 default:""
					value6 type:#string ui:eValue6 default:""
					name7 type:#string ui:eName7 default:""
					value7 type:#string ui:eValue7 default:""
					name8 type:#string ui:eName8 default:""
					value8 type:#string ui:eValue8 default:""
					name9 type:#string ui:eName9 default:""
					value9 type:#string ui:eValue9 default:""
				)
					
				rollout AlembicMetadataRLT "Alembic Metadata"
				(
					edittext eName0 "n0" fieldWidth:120 labelOnTop:false
					edittext eValue0 "v0" fieldWidth:120 labelOnTop:false
					edittext eName1 "n1" fieldWidth:120 labelOnTop:false
					edittext eValue1 "v1" fieldWidth:120 labelOnTop:false
					edittext eName2 "n2" fieldWidth:120 labelOnTop:false
					edittext eValue2 "v2" fieldWidth:120 labelOnTop:false
					edittext eName3 "n3" fieldWidth:120 labelOnTop:false
					edittext eValue3 "v3" fieldWidth:120 labelOnTop:false
					edittext eName4 "n4" fieldWidth:120 labelOnTop:false
					edittext eValue4 "v4" fieldWidth:120 labelOnTop:false
					edittext eName5 "n5" fieldWidth:120 labelOnTop:false
					edittext eValue5 "v5" fieldWidth:120 labelOnTop:false
					edittext eName6 "n6" fieldWidth:120 labelOnTop:false
					edittext eValue6 "v6" fieldWidth:120 labelOnTop:false
					edittext eName7 "n7" fieldWidth:120 labelOnTop:false
					edittext eValue7 "v7" fieldWidth:120 labelOnTop:false
					edittext eName8 "n8" fieldWidth:120 labelOnTop:false
					edittext eValue8 "v8" fieldWidth:120 labelOnTop:false
					edittext eName9 "n9" fieldWidth:120 labelOnTop:false
					edittext eValue9 "v9" fieldWidth:120 labelOnTop:false
				)
			)

			custattributes.add selectedObject.modifiers["Alembic Metadata"] AlembicMetadataCA baseobject:false
		)
	)
)


function InitAlembicMetadataModifier selectedObject dname0 dvalue0 dname1 dvalue1 dname2 dvalue2 dname3 dvalue3 dname4 dvalue4 dname5 dvalue5 dname6 dvalue6 dname7 dvalue7 dname8 dvalue8 dname9 dvalue9 = (

	if selectedObject != undefined then 
	(
		selectedObject.modifiers["Alembic Metadata"].name0 = dname0;
		selectedObject.modifiers["Alembic Metadata"].value0 = dvalue0;
		selectedObject.modifiers["Alembic Metadata"].name1 = dname1;
		selectedObject.modifiers["Alembic Metadata"].value1 = dvalue1;
		selectedObject.modifiers["Alembic Metadata"].name2 = dname2;
		selectedObject.modifiers["Alembic Metadata"].value2 = dvalue2;
		selectedObject.modifiers["Alembic Metadata"].name3 = dname3;
		selectedObject.modifiers["Alembic Metadata"].value3 = dvalue3;
		selectedObject.modifiers["Alembic Metadata"].name4 = dname4;
		selectedObject.modifiers["Alembic Metadata"].value4 = dvalue4;
		selectedObject.modifiers["Alembic Metadata"].name5 = dname5;
		selectedObject.modifiers["Alembic Metadata"].value5 = dvalue5;
		selectedObject.modifiers["Alembic Metadata"].name6 = dname6;
		selectedObject.modifiers["Alembic Metadata"].value6 = dvalue6;
		selectedObject.modifiers["Alembic Metadata"].name7 = dname7;
		selectedObject.modifiers["Alembic Metadata"].value7 = dvalue7;
		selectedObject.modifiers["Alembic Metadata"].name8 = dname8;
		selectedObject.modifiers["Alembic Metadata"].value8 = dvalue8;
		selectedObject.modifiers["Alembic Metadata"].name9 = dname9;
		selectedObject.modifiers["Alembic Metadata"].value9 = dvalue9;
	)
)

---------------------------------------------------------------------------------------------------------
-- MacroScript to link the menu items to MAXScript functions

macroScript AlembicCreateMetadataModifier
    category:"Alembic" 
    buttonText:"Add Alembic Metadata..."
    tooltip:"Add Alembic Metadata Display Modifier To Current Selection..."
(
    on execute do 
    (
    	for i = 1 to selection.count do
    	(
    		CreateAlembicMetadataModifier selection[i]
    	)
    )
)
