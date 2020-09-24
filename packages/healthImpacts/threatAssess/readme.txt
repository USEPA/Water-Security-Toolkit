Threat Assessment
-----------------

Build Notes:

This project now requires the DCDFLib project for the probit dose-response
model.  This project is in the UC repository.  It provides a cumulative
distribution function necessary for the probit model.


Usage: teva_assess.exe <TAI Filename> [<SSS Filename>]

Output Files and formats:

TAO File
nnodes          (int) number of nodes
nt              (int) number of time steps
ns              (int) number of scenarios
S[1]�S[nt]          (float) Average susceptible population
I[1]�I[nt]          (float) Average infected population
D[1]�D[nt]          (float) Average diseased population
F[1]�F[nt]          (float) Average fatality
dos[1]�dos[nt]      (float) Average population dose
res[1]�res[nt]      (float) Average population dose-response
1 receptor_node_id[1] s[1] i[1] d[1] f[1] dos[1] res[1]
...
nn receptor_node_id[nn] s[nn] i[nn] d[nn] f[nn] dos[n] res[n]
Scenario Results
ID Fatalities Dosage Response
SourceNodeID[1] fatalities[1] dosage[1] response[1]
...
SourceNodeID[ns] fatalities[ns] dosage[ns] response[ns]

SAP file: contains Sensor analysis outputs
VarName[1]   varValue[1]
...
VarName[n]   varValue[n]



Input File format:

This can be copied and pasted into an editor.

; THREAT ASSESSMENT INPUT (TAI) FILE
; USAGE: teva-assess.exe <TAI filename>
; Data items explained below
; UPPERCASE items are non-modifiable keywords
; lowercase items are user-supplied values
; | indicates a selection
; ALL is all junction nodes
;----------------------------------------------------------------------
; INPUT-OUTPUT
; * TSONAME    - dirOrFilename:
;                 Name of threat simulator output (TSO) file if single file
;                 Name of directory if multi-part file
;              - patternIfDir - pattern to match tso file parts if
;                  using multi-part TSO file
;              - version - required only for versions 1 and 2
;                  newer versions contain this information in the header
;                1 if it is generated from the currentle installed TEVA
;                  distributed version (as of 3/9/2005)
;                2 if it is generated from Jim's latest code
;              - storage - required only for versions 1 and 2
;                0 Original version - all concentration data stored - one
;                  record for each time step
;                1 Jim's storage reduction scheme - essentially one record
;                  for each node that has at least one non-zero value.
;                2 Each row of data is further reduced in size by doing
;                  a modified run length encoding.
; * TAONAME    - Name of threat assessment output (TAO) file
; * SAPNAME    - Sensor Analysis Parameter output file (optional)
; * SAPVARS    - Variables to output to SAP file.  Currently supported values:
;                   MeanInfections
;                   MaxInfections
;                This is required if there is a SAP file specified
;                The SAP file and vars are only needed when running the
;                sensor analysis scripts
;----------------------------------------------------------------------
TSONAME       charstring
TAONAME       charstring
SAPNAME       charstring
SAPVARS       varname_1 ... varname_n
;----------------------------------------------------------------------
; DOSE-RESPONSE PARAMETERS
; * A - function coefficient
; * M - function coefficient
; * N - function coefficient
; * TAU - function coefficient
; * BODYMASS - exposed individual body mass (Kg)
; * NORMALIZE - Dose in mg/Kg ('yes') or mg ('no')
; * BETA - Beta value for probit dose response model
; * LD50 - LD50 or ID50 value for the agent being studied
; * TYPE - either PROBIT or OLD depending on the dose response equation
;          to be used.  If it is PROBIT, only the LD50 and BETA values
;          need to be specified, and if it is OLD, the A, M, N, and TAU
;          values need to be specified.  The BODYMASS and NORMALIZE
;          apply to both equations.
;----------------------------------------------------------------------
DR:A          value
DR:M          value
DR:N          value
DR:TAU        value
BODYMASS      value
NORMALIZE     YES|NO
DR:BETA       value
DR:LD50       value
DR:TYPE       probit | old

;----------------------------------------------------------------------
; DISEASE MODEL
; * LATENCYTIME - time from exposed to symptoms (hours)
; * FATALITYTIME - time from symptoms till death (hours)
; * FATALITYRATE - Fraction of exposed population that die
;----------------------------------------------------------------------
LATENCYTIME   value
FATALITYTIME  value
FATALITYRATE  value

;----------------------------------------------------------------------
; EXPOSURE MODEL
; * DOSETYPE - TOTAL = total ingested mass
; * INGESTIONTYPE - DEMAND = ingestion probability prop. to demand
;                   ATUS RANDOM = ATUS ingestion model, random volume
;                                 selection from volume curve
;                   ATUS MEAN   = ATUS ingestion model, mean volume of value
;                   FIXED5 RANDOM = 5 fixed ingestion times (7am, 9:30am,Noon,3PM,6PM),
;                                 random volume selection from volume curve
;                   FIXED5 MEAN = 5 fixed ingestion times (7am, 9:30am,Noon,3PM,6PM), mean volume of value
; * INGESTIONRATE - volumetric ingestion rate (liters/day) - used for demand and ATUS MEAN and FIXED5 MEAN
;----------------------------------------------------------------------
DOSETYPE      TOTAL
INGESTIONTYPE DEMAND
INGESTIONRATE value

;----------------------------------------------------------------------
; POPULATION MODEL
; * POPULATION FILE  - value is the filename that contains the node-based
;                population.  The format of the file is simply one line
;                per node with the node id and the popuylation value
;                for that node.
; * POPULATION DEMAND  - value is the per capita usage rate (flow units/person).
;                The population will be estimated by demand
;----------------------------------------------------------------------
POPULATION    FILE | DEMAND  value
;----------------------------------------------------------------------
; DOSE OVER THRESHOLD MODEL
; * DOSE_THRESHOLDS - The dose over each threshold specified will be 
;                     computed and output to the TAO file.
; * DOSE_BINDATA - Specifies the endpoints of bins to tally the number
;                  of people with a dose between the two endpoints.
;                  Values can be either does values or response values -
;                  Response values are converted to Dose values using the
;                  dose-response data specified in this file and are indicated
;                  on this line by the keyword RESPONSE.  Dose values are
;                  IDENTIFIED by the keyword DOSE
;----------------------------------------------------------------------
DOSE_THRESHOLDS value1 ... value_n
DOSE_BINDATA (DOSE | RESPONSE) value1 ... value_n
