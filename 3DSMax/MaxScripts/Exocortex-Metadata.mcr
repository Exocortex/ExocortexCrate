function CreateAlembicMetadataModifier selectedObject name value = (

	if selectedObject != undefined then (
		--only allow one modifier
		if selectedObject.modifiers["Alembic Metadata"] == undefined then (

			AlembicMetaDataModifier = EmptyModifier()
			AlembicMetaDataModifier.name = "Alembic Metadata"
			modCount = selectedObject.modifiers.count
			addmodifier selectedObject AlembicMetaDataModifier before:modCount
			selectedObject.modifiers["Alembic Metadata"].enabled = false
		)

		count = custattributes.count selectedObject.modifiers["Alembic Metadata"]
		count = count + 1

		evalstr = "AlembicMetadata = attributes AlembicMetadata\n" + 
		"(\n" +
		"	parameters AlembicMetadataPRM1 rollout:AlembicMetadataRLT1\n" +
		"	(\n" +
		"		_name type:#string ui:eName default:\"" + name + "\"\n" +
		"		_value type:#string ui:eValue default:\"" + (value as string) + "\"\n" +
		"	)\n" +
		"	rollout AlembicMetadataRLT1 \"Alembic Metadata " + (count as string) + "\"\n" +
		"	(\n" +
		"		edittext eName \"n\" fieldWidth:120 labelOnTop:false\n" +
		"		edittext eValue \"v\" fieldWidth:120 labelOnTop:false\n" +
		"	)\n" +
		")"

		AlembicMetadata = execute(evalstr);

		custattributes.add selectedObject.modifiers["Alembic Metadata"] AlembicMetadata baseobject:false

	)
)

function ImportMetadata selectedObject metadataArray = (

	if selectedObject != undefined then
	(
		size = metadataArray.count/2
		a = 1
		for i = 1 to size do 
		(
			name = metadataArray[a]
			a = a + 1
			value = metadataArray[a]
			a = a + 1
			CreateAlembicMetadataModifier selectedObject name value
		)

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
    		CreateAlembicMetadataModifier selection[i] "" ""
    	)
    )
)
