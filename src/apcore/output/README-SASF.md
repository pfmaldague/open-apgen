# SASF output

## WRITE SASF command
SASF output is initiated by a WRITE_SASFrequest object, which is created either from the GUI (File -> Export SASF) or from a script (WRITESASF command). The command specifies the following parameters:

1. The actual name of the output SASF file.
2. The start time of the SASF. An activity whose start time is earlier than this time will be omitted from the output.
3. The end time of the SASF. An activity whose start time is equal to or later than this time will be omitted from the output.
4. A list of symbolic file attributes. In order to be included in the output, an activity must have a "file" element defined in its SASF attribute, and the value of the file element must be one of the symbolic file attributes in the list.
5. An equal time inclusion flag. If this flag is 0, item 3 above works as indicated. If the flag is 1, activities whose start time is equal to the speficied end time will be included in the output.

## Criteria for an activity to be included in an SASF

The prerequisite for any activity to be included in SASF output is that it should have the SASF attribute defined. This attribute is an array containing a number of keyword-value pairs. The following keywords are recognized:

keyword | description
------- | -----------
activity_name | For activity steps, determines the name of the activity in the output.
command_name | For command steps, determines the name of the command in the output.
cyclic | Obsolete; do not use.
editgroup | Consult Users' Guide.
file | The symbolic file name that this activity belongs to. Symbolic files that should be included in the SASF output are listed in the WRITE_SASF command as indicated above.
genealogy | The genealogy to list in the SASF output.
group_name | Consult Users' Guide.
identifier | Consult Users' Guide.
key | Which seqgen legend the activity should appear on.
lower_label | label used on the seqgen display (may be obsolete).
parameterFormat | obsolete - do not use.
processor | The processor to list in the SASF output.
status | Consult Users' Guide.
step_label | Consult Users' Guide.
step_name | Consult Users' Guide.
TEXT | For note steps, describes the content of the note.
text | same as TEXT.
type | The type of step which this activity will be represented as in the SASF output. The following types are recognised: request, activity, command, note, notes. Type "note" creates one note step; type "notes" creates two. 
upper_label | Consult Users' Guide.
workgroup | Consult Users' Guide.

## Output algorithm (sketch)

For each symbolic file name, APGenX collects all activities in the plan whose SASF attribute contains a "file" attribute with that symbolic name as its value. It then arranges them in sequences. Each activity with an SASF file attribute equal to "request" will create a separate sequence in the SASF. Activities that are not requests are included in one of those sequences. To determine which sequence a given activity belons, APGenX looks at the ancestors of each activity. If one of these ancestors has type "request", the activity will be included in the sequence of that ancestor. If no ancestor qualifies, the activity is included in a default sequence that catches all the "floating" activities in the plan (i. e., the activities that do not have a qualified ancestor request.)
