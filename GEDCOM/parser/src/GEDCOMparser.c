#include "LinkedListAPI.h"
#include "GEDCOMparser.h"
#include "GEDCOMutilities.h"

// ****************************** A1 functions ******************************
GEDCOMerror createGEDCOM(char* fileName, GEDCOMobject** obj) {
    FILE* gedFile;
	ErrorCode errorType;
    GEDCOMerror gedErr; //error type declaration of type GEDCOMerror 
	if (fileName == NULL) { //If file is invalid
        errorType = INV_FILE;
        gedErr.type = errorType; //INV_FILE
        gedErr.line = -1;
        return gedErr;
    }
    gedFile = fopen(fileName, "r"); // open gedcom file
	if (gedFile == NULL) {
		errorType = INV_FILE;
        gedErr.type = errorType; //INV_FILE
        gedErr.line = -1;
        return gedErr;
	}
    //INV_FILE detection
    if (strstr(fileName, ".ged") == NULL) { //If file is invalid
        errorType = INV_FILE;
        gedErr.type = errorType; //INV_FILE
        gedErr.line = -1;
		fclose(gedFile); //close gedcom file
        return gedErr;
    }

    *obj = malloc(sizeof (GEDCOMobject)); //malloc allocation for object of type GEDCOMobject
    (*obj)->individuals = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals); //initialize list for individuals in 
    (*obj)->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	(*obj)->header = NULL;
	(*obj)->submitter = NULL;
    if (*obj == NULL) { //Check if obj malloc is NULL
        errorType = OTHER_ERROR;
        gedErr.type = errorType; //OTHER_ERROR
        fclose(gedFile); //close gedcom file
        return gedErr;
    }
	
	int maxLength;
    char* token; //for tokenizing strings
    int currentLine = 0; //counts the current line in the file being read
    char otherTag[255]; //Stores otherFields's tag of type Field
    char otherValue[255]; //Stores otherFields's value of type Field

    char* input = malloc(500 * sizeof (char)); //Used to store each line from file
    if (input == NULL) {
        errorType = OTHER_ERROR;
        gedErr.type = errorType; //OTHER_ERROR
        fclose(gedFile); //close gedcom file
        free(input);
        return gedErr;
    }

    char* version = malloc(4 * sizeof (char)); //malloc version for GEDCOM version
    if (version == NULL) {
        errorType = OTHER_ERROR;
        gedErr.type = errorType; //OTHER_ERROR
        fclose(gedFile); //close gedcom file
        free(input);
        free(version);
        return gedErr;
    }
    for (int i = 0; i < 4; i++) {
        version[i] = '\0';
    }

    Header* headObj = malloc(sizeof (Header)); //header object declaration
	headObj->submitter = NULL;
    if (headObj == NULL) { //Check if obj malloc is NULL
        errorType = OTHER_ERROR;
        gedErr.type = errorType; //OTHER_ERROR
        fclose(gedFile); //close gedcom file
        free(input);
        free(headObj);
        free(version);
        return gedErr;
    }
    //Submitter Variables
    Submitter* submObj;
    submTag submitterTag = sNONE;

    //Individual Variables
    char xref[22];
    char* parsedName;
    int lastnameIndex = 0;
    char lastName[120];
    char firstName[120];
    //int xrefIndex = 0;
    recType recordType = HEAD;
    indivTag individualTag = iNONE;
    Individual* individualObj;

    //Event Variables
    char eventType[5];
    char eventDate[35];
    char eventPlace[120];
    Event* eventObj;

    //Family Variables
    famTag familyTag = fNONE;
    Family* famObj;

    //Field Declaration
    headObj->otherFields = initializeList(&printField, &deleteField, &compareFields); //Initialize List
    Field* fieldObj;

    //Enums
    boolInvGedcom trlrFound = NO, inHeader = NO, inRecord = NO, headFound = NO; //inHeader/inRecord used to determine if currently reading in header information
    reqHeaderTags SOUR = EMPTY, SUBM = EMPTY, GEDC = EMPTY, gedVERS = EMPTY, gedFORM = EMPTY, CHAR = EMPTY; //SOUR, SUBM, GEDC, gedVERS, CHAR
    levelNumber level = NONE;
    init initHead = UNINITIALIZED;
    init initSubm = UNINITIALIZED;
    levelOneHeadTag lvloneHTAG = oNONE;
    levelTwoHeadTag lvltwoHTAG = tNONE;

    //GEDCOMutilities variables for hash table
    keyData hashTable[51];
    for (int i = 0; i < 51; i++) {
        hashTable[i].key = 0;
        hashTable[i].recordType = '\0'; 
		hashTable[i].ptrLength = 0;
		strcpy(hashTable[i].XREF, "");
		for (int j = 0; j < 5; j++) {
			hashTable[i].pointer[j] = NULL;
		}
    }
    int key; //Hash Key

    //Read in each line until EOF
    while (fgets(input, 500, gedFile) != NULL) {
		currentLine = currentLine + 1; //Line in file counter
		maxLength = strlen(input);
		if (maxLength > 255) { //Check if total length of GEDCOM line is over 255
			if (recordType == HEAD) {
				fclose(gedFile);
				free(input);
				free(version);
				free(headObj);
				gedErr.type = INV_HEADER;
				gedErr.line = currentLine;
				deleteGEDCOM(*obj);
				*obj = NULL;
				return gedErr;
			}
			else {
				fclose(gedFile);
				free(input);
				free(version);
				free(headObj);
				gedErr.type = INV_RECORD;
				gedErr.line = currentLine;
				deleteGEDCOM(*obj);
				*obj = NULL;
				return gedErr;
			}
		}
        /*###############################################################	LEVEL NUMBER   ###############################################################*/
        token = strtok(input, " \n\r\t"); //Get level number
        if ((token != NULL) && (token[0] != '\n') && (token[0] != '\r') && (strcmp(token, "\n\r") != 0) && (strcmp(token, "\r\n") != 0)) { //check if line is empty with "\r" "\n" "\r\n" "\n\r" line terminators
            if (token[0] == '0') { //new record. Level 0 indicates new record
                inHeader = NO; //reset, will be set to YES if found below
                inRecord = YES;
                level = ZERO;
                if (recordType == INDIVIDUAL) { //add individual to GEDCOMobj's individual list
                    if ((individualTag == evenDate) || (individualTag == evenOther) || (individualTag == evenPlace) || (individualTag == EVENT)) { //if the next line level is 0 after parsing event, add eventObj to list
                        insertBack(&(individualObj->events), (void*) (eventObj));
                    }
                    insertBack((&(*obj)->individuals), individualObj);
                    recordType = newRECORD;
                    individualTag = iNONE;
	
                    key = getKey(xref);
                    insert(key, 'I', (void*) (getFromBack((*obj)->individuals)), hashTable, xref); //Insert individual address into hash table
                } else if (recordType == SUBMITTER) { //add subm to GEDCOMObj
                    (*obj)->submitter = submObj;
                    initSubm = INITIALIZED;;
                    recordType = newRECORD;
                    submitterTag = sNONE;

                    key = getKey(xref);
                    insert(key, 'U', (void*) ((*obj)->submitter), hashTable, xref); //Insert submitter address into hash table
                } else if (recordType == FAMILY) {
                    if ((familyTag == fDATE) || (familyTag == fPLACE) || (familyTag == feventOTHER)) { //if the next line level is 0 after parsing event, add eventObj to list
                        insertBack(&(famObj->events), (void*) (eventObj));
                    }
                    insertBack((&(*obj)->families), famObj);
					
                    key = getKey(xref);
                    insert(key, 'F', (void*) (getFromBack((*obj)->families)), hashTable, xref); //insert Family address into hash table
                    recordType = newRECORD;
                    familyTag = fNONE;
                } else if (initHead == INITIALIZED) { //After every fam, indi record, reset to new record
                    recordType = newRECORD;
                }
            } else if (token[0] == '1') { //level 1
                level = ONE;
            } else if (token[0] == '2') { //level 2
				if (level == ZERO) { //Check for wrong level increment
					fclose(gedFile);
					free(input);
					free(version);
					free(headObj);
					gedErr.type = INV_RECORD;
					gedErr.line = currentLine;
					*obj = NULL;
					return gedErr;
				}
                level = TWO;
            } else { //any level higher than 2
				if ((level == ZERO) || (level == ONE)) {//Check for wrong level increment
					fclose(gedFile);
					free(input);
					free(version);
					if ((*obj)->header == NULL) {
						clearList(&(headObj->otherFields));
						free(headObj);
					}
					if (recordType == INDIVIDUAL) {
						deleteIndividual((void*)individualObj);
					}
					//deleteField((void*)fieldObj);
					gedErr.type = INV_RECORD;
					gedErr.line = currentLine;
					deleteGEDCOM(*obj);
					*obj = NULL;
					//printf("\n******************** ERROR FOUND *****************************\n");
					return gedErr;
				}
                level = NONE;
                //printf("ERROR: CHECK CODE\n");
            }

            if ((level == ZERO) && (headFound == YES) && (initHead == UNINITIALIZED)) { //Create the Header
                if ((SOUR == EMPTY) || (SUBM == EMPTY) || (GEDC == EMPTY) || (gedVERS == EMPTY) || (gedFORM == EMPTY) || (CHAR == EMPTY)) {
                    fclose(gedFile); //close gedcom file
                    free(input);
                    free(version);
					clearList(&(headObj->otherFields));
					free(headObj);
                    gedErr.type = INV_HEADER;
                    gedErr.line = currentLine - 1;
					deleteGEDCOM(*obj);
					*obj = NULL;
                    return gedErr;
                }
                (*obj)->header = headObj;
                initHead = INITIALIZED;
                recordType = newRECORD;
            }
            //printf("Level: *%s*\n", token);
            /*###############################################################	TAGS AND POINTERS ###############################################################*/
            token = strtok(NULL, " \n\r"); //get xref_ID (pointer) or tag
            if (token == NULL) { //if tag is missing ERROR CHECKING
                if ((level == ZERO) && (headFound == NO)) { //INV_GEDCOM if missing HEAD tag
                    //printf("TEST");
                    fclose(gedFile); //close gedcom file
                    free(input);
                    free(version);
                    free(headObj);
                    gedErr.type = INV_GEDCOM;
                    gedErr.line = -1;
					deleteGEDCOM(*obj);
					*obj = NULL;
                    return gedErr;
                } else if (inHeader == YES) { //INV_HEADER for missing tag
                    fclose(gedFile);
                    free(input);
                    free(version);
                    free(headObj);
                    gedErr.type = INV_HEADER;
                    gedErr.line = currentLine;
					deleteGEDCOM(*obj);
					*obj = NULL;
                    return gedErr;
                } else if ((inRecord == YES) && (level != ZERO)) { //INV_RECORD for missing tag
                    fclose(gedFile);
                    free(input);
                    free(version);
                    free(headObj);
                    gedErr.type = INV_RECORD;
                    gedErr.line = currentLine;
					deleteGEDCOM(*obj);
					*obj = NULL;
                    return gedErr;
                } else if ((level == ZERO) && (trlrFound == NO)) {
                    //printf("TEST");
                    fclose(gedFile); //close gedcom file
                    free(input);
                    free(version);
                    free(headObj);
                    gedErr.type = INV_GEDCOM;
                    gedErr.line = -1;
					deleteGEDCOM(*obj);
					*obj = NULL;
                    return gedErr;
                }
            } else { //if tag is not missing
                switch (level) {
                    case ZERO:
                        if ((individualTag == evenDate) || (individualTag == evenOther) || (individualTag == evenPlace) || (individualTag == EVENT)) { //submit event into individual event list
                            insertBack(&(individualObj->events), (void*) (eventObj));
                        }
                        if ((familyTag == fDATE) || (familyTag == fPLACE) || (familyTag == feventOTHER)) { //submit event into individual event list
                            insertBack(&(famObj->events), (void*) (eventObj));
                        }
                        if (strcmp(token, "HEAD") == 0) { //Head Found
                            headFound = YES;
                            inHeader = YES;
                        } else if (strcmp(token, "TRLR") == 0) { //TRLR Found
                            trlrFound = YES;
                            recordType = TRLR;
                        } else if (token[0] == '@' && (token[strlen(token) - 1] == '@')) { //Finding xref in TAG
                            strcpy(xref, token);
                        }
                        break;

                    case ONE:
                        if (recordType == HEAD) { //Check level 1 Header tags
                            if ((strcmp(token, "HEAD") == 0) && (level == ONE)) {// Header SOUR
								fclose(gedFile);
								free(input);
								free(version);
								free(headObj);
								gedErr.type = INV_HEADER;
								gedErr.line = currentLine;
								deleteGEDCOM(*obj);
								*obj = NULL;
								return gedErr;
                            } else if ((strcmp(token, "SOUR") == 0) && (level == ONE)) {// Header SOUR
                                lvloneHTAG = hSOUR;
                            } else if (strcmp(token, "SUBM") == 0) { // header SUBM
                                lvloneHTAG = hSUBM;
                            } else if (strcmp(token, "GEDC") == 0) { // header GEDC
                                lvloneHTAG = hGEDC;
                                GEDC = FOUND;
                            } else if (strcmp(token, "CHAR") == 0) { // header CHAR
                                lvloneHTAG = hCHAR;
                            } else {
                                lvloneHTAG = hOTHER;
                                strcpy(otherTag, token);
                            }
                        } else if (recordType == INDIVIDUAL) { //Check level 1 Individual tags
                            if (strcmp(token, "NAME") == 0) { //if individual NAME TAG
                                individualTag = NAME;
                            } else if (strcmp(token, "EVEN") == 0) { //if individual EVEN tag
                                individualTag = EVEN;
                            }//If tag is an individual's EVENT
                            else if ((strcmp(token, "BIRT") == 0) || (strcmp(token, "CHR") == 0) || (strcmp(token, "BURI") == 0) ||
                                    (strcmp(token, "CREM") == 0) || (strcmp(token, "DEAT") == 0) || (strcmp(token, "ADOP") == 0) || (strcmp(token, "BAPM") == 0) ||
                                    (strcmp(token, "BARM") == 0) || (strcmp(token, "BASM") == 0) || (strcmp(token, "BLES") == 0) || (strcmp(token, "CHRA") == 0) ||
                                    (strcmp(token, "CONF") == 0) || (strcmp(token, "FCOM") == 0) || (strcmp(token, "ORDN") == 0) || (strcmp(token, "NATU") == 0) ||
                                    (strcmp(token, "EMIG") == 0) || (strcmp(token, "IMMI") == 0) || (strcmp(token, "CENS") == 0) || (strcmp(token, "PROB") == 0) ||
                                    (strcmp(token, "WILL") == 0) || (strcmp(token, "GRAD") == 0) || (strcmp(token, "RETI") == 0)) {
                                if ((individualTag == evenDate) || (individualTag == evenOther) || (individualTag == evenPlace)) { //submit event into individual event list
                                    insertBack(&(individualObj->events), (void*) (eventObj));
                                }
                                individualTag = EVENT;
                                strcpy(eventType, token);
                            } else if (strcmp(token, "FAMS") == 0) { //if FAMS is found. level 1_tag_individual
                                if ((individualTag == evenDate) || (individualTag == evenOther) || (individualTag == evenPlace)) { //submit event into individual event list
                                    insertBack(&(individualObj->events), (void*) (eventObj));
                                }
                                individualTag = FAMS;
                            } else if (strcmp(token, "FAMC") == 0) {//if FAMC is found. level 1_tag_individual
                                if ((individualTag == evenDate) || (individualTag == evenOther) || (individualTag == evenPlace)) { //submit event into individual event list
                                    insertBack(&(individualObj->events), (void*) (eventObj));
                                }
                                individualTag = FAMC;
                            } else { //individual's Other tags go here. level 1_tag_individual
                                if ((individualTag == evenDate) || (individualTag == evenOther) || (individualTag == evenPlace)) { //submit event into individual event list
                                    insertBack(&(individualObj->events), (void*) (eventObj));
                                }
                                strcpy(otherTag, token);
                                individualTag = iOther;
                            }
                        } else if (recordType == SUBMITTER) { //Check level 1 Individual tags
                            if (strcmp(token, "NAME") == 0) { //if level 1 submitter tag is NAME
                                submitterTag = submNAME;
                            } else if (strcmp(token, "ADDR") == 0) { //if level 1 submitter tag is ADDR
                                submitterTag = submAddress;
                            } else { //if level 1 submitter tag belongs in otherFields
                                strcpy(otherTag, token);
                                submitterTag = sOther;
                            }
                        } else if (recordType == FAMILY) {//finds level 1 FAMILY tags
                            if ((familyTag == fDATE) || (familyTag == feventOTHER) || (familyTag == fPLACE)) { //submit event into individual event list
                                insertBack(&(famObj->events), (void*) (eventObj));
                            }
                            if (strcmp(token, "HUSB") == 0) {
                                familyTag = HUSB;
                            } else if (strcmp(token, "WIFE") == 0) {
                                familyTag = WIFE;
                            } else if (strcmp(token, "CHIL") == 0) {
                                familyTag = CHIL;
                            } else if ((strcmp(token, "ANUL") == 0) || (strcmp(token, "CENS") == 0) || (strcmp(token, "DIV") == 0) || (strcmp(token, "DIVF") == 0) ||
                                    (strcmp(token, "ENGA") == 0) || (strcmp(token, "MARR") == 0) || (strcmp(token, "MARB") == 0) || (strcmp(token, "MARL") == 0) ||
                                    (strcmp(token, "MARS") == 0) || (strcmp(token, "EVEN") == 0) || (strcmp(token, "MARC") == 0)) {
                                familyTag = fEVENT;
                                strcpy(eventType, token);
                            } else { //Family's Other tags go here. level 1_tag_individual
                                strcpy(otherTag, token);
                                familyTag = fOTHER;
                            }
                        }
                        break;

                    case TWO:
                        if (recordType == HEAD) { //Check level 2 Header tags. HEADER ONLY
							if (strcmp(token, "HEAD") == 0) {// Header SOUR
								fclose(gedFile);
								free(input);
								free(version);
								free(headObj);
								gedErr.type = INV_HEADER;
								gedErr.line = currentLine;
								deleteGEDCOM(*obj);
								*obj = NULL;
								return gedErr;
                            } else if ((strcmp(token, "VERS") == 0) && (lvloneHTAG == hGEDC)) { // header GEDCvers
                                lvltwoHTAG = hGEDCvers;
                            } else if ((strcmp(token, "FORM") == 0) && (lvloneHTAG == hGEDC)) { // header GEDCform
                                lvltwoHTAG = hGEDCform;
                                strcpy(otherTag, token);
                            } else {
                                lvltwoHTAG = tOther;
                                strcpy(otherTag, token);
                            }
                        } else if (recordType == INDIVIDUAL) { //Check level 2 Individual tags. This checks EVENT data
                            if ((individualTag == EVENT) || (individualTag == evenDate) || (individualTag == evenPlace) || (individualTag == evenOther) || (individualTag == GIVN) || (individualTag == SURN) || (individualTag == NAME)) { //if recently checked tag is part of EVENT
                                if (strcmp(token, "DATE") == 0) { //find INDIVIDUAL's EVENT DATE level 2 tag
                                    individualTag = evenDate;
                                } else if (strcmp(token, "PLAC") == 0) { //find INDIVIDUAL's EVENT PLACE level 2 tag
                                    individualTag = evenPlace;
                                } else if (strcmp(token, "GIVN") == 0) {
                                    individualTag = GIVN;
                                } else if (strcmp(token, "SURN") == 0) {
                                    individualTag = SURN;
                                } else { //find any other INDIVIDUAL's EVENT level 2 tag here
                                    strcpy(otherTag, token);
                                    individualTag = evenOther;
                                }
                            }
                        } else if (recordType == SUBMITTER) { //Check level 2 submitter tags
                            if ((submitterTag == submAddress) && (strcmp(token, "CONT") == 0)) {
                                submitterTag = addrCont;
                            } else {

                            }
                        } else if (recordType == FAMILY) { //finds level 2 FAMILY tags
                            if ((familyTag == fEVENT) || (familyTag == fDATE) || (familyTag == fPLACE) || (familyTag == fOTHER)) { //if recently checked tag is part of EVENT
                                if (strcmp(token, "DATE") == 0) { //find FAMILY's EVENT DATE level 2 tag
                                    familyTag = fDATE;
                                } else if (strcmp(token, "PLAC") == 0) { //find FAMILY's EVENT PLACE level 2 tag
                                    familyTag = fPLACE;
                                } else { //find any other FAMILY's EVENT level 2 tag here
                                    strcpy(otherTag, token);
                                    familyTag = feventOTHER;
                                }
                            }
                        }
                        break;

                    case THREE:
                        break;

                    case FOUR:
                        break;

                    case NONE:
                        break;
                }
				
                /*############################################################### LINE VALUE ###############################################################*/
                token = strtok(NULL, "\n\r\0"); //get line value (if there is any)

                if (token != NULL) {//check if there is a line value
                    if (inHeader == YES) { //if currently handling header
                        if (level == ONE) {
                            switch (lvloneHTAG) { //Level one Header Tags
                                case hSOUR: //header source
                                    strcpy(headObj->source, token);
                                    SOUR = FOUND; //REQUIRED HEADER TAG
                                    break;

                                case hSUBM: //header submitter
                                    SUBM = FOUND; //REQUIRED HEADER TAG
                                    break;

                                case hGEDC: //header GEDC
                                    //do nothing because GEDC has no line value. Just a tag.
                                    break;

                                case hCHAR: //header GEDC
                                    headObj->encoding = getCharSet(token);
                                    CHAR = FOUND; //REQUIRED HEADER TAG
                                    break;
                                case hOTHER:
                                    break;
                                case oNONE:
                                    break;
                                    /*default: // Other Field
                                        strcpy(otherValue, token);
                                        fieldObj = malloc(sizeof (Field)); //create field object
                                        fieldObj->tag = malloc(255 * sizeof (char));
                                        fieldObj->value = malloc(255 * sizeof (char));
                                        strcpy(fieldObj->tag, otherTag);
                                        strcpy(fieldObj->value, otherValue);
                                        insertBack(&(headObj->otherFields), (void*) (fieldObj)); //insert Node*/
                                    //printf("lvl 1_Other Tag: %s\n", otherTag);
                                    //printf("lvl 1_Other Value: %s\n", otherValue); 
                            }
                        } else if (level == TWO) {
                            switch (lvltwoHTAG) { // Level two header Tags
                                case hGEDCvers: //header GEDC version
                                    strncat(version, token, 3);
                                    headObj->gedcVersion = atof(version);
                                    gedVERS = FOUND; //REQUIRED HEADER TAG
                                    break;

                                case hGEDCform: //header GEDC form
                                    strcpy(otherValue, token);
                                    fieldObj = malloc(sizeof (Field)); //create field object
                                    fieldObj->tag = malloc(255 * sizeof (char));
                                    fieldObj->value = malloc(255 * sizeof (char));
                                    strcpy(fieldObj->tag, otherTag);
                                    strcpy(fieldObj->value, otherValue);
                                    insertBack(&(headObj->otherFields), (void*) (fieldObj)); //insert Node
                                    gedFORM = FOUND; //REQUIRED HEADER TAG
                                    break;

                                default: //Level 2 Other Field
                                    strcpy(otherValue, token);
                                    fieldObj = malloc(sizeof (Field)); //create field object
                                    fieldObj->tag = malloc(255 * sizeof (char));
                                    fieldObj->value = malloc(255 * sizeof (char));
                                    strcpy(fieldObj->tag, otherTag);
                                    strcpy(fieldObj->value, otherValue);
                                    insertBack(&(headObj->otherFields), (void*) (fieldObj)); //insert Node
                            }
                        }
                    } else if (recordType == newRECORD) { //Check new record type tag
                        if (strcmp(token, "INDI") == 0) { //If found SUBM, new record is type INDIVIDUAL
                            recordType = INDIVIDUAL;
                            individualObj = malloc(sizeof (Individual)); //*****NEED MALLOC NULL CHECKER******
                            individualObj->families = initializeList(&printFamily, &deleteDummy, &compareFamilies);
                            individualObj->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
                            individualObj->otherFields = initializeList(&printField, &deleteField, &compareFields);
                            individualObj->givenName = malloc(120 * sizeof (char));
                            individualObj->surname = malloc(120 * sizeof (char));
                        } else if (strcmp(token, "SUBM") == 0) { //If found SUBM, new record is type SUBMITTER
                            recordType = SUBMITTER;
                            submObj = malloc(sizeof (Submitter) + 255); //submitter object delcaration
                            strcpy(submObj->address, "");
                            submObj->otherFields = initializeList(&printField, &deleteField, &compareFields);
                            if (submObj == NULL) { //Check if obj malloc is NULL
                                errorType = OTHER_ERROR;
                                gedErr.type = errorType; //OTHER_ERROR
                                fclose(gedFile); //close gedcom file
                                free(input);
                                free(headObj);
                                free(version);
                                free(submObj);
                                return gedErr;
                            }
                            //printf("Submitter_Pointer: %p\n\n", &submObj);
                        } else if (strcmp(token, "FAM") == 0) { //If found FAM, new record is type FAMILY
                            recordType = FAMILY;
                            famObj = malloc(sizeof(Family));
							famObj->wife = NULL;
							famObj->husband = NULL;
                            famObj->children = initializeList(&printIndividual, &deleteDummy, &compareIndividuals);
                            famObj->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
                            famObj->otherFields = initializeList(&printField, &deleteField, &compareFields);
                        }
                    } else if (recordType == INDIVIDUAL) { //switch statement for INDIVIDUAL's TAGS to perform actions for the line value
                        switch (individualTag) {
                            case NAME: //INDIVIDUAL's name tag and value are found
                                parsedName = strtok(token, " \n\r");
                                resetString(lastName, 120);
                                while (parsedName != NULL) {
                                    if ((parsedName[0] == '/') && (parsedName[strlen(parsedName) - 1] == '/')) { //Check for last name
                                        for (int i = 0; i < strlen(parsedName); i++) {
                                            if (parsedName[i] != '/') {
                                                lastName[lastnameIndex] = parsedName[i];
                                                lastnameIndex++;
                                            }
                                        }
                                        lastName[lastnameIndex] = '\0';
                                    } else { //First name
                                        strcpy(firstName, parsedName);
                                    }
                                    parsedName = strtok(NULL, " \n\r");
                                }
                                strcpy(individualObj->givenName, firstName);
                                strcpy(individualObj->surname, lastName);
                                parsedName = '\0';
								strcpy(firstName, "");
								strcpy(lastName, "");
                                lastnameIndex = 0;
                                break;
                            case GIVN:
                                strcpy(individualObj->givenName, token);
                                break;
                            case SURN:
                                strcpy(individualObj->surname, token);
                                break;
                            case FAMS: //INDIVIDUAL's FAMS tag and pointer are found
                                break;
                            case FAMC: //INDIVIDUAL's FAMC tag and pointer are found
                                break;
                            case evenOther: //INDIVIDUAL's EVENT OTHERFIELD's tag and value are found
                                strcpy(otherValue, token);
                                fieldObj = malloc(sizeof (Field)); //create field object
                                fieldObj->tag = malloc(255 * sizeof (char));
                                fieldObj->value = malloc(255 * sizeof (char));
                                strcpy(fieldObj->tag, otherTag);
                                strcpy(fieldObj->value, otherValue);
                                insertBack(&(eventObj->otherFields), (void*) (fieldObj));
                                break;

                            case evenDate: //INDIVIDUAL's EVENT DATE tag and value are found
                                strcpy(eventDate, token);
                                strcpy(eventObj->date, eventDate);
                                break;

                            case evenPlace: //INDIVIDUAL's EVENT PLACE tag and value are found
                                strcpy(eventPlace, token);
                                strcpy(eventObj->place, eventPlace);
                                break;

                            case EVENT: //may need to change/delete because its in the lower else statement
                                //Check section where line value is missing
                                break;

                            case EVEN: //may need to change/delete
                                break;

                            case iOther: //INDIVIDUAL's OTHERFIELD tag and value. field node created and input into list
                                strcpy(otherValue, token);
                                fieldObj = malloc(sizeof (Field)); //create field object
                                fieldObj->tag = malloc(255 * sizeof (char));
                                fieldObj->value = malloc(255 * sizeof (char));
                                strcpy(fieldObj->tag, otherTag);
                                strcpy(fieldObj->value, otherValue);
                                insertBack(&(individualObj->otherFields), (void*) (fieldObj));
                                break;

                            case iNONE:
                                //printf("iNONE\n");
                                break;
                        }
                    } else if (recordType == SUBMITTER) { //SUBMITTER LINE VALUES
                        switch (submitterTag) {
                            case submNAME:
                                parsedName = strtok(token, " \n\r");
                                resetString(lastName, 120);
                                while (parsedName != NULL) {
                                    if ((parsedName[0] == '/') && (parsedName[strlen(parsedName) - 1] == '/')) { //Check for last name
                                        for (int i = 0; i < strlen(parsedName); i++) {
                                            if (parsedName[i] != '/') {
                                                lastName[lastnameIndex] = parsedName[i];
                                                lastnameIndex++;
                                            }
                                        }
                                        lastName[lastnameIndex] = '\0';
                                    } else { //First name
                                        strcpy(firstName, parsedName);
                                    }
                                    parsedName = strtok(NULL, " \n\r");
                                }
                                strcpy(submObj->submitterName, firstName);
                                if (lastName[0] != '\0') {
                                    strcat(submObj->submitterName, " ");
                                    strcat(submObj->submitterName, lastName);
                                }
                                parsedName = '\0';
                                lastnameIndex = 0;
                                break;
                            case submAddress:
                                strcpy(submObj->address, token);
                                break;
                            case addrCont:
                                strcat(submObj->address, "\n");
                                strcat(submObj->address, token);
                                break;
                            case sOther:
                                strcpy(otherValue, token);
                                fieldObj = malloc(sizeof (Field)); //create field object
                                fieldObj->tag = malloc(255 * sizeof (char));
                                fieldObj->value = malloc(255 * sizeof (char));
                                strcpy(fieldObj->tag, otherTag);
                                strcpy(fieldObj->value, otherValue);
                                insertBack(&(submObj->otherFields), (void*) (fieldObj));
                                break;
                            case sNONE:
                                break;

                        }
                    } else if (recordType == FAMILY) { //FAMILY LINE VALUES
                        switch (familyTag) {
                            case HUSB:
                                //printf("FAMILY_HUSB POINTER_Line:%d_%s\n\n", currentLine, token);
                                break;
                            case WIFE:
                                //printf("FAMILY_WIFE POINTER_Line:%d_ %s\n\n", currentLine, token);
                                break;
                            case CHIL:
                                //printf("FAMILY_CHIL POINTER_Line:%d_%s\n\n", currentLine, token);
                                break;
                            case feventOTHER:
                                strcpy(otherValue, token);
                                fieldObj = malloc(sizeof (Field)); //create field object
                                fieldObj->tag = malloc(255 * sizeof (char));
                                fieldObj->value = malloc(255 * sizeof (char));
                                strcpy(fieldObj->tag, otherTag);
                                strcpy(fieldObj->value, otherValue);
                                insertBack(&(eventObj->otherFields), (void*) (fieldObj));
                                break;
                            case fOTHER:
                                strcpy(otherValue, token);
                                fieldObj = malloc(sizeof (Field)); //create field object
                                fieldObj->tag = malloc(255 * sizeof (char));
                                fieldObj->value = malloc(255 * sizeof (char));
                                strcpy(fieldObj->tag, otherTag);
                                strcpy(fieldObj->value, otherValue);
                                insertBack(&(famObj->otherFields), (void*) (fieldObj));
                                break;
                            case fNONE:
                                break;
                            case fEVENT: //Located near the bottom
                                break;
                            case fDATE:
                                strcpy(eventDate, token);
                                strcpy(eventObj->date, eventDate);
                                break;
                            case fPLACE:
                                strcpy(eventPlace, token);
                                strcpy(eventObj->place, eventPlace);
                                break;
                        }
                    }
                } else { //If there is NO LINE VALUE, go here
                    if ((level == ONE) && (initHead != INITIALIZED)) { //if level is '1', check all required level 1 header tags
                        switch (lvloneHTAG) {
                            case hSOUR:
                                fclose(gedFile); //close gedcom file
                                free(input);
                                free(version);
                                free(headObj);
                                gedErr.type = INV_HEADER;
                                gedErr.line = currentLine;
                                return gedErr;
                                break;

                            case hSUBM:
                                fclose(gedFile); //close gedcom file
                                free(input);
                                free(version);
                                free(headObj);
                                gedErr.type = INV_HEADER;
                                gedErr.line = currentLine;
                                return gedErr;
                                break;

                            case hGEDC:
                                //Do Nothing Because it doesnt require a line value
                                break;

                            case hCHAR:
                                fclose(gedFile); //close gedcom file
                                free(input);
                                free(version);
                                free(headObj);
                                gedErr.type = INV_HEADER;
                                gedErr.line = currentLine;
                                return gedErr;
                                break;
                            case oNONE:
                                break;
                            case hOTHER:
                                break;
                        }
                    } else if ((level == TWO) && (initHead != INITIALIZED)) { //if level is '2', check all required level 2 header tags
                        switch (lvltwoHTAG) {
                            case hGEDCvers:
                                fclose(gedFile); //close gedcom file
                                free(input);
                                free(version);
                                free(headObj);
                                gedErr.type = INV_HEADER;
                                gedErr.line = currentLine;
                                return gedErr;
                                break;

                            case hGEDCform:
                                fclose(gedFile); //close gedcom file
                                free(input);
                                free(version);
                                free(headObj);
                                gedErr.type = INV_HEADER;
                                gedErr.line = currentLine;
                                return gedErr;
                                break;
                            case tOther:
                                break;
                            case tNONE:
                                break;
                        }
                    } else {
                        if (individualTag == EVENT) { //A INDIVIDUAL'S EVENT TAG IS FOUND
                            eventObj = malloc(sizeof (Event));
                            eventObj->otherFields = initializeList(&printField, &deleteField, &compareFields);
                            eventObj->date = malloc(35 * sizeof (char));
                            eventObj->place = malloc(120 * sizeof (char));
                            strcpy(eventObj->type, eventType);
							strcpy(eventObj->date, "");
							strcpy(eventObj->place, "");
                        } else if (familyTag == fEVENT) { //A FAMILY'S EVENT TAG IS FOUND
                            //printf("FAMILY EVENT TAG FOUND_Line:%d_%s\n", currentLine, eventType);
                            eventObj = malloc(sizeof (Event));
                            eventObj->otherFields = initializeList(&printField, &deleteField, &compareFields);
                            eventObj->date = malloc(35 * sizeof (char));
                            eventObj->place = malloc(120 * sizeof (char));
                            strcpy(eventObj->type, eventType);
							strcpy(eventObj->date, "");
							strcpy(eventObj->place, "");
                        } else {
                            //printf("POSSIBLE ERROR, ADD PRINT ERROR HERE: %d\n", currentLine);
                        }

                        //Do Nothing For Now
                    }
                }
            }
        }
        token = "\0"; //reset token
    }
    fclose(gedFile); //close gedcom file
    if ((headFound == NO) || (trlrFound == NO) || (initSubm == UNINITIALIZED)) { //NEED TO INCLUDE SUBMITTER LATER
        free(input);
        free(version);
		if (headFound == NO) {
			free(headObj);
		}
		if (trlrFound == NO)  {
			clearList(&(submObj->otherFields));
			free(submObj);
		}
        gedErr.type = INV_GEDCOM;
        gedErr.line = -1;
		deleteGEDCOM(*obj);
		*obj = NULL;
        return gedErr;
    } else {
        /***********************************SECOND PASS FOR POINTERS******************************************/
        gedFile = fopen(fileName, "r"); // open gedcom file for reading again
        currentLine = 0;
        void* leftsidePointer;
        void* rightsidePointer;
        Family* tempFam;
        Individual* tempIndi;
        char hashType;
        char tempString[255];
        secondPass hashTag = NONEt; //FAMSt, FAMCt, HUSBt, WIFEt, CHILt, SUBMt, NONEt
        while (fgets(input, 255, gedFile) != NULL) {
            currentLine += 1;
			
            //Line Number, dont need here
            strcpy(tempString, input);
            token = strtok(input, " \n\r\t");

            //Line Tag/Pointer 
            token = strtok(NULL, " \n\r");
            if (token != NULL) {
                if (token[0] == '@') { //LEFT SIDE POINTER (INDI, FAM)
                    key = getKey(token);
                    if (strstr(tempString, "INDI")) {
                        hashType = 'I';
                    } else if (strstr(tempString, "FAM")) {
                        hashType = 'F';
                    } else if (strstr(tempString, "SUBM")) {
                        hashType = 'U';
                    }
                    leftsidePointer = search(key, hashType, hashTable, token);
                } else if (strcmp("FAMS", token) == 0) {
                    hashTag = FAMSt;
                } else if (strcmp("FAMC", token) == 0) {
                    hashTag = FAMCt;
                } else if (strcmp("SUBM", token) == 0) {
                    hashTag = SUBMt;
                } else if (strcmp("HUSB", token) == 0) {
                    hashTag = HUSBt;
                } else if (strcmp("WIFE", token) == 0) {
                    hashTag = WIFEt;
                } else if (strcmp("CHIL", token) == 0) {
                    hashTag = CHILt;
                }
            }

            //Line Value
            token = strtok(NULL, "\n\r\0");
            if ((token != NULL) && (token[0] == '@')) { //RIGHT SIDE POINTER (FAMS, FAMC, HUSB, WIFE, CHIL, SUBM)

                switch (hashTag) {
                    case FAMSt:
                        key = getKey(token);
                        rightsidePointer = search(key, 'F', hashTable, token);
                        tempIndi = (Individual*)leftsidePointer;
                        insertBack(&(tempIndi->families), (Family*)rightsidePointer);
                        break;
                    case FAMCt:
                        key = getKey(token);
                        rightsidePointer = search(key, 'F', hashTable, token);
                        tempIndi = (Individual*)leftsidePointer;
                        insertBack(&(tempIndi->families), (Family*)rightsidePointer);
                        break;
                    case HUSBt:
                        key = getKey(token);
                        rightsidePointer = search(key, 'I', hashTable, token);
                        tempFam = (Family*)leftsidePointer;
                        tempFam->husband = (Individual*)rightsidePointer;
                        break;
                    case WIFEt:
                        key = getKey(token);
                        rightsidePointer = search(key, 'I', hashTable, token);
                        tempFam = (Family*)leftsidePointer;
                        tempFam->wife = (Individual*)rightsidePointer;
                        break;
                    case CHILt:
                        key = getKey(token);
                        rightsidePointer = search(key, 'I', hashTable, token);
                        tempFam = (Family*)leftsidePointer;
                        insertBack(&(tempFam->children), (Individual*)rightsidePointer);
                        break;
                    case SUBMt:
                        key = getKey(token);
                        rightsidePointer = search(key, 'U', hashTable, token);
                        (*obj)->header->submitter = (Submitter*)rightsidePointer;
                        break;
                    case NONEt:
                        break;

                }
            }
            token = "\0"; //reset token
        }
    }
    fclose(gedFile);
    free(input);
    free(version);
    gedErr.type = OK;
    gedErr.line = -1;
    return gedErr; //if successful
}

Individual* findPerson(const GEDCOMobject* familyRecord, bool(*compare)(const void* first, const void* second), const void* person) {
    Individual* pers;
	if ((familyRecord == NULL) || (*compare == NULL) || (person == NULL)) { //CHECK IF PARAMS ARE NULL
		pers = NULL;
		return pers;
	}
	Node* indiNode = familyRecord->individuals.head;
	while (indiNode != NULL) {
		pers = (Individual*)(indiNode->data);
		if (compare(pers,person)) {
			return pers;
		}
		indiNode = indiNode->next;
	}
	pers = NULL;
    return pers;
}

List getDescendants(const GEDCOMobject* familyRecord, const Individual* person) {
    List returnList =  initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);
	int countDescendants = 0;
	char returnEmpty = 'N';
	Individual* checkIndiv = NULL;
	ListIterator checkList;
    if ((familyRecord == NULL)||(person == NULL)) {
		return returnList;
	}
	
	findDescendants(&returnList, person);
	
	checkList = createIterator(returnList); 
	while ((checkIndiv = nextElement(&checkList)) != NULL) { 
		countDescendants += 1;
		if ((strcmp(checkIndiv->givenName, person->givenName) == 0) && (strcmp(checkIndiv->surname, person->surname) == 0)) { //checks if the person(assuming they are the parent) is in the descendants list
			returnEmpty = 'Y';
		}
	}
	if ((countDescendants == 1) && (returnEmpty == 'Y')) {
		clearList(&returnList);
		returnList =  initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);
	}
    return returnList;
}

char* printGEDCOM(const GEDCOMobject* obj) {
    char* str = malloc(255 * sizeof (char));
    char* temp;
    int newLen = 255;
    char floatString[10];
    char enumString[10];
    if (obj == NULL) {
        strcpy(str, "Obj is NULL");
        return str;
    }
    switch (obj->header->encoding) {
        case ANSEL:
            strcpy(enumString, "ANSEL");
            break;
        case UTF8:
            strcpy(enumString, "UTF8");
            break;
        case UNICODE:
            strcpy(enumString, "UNICODE");
            break;
        case ASCII:
            strcpy(enumString, "ASCII");
            break;
    }

    strcpy(str, "                 Individual's List                 \n");
    strcat(str, "-------------------------------------------------------");
    temp = toString(obj->individuals);
    newLen = strlen(str) + 50 + strlen(temp);
    str = (char*) realloc(str, newLen);
    strcat(str, temp);
    strcat(str, "\n");
    free(temp);

    newLen = strlen(str) + 300;
    str = (char*) realloc(str, newLen);
    strcat(str, "                   Families List                   \n");
    strcat(str, "-------------------------------------------------------");
    temp = toString(obj->families);
    newLen = strlen(str) + 50 + strlen(temp);
    str = (char*) realloc(str, newLen);
    strcat(str, temp);
    strcat(str, "\n\n");
    free(temp);

    newLen = strlen(str) + 300;
    str = (char*) realloc(str, newLen);
    strcat(str, "                 Submitter Information                 \n");
    strcat(str, "-------------------------------------------------------\n");
    strcat(str, "Submitter Name: ");
    strcat(str, obj->submitter->submitterName);
    strcat(str, "\nSubmitter Address: ");
    strcat(str, obj->submitter->address);
    strcat(str, "\n");
    temp = toString(obj->submitter->otherFields);
    newLen = strlen(str) + 50 + strlen(temp);
    str = (char*) realloc(str, newLen);
    strcat(str, temp);
    strcat(str, "\n\n");
    free(temp);

    newLen = strlen(str) + 300;
    str = (char*) realloc(str, newLen);
    strcat(str, "                  Header Information                  \n");
    strcat(str, "-------------------------------------------------------\n");
    strcat(str, "Source Name: ");
    strcat(str, obj->header->source);
    strcat(str, "\ngedcVersion: ");
    snprintf(floatString, sizeof (str), "%.1f", obj->header->gedcVersion);
    strcat(str, floatString);
    strcat(str, "\nEncodiing: ");
    strcat(str, enumString);
    strcat(str, "\nSubmitter Name: ");
    strcat(str, obj->submitter->submitterName);
    temp = toString(obj->header->otherFields);
    newLen = strlen(str) + 50 + strlen(temp);
    str = (char*) realloc(str, newLen);
    strcat(str, temp);
    free(temp);

    return str;
}

void deleteGEDCOM(GEDCOMobject* obj) {
	//ListIterator iter;
	//void* field = NULL;
	//printf("************** %d *************", getLength(obj->header->otherFields));
    if (obj != NULL) {
        if (obj->header != NULL) {
            if (getFromFront(obj->header->otherFields) != NULL) {
                clearList(&(obj->header->otherFields));
            }
            if (obj->header->submitter != NULL) {
				if (getFromFront(obj->submitter->otherFields) != NULL) {
					clearList(&(obj->submitter->otherFields));
				}
                free(obj->header->submitter);
            }
            free(obj->header);
        }
        if (getFromFront(obj->individuals) != NULL) {
            clearList(&(obj->individuals));
        }

        if (getFromFront(obj->families) != NULL) {
            clearList(&(obj->families));
        }
        free(obj);
    } 
}

char* printError(GEDCOMerror err) { //DONE print error function
    char* errString = malloc(255 * (sizeof (char)));
    char* line = malloc(5 * sizeof (char));
    sprintf(line, "%d", err.line);
    switch (err.type) {
        case OK:
            strcpy(errString, "------Error Result------\n");
            strcat(errString, "Error Type: OK\n");
            strcat(errString, "Line Value: ");
            strcat(errString, line);
            strcat(errString, "\n");
            break;
        case INV_FILE:
            strcpy(errString, "------Error Result------\n");
            strcat(errString, "Error Type: Invalid File\n");
            strcat(errString, "Line Value: ");
            strcat(errString, line);
            strcat(errString, "\n");
            break;
        case INV_GEDCOM:
            strcpy(errString, "------Error Result------\n");
            strcat(errString, "Error Type: Invalid GEDCOM\n");
            strcat(errString, "Line Value: ");
            strcat(errString, line);
            strcat(errString, "\n");
            break;
        case INV_HEADER:
            strcpy(errString, "------Error Result------\n");
            strcat(errString, "Error Type: Invalid Header\n");
            strcat(errString, "Line Value: ");
            strcat(errString, line);
            strcat(errString, "\n");
            break;
        case INV_RECORD:
            strcpy(errString, "------Error Result------\n");
            strcat(errString, "Error Type: Invalid Record\n");
            strcat(errString, "Line Value: ");
            strcat(errString, line);
            strcat(errString, "\n");
            break;
        case OTHER_ERROR:
            strcpy(errString, "------Error Result------\n");
            strcat(errString, "Error Type: Other Error\n");
            strcat(errString, "Line Value: ");
            strcat(errString, line);
            strcat(errString, "\n");
            break;
		case WRITE_ERROR:
			strcpy(errString, "------Write GEDCOM Error Result------\n");
            strcat(errString, "Error Type: WRITE_ERROR\n");
            strcat(errString, "Line Value: ");
            strcat(errString, line);
            strcat(errString, "\n");
    }
    free(line);
    return errString;
}

void deleteField(void* toBeDeleted) {
    Field* deleteField = (Field*) (toBeDeleted);
    free(deleteField->tag);
    free(deleteField->value);
    free(deleteField);
}

int compareFields(const void* first, const void* second) {
    Field* firstField = (Field*) first;
    Field* secondField = (Field*) second;
    char firstF[255];
    char secondF[255];

    strcpy(firstF, firstField->tag);
    strcat(firstF, " ");
    strcat(firstF, firstField->value);

    strcpy(secondF, secondField->tag);
    strcat(secondF, " ");
    strcat(secondF, secondField->value);

    for (int i = 0; i < strlen(firstF); i++) {
        if (firstF[i] >= 'A' && firstF[i] <= 'Z') {
            firstF[i] = firstF[i] + 32;
        }
    }
    for (int i = 0; i < strlen(secondF); i++) {
        if (secondF[i] >= 'A' && secondF[i] <= 'Z') {
            secondF[i] = secondF[i] + 32;
        }
    }
    return strcmp(firstF, secondF);
}

char* printField(void* toBePrinted) { //DONE print field function
    Field* temp = malloc(sizeof (Field));
    temp->tag = malloc(255 * sizeof (char));
    temp->value = malloc(255 * sizeof (char));

    strcpy(temp->tag, ((Field*) toBePrinted)->tag);
    strcpy(temp->value, ((Field*) toBePrinted)->value);

    char* str = malloc(255 * sizeof (char));

    strcpy(str, temp->tag);
    strcat(str, " = ");
    strcat(str, temp->value);

    free(temp->tag);
    free(temp->value);
    free(temp);

    return str;
}

void deleteIndividual(void* toBeDeleted) {
    Individual* deleteIndi = (Individual*) toBeDeleted;
    clearList(&(deleteIndi->otherFields));
    clearList(&(deleteIndi->events));
    clearList(&(deleteIndi->families));
    free(deleteIndi->givenName);
    free(deleteIndi->surname);
    free(deleteIndi);
}

int compareIndividuals(const void* first, const void* second) {
    Individual* firstIndi = (Individual*) first;
    Individual* secondIndi = (Individual*) second;
    char firstName[255];
    char secondName[255];

    strcpy(firstName, firstIndi->surname);
    strcat(firstName, ",");
    strcat(firstName, firstIndi->givenName);

    strcpy(secondName, secondIndi->surname);
    strcat(secondName, ",");
    strcat(secondName, secondIndi->givenName);

    for (int i = 0; i < strlen(firstName); i++) {
        if (firstName[i] >= 'A' && firstName[i] <= 'Z') {
            firstName[i] = firstName[i] + 32;
        }
    }
    for (int i = 0; i < strlen(secondName); i++) {
        if (secondName[i] >= 'A' && secondName[i] <= 'Z') {
            secondName[i] = secondName[i] + 32;
        }
    }
    return strcmp(firstName, secondName);
}

char* printIndividual(void* toBePrinted) { //FIX LATER
    Individual* temp = malloc(sizeof (Individual));
    Individual* temp2 = (Individual*) toBePrinted;
	int fLength = 0;
	int tLength = 0;
	int strLength = 0;
    temp->givenName = malloc(120 * sizeof (char));
    temp->surname = malloc(120 * sizeof (char));
    if (((Individual*) toBePrinted)->givenName != NULL) {
        strcpy(temp->givenName, ((Individual*) toBePrinted)->givenName);
    }
    if (((Individual*) toBePrinted)->surname != NULL) {
        strcpy(temp->surname, ((Individual*) toBePrinted)->surname);
    }
    char* str = malloc(255 * sizeof (char));
	str[0] = '\0';
	
    char* fieldStr = toString(temp2->otherFields);
	fLength = strlen(fieldStr);
	strLength = strlen(str);
    int newLen = strLength + 500 + fLength;
    str = (char*) realloc(str, newLen);

    char* tempStr = toString(temp2->events);
	tLength = strlen(tempStr);
	strLength = strlen(str);
    newLen = strLength + 500 + tLength;
    str = (char*) realloc(str, newLen);

    strcpy(str, "name: ");
    strcat(str, temp->givenName);
    strcat(str, " ");
    strcat(str, temp->surname);
    strcat(str, fieldStr);
    strcat(str, tempStr);
    strcat(str, "\n");

    free(fieldStr);
    free(tempStr);
    free(temp->givenName);
    free(temp->surname);
    free(temp);

    return str;
}

void deleteEvent(void* toBeDeleted) {
    if (toBeDeleted != NULL) {
        Event* deleteEvent = (Event*) (toBeDeleted);
        free(deleteEvent->date);
        free(deleteEvent->place);
        clearList(&(deleteEvent->otherFields));
        free(deleteEvent);
    }
}

int compareEvents(const void* first, const void* second) {
    Event* firstEvent = (Event*) first;
    Event* secondEvent = (Event*) second;
    char firstE[255];
    char secondE[255];

    if ((firstEvent->date != NULL) || (secondEvent->date != NULL)) {
        strcpy(firstE, firstEvent->date);
        strcpy(secondE, secondEvent->date);
    } else {
        strcpy(firstE, firstEvent->type);
        strcpy(secondE, secondEvent->type);
    }

    for (int i = 0; i < strlen(firstE); i++) {
        if (firstE[i] >= 'A' && firstE[i] <= 'Z') {
            firstE[i] = firstE[i] + 32;
        }
    }
    for (int i = 0; i < strlen(secondE); i++) {
        if (secondE[i] >= 'A' && secondE[i] <= 'Z') {
            secondE[i] = secondE[i] + 32;
        }
    }
    return strcmp(firstE, secondE);
}

char* printEvent(void* toBePrinted) {
    Event* temp = malloc(sizeof (Event));
    temp->date = malloc(35 * sizeof (char));
    temp->place = malloc(120 * sizeof (char));
    Event* temp2 = (Event*) toBePrinted;
    strcpy(temp->type, ((Event*) toBePrinted)->type);
    strcpy(temp->date, ((Event*) toBePrinted)->date);
    strcpy(temp->place, ((Event*) toBePrinted)->place);

    char* str = malloc(255 * sizeof (char));
    char* tempStr;
    tempStr = toString(temp2->otherFields);
    strcpy(str, "Type: ");
    strcat(str, temp->type);
    strcat(str, "\nDate: ");
    strcat(str, temp->date);
    strcat(str, "\nPlace: ");
    strcat(str, temp->place);
    strcat(str, tempStr);

    free(tempStr);
    free(temp->place);
    free(temp->date);
    free(temp);
    return str;
}

void deleteFamily(void* toBeDeleted) {
    if (toBeDeleted != NULL) {
        Family* deleteFam = (Family*) toBeDeleted;
        clearList(&(deleteFam->children));
        clearList(&(deleteFam->events));
        clearList(&(deleteFam->otherFields));
        free(deleteFam);
    }
}

int compareFamilies(const void* first, const void* second) {
    Family* firstFam = (Family*) first;
    Family* secondFam = (Family*) second;
    int firstCount = 0;
    int secondCount = 0;

    if (firstFam->wife != NULL) {
        firstCount += 1;
    }
    if (firstFam->husband != NULL) {
        firstCount += 1;
    }

    if (secondFam->wife != NULL) {
        secondCount += 1;
    }
    if (secondFam->husband != NULL) {
        secondCount += 1;
    }

    ListIterator firstIter = createIterator(firstFam->children);
    ListIterator secondIter = createIterator(secondFam->children);

    while ((nextElement(&firstIter)) != NULL) {
        firstCount += 1;
    }
    while ((nextElement(&secondIter)) != NULL) {
        secondCount += 1;
    }

    if (firstCount == secondCount) {
        return 0;
    } else if (firstCount > secondCount) {
        return 1;
    } else if (firstCount < secondCount) {
        return -1;
    }
    return 0;
}

char* printFamily(void* toBePrinted) {
    Family* temp2 = (Family*) toBePrinted;
    char* str = malloc(255 * sizeof (char));
    int newLen = 255;
    char* tempStr;
    char* fieldStr;
    strcpy(str, "*-*-*-*-*-*-*-*Start of Family*-*-*-*-*-*-*-*\n");
    if (temp2->husband != NULL) {
        strcat(str, "Husband's ");
        tempStr = printIndividual(temp2->husband);
        newLen = strlen(str) + 100 + strlen(tempStr);
        str = (char*) realloc(str, newLen);
        strcat(str, tempStr);
        free(tempStr);
    }
    strcat(str, "\n");
    if (temp2->wife != NULL) {
        strcat(str, "Wife's ");
        tempStr = printIndividual(temp2->wife);
        newLen = strlen(str) + 100 + strlen(tempStr);
        str = (char*) realloc(str, newLen);
        strcat(str, tempStr);
        free(tempStr);
    }

    if (temp2->otherFields.head != NULL) {
        fieldStr = toString(temp2->otherFields);
        newLen = strlen(str) + 100 + strlen(fieldStr);
        str = (char*) realloc(str, newLen);
        strcat(str, fieldStr);
        free(fieldStr);
    }
    if (temp2->events.head != NULL) {
        tempStr = toString(temp2->events);
        newLen = strlen(str) + 100 + strlen(tempStr);
        str = (char*) realloc(str, newLen);
        strcat(str, tempStr);
        strcat(str, "\n");
        free(tempStr);
    }
    if (temp2->children.head != NULL) {
        strcat(str, "\n   Children\n****************");
        tempStr = toString(temp2->children);
        newLen = strlen(str) + 5100 + strlen(tempStr);
        str = (char*) realloc(str, newLen);
        strcat(str, tempStr);
        free(tempStr);
        strcat(str, "****************");
    }
    strcat(str, "\n*-*-*-*-*-*-*-*End of Family*-*-*-*-*-*-*-*\n");
    return str;
}

// ****************************** A2 functions ******************************
GEDCOMerror writeGEDCOM(char* fileName, const GEDCOMobject* obj) {
	if (obj == NULL) {
		return returnErr(WRITE_ERROR, -1);
	}
	
	char floatString[10];
	FILE* writeFile;
	
	//variables used for INDIVIDUALS
	char stringXref[3];
	int indivIndex = 0;
	int indivListLength = getLength(obj->individuals); //size of gedcom individual's list
	int indivOtherLength;
	addressArray indiAddress[indivListLength];
	ListIterator iter;
	ListIterator iterOther;
	ListIterator iterFamily;
	
	//variables used for EVENTS
	int indivEventLength; //size of individual's event list
	ListIterator iterEvent;
	Event* event;
	
	//otherFields
	Field* oField;
	
	//for address iterator
	void* address;
	
	//variables used for FAMILIES
	int famListLength = getLength(obj->families);
	int famIndex = 0;
	int famEventLength;
	ListIterator iterChildren;
	addressArray famAddress[famListLength];
	Family* familyElem;
	Individual* childElem;
	
    writeFile = fopen(fileName, "w+"); //open file for writing 
	if ((writeFile == NULL) || (fileName == NULL) || (strstr(fileName, ".ged") == NULL)) { //CHECK FOR WRITE_ERROR
		return returnErr(WRITE_ERROR, -1);
	}
	
	iterFamily = createIterator(obj->families);
	while ((address = nextElement(&iterFamily)) != NULL) { //Iterate through obj family and store address into famAddress array
		famAddress[famIndex++].pointer = address;
	}
	
	//OBJ HEADER
	fprintf(writeFile, "0 HEAD\n");
	if (obj->header->source[0] != '\0') { //header source
		fprintf(writeFile, "1 SOUR ");
		fprintf(writeFile, obj->header->source);
		fprintf(writeFile, "\n");
	}
	else {
		return returnErr(WRITE_ERROR, -1);
	}
	
	if (obj->header->gedcVersion != 0) { //header gedc version
		fprintf(writeFile, "1 GEDC\n");
		fprintf(writeFile, "2 VERS ");
		snprintf(floatString, 10, "%.1f", obj->header->gedcVersion);
		fprintf(writeFile, floatString);
		fprintf(writeFile, "\n");
		fprintf(writeFile, "2 FORM LINEAGE-LINKED\n");
	}
	else {
		return returnErr(WRITE_ERROR, -1);
	}
	fprintf(writeFile, "1 CHAR "); //header encoding
	fprintf(writeFile, charSetString(obj->header->encoding)); //charSetString converts encoding enum to a String
	fprintf(writeFile, "1 SUBM @U1@\n"); //header subm reference
	
	//SUBMITTER RECORD
	fprintf(writeFile, "0 @U1@ SUBM\n"); 
	fprintf(writeFile, "1 NAME ");
	fprintf(writeFile, obj->submitter->submitterName);
	fprintf(writeFile, "\n");
	
	if (obj->submitter->address[0] != '\0') { //if submitter address exists, print it
		fprintf(writeFile, "1 ADDR ");
		fprintf(writeFile, obj->submitter->address);
		fprintf(writeFile, "\n");
	}
	
	//INDIVIDUAL RECORDS
	if (indivListLength > 0) {
		iter = createIterator(obj->individuals);
		while ((address = nextElement(&iter)) != NULL) { //grab the addresses of each individual and store them into indiAddress[]
			indiAddress[indivIndex++].pointer = address; //store Individual's address
			
			//Write Individual's XREF pointer, given name and surname
			fprintf(writeFile, "0 @I");
			snprintf(stringXref, 3, "%d", indivIndex); //convert index int to string
			fprintf(writeFile, stringXref);
			fprintf(writeFile, "@ INDI\n");
			fprintf(writeFile, "1 NAME ");
			fprintf(writeFile, ((Individual*)address)->givenName);
			fprintf(writeFile, " /");
			fprintf(writeFile, ((Individual*)address)->surname);
			fprintf(writeFile, "/\n");
			
			//Printing Individual's otherFields
			indivOtherLength = getLength(((Individual*)address)->otherFields);
			if (indivOtherLength > 0) { //if individual's otherFields list is not empty
				iterOther = createIterator(((Individual*)address)->otherFields);
				while ((oField = nextElement(&iterOther)) != NULL) { //Traverse through individual's otherFields list
					if ((strcmp(oField->tag, "GIVN") == 0) || (strcmp(oField->tag, "SURN") == 0)) { //if GIVN or SURN is found (The only level 2 tags we want)
						fprintf(writeFile, "2 ");
						fprintf(writeFile, oField->tag);
						fprintf(writeFile, " ");
						fprintf(writeFile, oField->value);
						fprintf(writeFile, "\n");
					}
					else if ((strcmp(oField->tag, "RESN") == 0) || (strcmp(oField->tag, "SEX") == 0) || (strcmp(oField->tag, "RFN") == 0) || (strcmp(oField->tag, "AFN") == 0) || (strcmp(oField->tag, "RIN") == 0)) {
						fprintf(writeFile, "1 ");
						fprintf(writeFile, oField->tag);
						fprintf(writeFile, " ");
						fprintf(writeFile, oField->value);
						fprintf(writeFile, "\n");
					}
				}
			}
			//Printing Individual's events
			indivEventLength = getLength(((Individual*)address)->events);
			if (indivEventLength > 0) {
				iterEvent = createIterator(((Individual*)address)->events);
				while ((event = nextElement(&iterEvent)) != NULL) { //iterate through individual's event list
					fprintf(writeFile, "1 ");
					fprintf(writeFile, event->type); 
					fprintf(writeFile, "\n");
					if (event->date[0] != '\0') {
						fprintf(writeFile, "2 DATE ");
						fprintf(writeFile, event->date); 
						fprintf(writeFile, "\n");
					}
					if (event->place[0] != '\0') {
						fprintf(writeFile, "2 PLAC ");
						fprintf(writeFile, event->place); 
						fprintf(writeFile, "\n");						
					}
				}
			}
			//printing Individual's FAMC or FAMS pointers
			if (famListLength > 0) { //Individual has families (family list is not empty)
				iterFamily = createIterator(((Individual*)address)->families);
				while ((familyElem = nextElement(&iterFamily)) != NULL) { //Iterate through individual's families list
					if ((familyElem->husband == ((Individual*)address)) || (familyElem->wife == ((Individual*)address))) { //if individual is the father or the mother
						fprintf(writeFile, "1 FAMS @F");
						for (int i = 0; i < famListLength; i++) {
							if (famAddress[i].pointer == familyElem) { //find matching family in addressFamily array to get the correct XREF pointer
								snprintf(stringXref, 3, "%d", (i+1));
								fprintf(writeFile, stringXref);
								fprintf(writeFile, "@\n");
							}
						}
					}
					else { //if individual is a child in a family
						fprintf(writeFile, "1 FAMC @F");
						for (int i = 0; i < famListLength; i++) {
							if (famAddress[i].pointer == familyElem) {
								snprintf(stringXref, 3, "%d", (i+1));
								fprintf(writeFile, stringXref);
								fprintf(writeFile, "@\n");
							}
						}
					}
				}
			}
		}
	}
	else {
		//printf("*NO INDIVIDUALS*\n");
	}
	//FAMILY RECORDS
	if (famListLength > 0) {
		iterFamily = createIterator(obj->families);
		while ((familyElem = nextElement(&iterFamily)) != NULL) { //iterate through GEDCOM obj family
			for (int i = 0; i < famListLength; i++) {	//iterate through famAddress array
				if (famAddress[i].pointer == familyElem) { //if family address in GEDCOM obj matches address in famAddress array
					fprintf(writeFile, "0 @F");
					snprintf(stringXref, 3, "%d", (i+1));
					fprintf(writeFile, stringXref);
					fprintf(writeFile, "@ FAM\n");
					famEventLength = getLength(familyElem->events);
					if (famEventLength > 0) { //if individual's event list is not empty
						iterEvent = createIterator(familyElem->events);
						while ((event = nextElement(&iterEvent)) != NULL) { //iterate through individual's event list
							fprintf(writeFile, "1 ");
							fprintf(writeFile, event->type); 
							fprintf(writeFile, "\n");
							if (event->date[0] != '\0') {
								fprintf(writeFile, "2 DATE ");
								fprintf(writeFile, event->date); 
								fprintf(writeFile, "\n");
							}
							if (event->place[0] != '\0') {
								fprintf(writeFile, "2 PLAC ");
								fprintf(writeFile, event->place); 
								fprintf(writeFile, "\n");						
							}
						}
					}
					for (int k = 0; k < indivListLength; k++) { //loop through individual's address list
						if (indiAddress[k].pointer == familyElem->husband) { //if husband is found, print 1 HUSB and its XREF pointer
							fprintf(writeFile, "1 HUSB @I");
							snprintf(stringXref, 3, "%d", (k+1));
							fprintf(writeFile, stringXref);
							fprintf(writeFile, "@\n");
						}
						else if (indiAddress[k].pointer == familyElem->wife) { //if wife is found, print 1 WIFE and its XREF pointer
							fprintf(writeFile, "1 WIFE @I");
							snprintf(stringXref, 3, "%d", (k+1));
							fprintf(writeFile, stringXref);
							fprintf(writeFile, "@\n");
						}
					}
				}
			}
			if (getLength(familyElem->children) > 0) { //if family has children
					iterChildren = createIterator(familyElem->children);
					while ((childElem = nextElement(&iterChildren)) != NULL) { //iterate through family's childrens list
						for (int c = 0; c < indivListLength; c++) {
							if (indiAddress[c].pointer == childElem) { //if child is found, print 1 CHIL and its XREF pointer
								fprintf(writeFile, "1 CHIL @I");
								snprintf(stringXref, 3, "%d", (c+1));
								fprintf(writeFile, stringXref);
								fprintf(writeFile, "@\n");
							}
						}
					}
			}
		}
	}
	
	//TRLR
	fprintf(writeFile, "0 TRLR\n"); //TRLR end of gedcom
	
	fclose(writeFile);
	return returnErr(OK, -1);
}

ErrorCode validateGEDCOM(const GEDCOMobject* obj) {
	ListIterator iter; 
	ListIterator iterEvent; 
	ListIterator iterOther;
	ListIterator iterFam;
	ListIterator iterChild;
	
	Individual* indivPtr;
	Individual* childPtr;
	Family* famPtr;
	Field* otherField;
	Event* eventPtr;
	
	int maxLength = 200;
	int checkLength = 0;
	
	int ilistLength = 0;
	int flistLength = 0;
	
	/*INV_GEDCOM*/
	if ((obj == NULL) || (obj->header == NULL) || (obj->submitter == NULL)) { //INV_GEDCOM. object header, object submitter, or obj->header->submitter is NULL
		return INV_GEDCOM;
	}
	else if (obj->header->submitter == NULL) {
		return INV_HEADER;
	}
	iter = createIterator(obj->header->otherFields);
	while ((otherField = nextElement(&iter)) != NULL) { //check if obj->header->otherFields->tag/value is NULL or empty
		if ((otherField->tag == NULL) || (otherField->value == NULL) || (otherField->tag[0] == '\0') || (otherField->value[0] == '\0')) {
			return INV_GEDCOM;
		}
		else {
			checkLength = strlen(otherField->tag); 
			if (checkLength > maxLength) { //check header otherField->tag length is over 200
				return INV_GEDCOM;
			}
			
			checkLength = strlen(otherField->value);
			if (checkLength > maxLength) { //check header otherField->value length is over 200
				return INV_GEDCOM;
			}
		}
	}
	
	iter = createIterator(obj->header->submitter->otherFields);
	while ((otherField = nextElement(&iter)) != NULL) { //check if obj->header->submitter->otherFields->tag/value is NULL or empty
		if ((otherField->tag == NULL) || (otherField->value == NULL) || (otherField->tag[0] == '\0') || (otherField->value[0] == '\0')) {
			return INV_GEDCOM;
		}
		else {
			checkLength = strlen(otherField->tag); 
			if (checkLength > maxLength) { //check submitter otherField->tag length is over 200
				return INV_GEDCOM;
			}
			
			checkLength = strlen(otherField->value);
			if (checkLength > maxLength) { //check submitter otherField->value length is over 200
				return INV_GEDCOM;
			}
		}
	}
	
	/*INV_HEADER*/
	if ((obj->header->source[0] == '\0') || (obj->header->submitter == NULL)) { //check if header required fields are empty
		return INV_HEADER;
	}
	
	/*INV_RECORDS*/
	if (obj->submitter->submitterName[0] == '\0') { //check if submitter name is empty
		return INV_RECORD;
	}
	
	iter = createIterator(obj->individuals);
	ilistLength = getLength(obj->individuals);
	for (int i = 0; i < ilistLength; i++) { //iterate through gedcom object's individuals list.
		indivPtr = nextElement(&iter);
		if (indivPtr == NULL) { //check if pointer to individual is NULL
			return INV_RECORD;
		}
		
		iterFam = createIterator(indivPtr->families);
		flistLength = getLength(indivPtr->families);
		for (int j = 0; j < flistLength; j++) {
			famPtr = nextElement(&iterFam);
			if (famPtr == NULL) {
				return INV_RECORD;
			}
		}
		
		checkLength = strlen(indivPtr->givenName);
		if (checkLength > maxLength) { //check if Individual->givenName length is less than 200
			return INV_RECORD;
		}
		
		checkLength = strlen(indivPtr->surname);
		if (checkLength > maxLength) { //check if Individual->surname length is less than 200
			return INV_RECORD;
		}
		iterEvent = createIterator(indivPtr->events);
		while ((eventPtr = nextElement(&iterEvent)) != NULL) { //iterate through individual's events list
			if (eventPtr->type[0] == '\0') { //check if event type is empty
				return INV_RECORD;
			}
			
			checkLength = strlen(eventPtr->date);
			if (checkLength > maxLength) { //check if Individual's event->date length is less than 200
				return INV_RECORD;
			}
			
			checkLength = strlen(eventPtr->place);
			if (checkLength > maxLength) { //check if Individual's event->place length is less than 200
				return INV_RECORD;
			}
			iterOther = createIterator(eventPtr->otherFields);
			while ((otherField = nextElement(&iterOther)) != NULL) { //iterate through individual->events->otherFields list. check tag and value
				checkLength = strlen(otherField->tag); 
				if (checkLength > maxLength) { //check individual->events->otherFields->tag length is over 200
					return INV_RECORD;
				}
				checkLength = strlen(otherField->value);
				if (checkLength > maxLength) { //check individual->events->otherFields->value length is over 200
					return INV_RECORD;
				}
			}
		}
		iterOther = createIterator(indivPtr->otherFields);
		while ((otherField = nextElement(&iterOther)) != NULL) { //iterate through individual->otherFields list. check tag and value
			checkLength = strlen(otherField->tag); 
			if (checkLength > maxLength) { //check individual->events->otherFields->tag length is over 200
				return INV_RECORD;
			}
			checkLength = strlen(otherField->value);
			if (checkLength > maxLength) { //check individual->events->otherFields->value length is over 200
				return INV_RECORD;
			}
		}
	}
	
	iter = createIterator(obj->families);
	flistLength = getLength(obj->families);
	for (int i = 0; i < flistLength; i++) { //iterate through gedcom object's family list.
		famPtr = nextElement(&iter); 
		if (famPtr == NULL) { //check if pointer to family is NULL
			return INV_RECORD;
		}

		iterChild = createIterator(famPtr->children);
		ilistLength = getLength(famPtr->children);
		for (int j = 0; j < ilistLength; j++) {
			childPtr = nextElement(&iterChild);
			if (childPtr == NULL) {
				return INV_RECORD;
			}
		}
		
		iterEvent = createIterator(famPtr->events);
		while ((eventPtr = nextElement(&iterEvent)) != NULL) { //iterate through Family's events list
			if (eventPtr->type[0] == '\0') { //check if event type is empty
				return INV_RECORD;
			}
			
			checkLength = strlen(eventPtr->date);
			if (checkLength > maxLength) { //check if Family's event->date length is less than 200
				return INV_RECORD;
			}
			
			checkLength = strlen(eventPtr->place);
			if (checkLength > maxLength) { //check if Family's event->place length is less than 200
				return INV_RECORD;
			}
			iterOther = createIterator(eventPtr->otherFields);
			while ((otherField = nextElement(&iterOther)) != NULL) { //iterate through Family->events->otherFields list. check tag and value
				checkLength = strlen(otherField->tag); 
				if (checkLength > maxLength) { //check Family->events->otherFields->tag length is over 200
					return INV_RECORD;
				}
				checkLength = strlen(otherField->value);
				if (checkLength > maxLength) { //check Family->events->otherFields->value length is over 200
					return INV_RECORD;
				}
			}
		}
		iterOther = createIterator(famPtr->otherFields);
		while ((otherField = nextElement(&iterOther)) != NULL) { //iterate through Family->otherFields list. check tag and value
			checkLength = strlen(otherField->tag); 
			if (checkLength > maxLength) { //check Family->events->otherFields->tag length is over 200
				return INV_RECORD;
			}
			checkLength = strlen(otherField->value);
			if (checkLength > maxLength) { //check Family->events->otherFields->value length is over 200
				return INV_RECORD;
			}
		}
	}
	return OK; //all is good
}

List getDescendantListN(const GEDCOMobject* familyRecord, const Individual* person, unsigned int maxGen) {
	List returnList =  initializeList(&printGeneration, &deleteGeneration, &compareGenerations);
	List** generationList;
	ListIterator iter;
	Individual* ptr;
	char dontAdd = 'N';
	int curGen = 0;
	if ((familyRecord == NULL) || (person == NULL)) { //Check if parameters are invalid. Return empty list if any parameters are invalid
		return returnList;
	}
	
	if (maxGen == 0) { //if maxGen = 0, return all descendants
		generationList = malloc(20*sizeof(List));
		if (generationList == NULL) {
			return returnList;
		}
		for (int i = 0; i < 20; i++) {
			generationList[i] = initializeDynamicList(&printIndividual, &deleteIndividual, &compareIndividuals);
		}
		helperAllDescendantsList(&returnList, generationList, person, curGen);
		for (int i = 0; i < 20; i++) {
			if (getLength(*generationList[i]) != 0) {
				insertBack(&returnList, (void*)(generationList[i]));
			}
			else {
				clearDynamicList(generationList[i]);
			}
		}
	}
	
	else if (maxGen >= 1) {
		generationList = malloc(maxGen*sizeof(List));
		if (generationList == NULL) {
			return returnList;
		}
		for (int i = 0; i < maxGen; i++) {
			generationList[i] = initializeDynamicList(&printIndividual, &deleteIndividual, &compareIndividuals);
		}
		helperDescendantListNFix(&returnList, generationList, person, maxGen, curGen);
		for (int i = 0; i < maxGen; i++) {
			iter = createIterator(*generationList[i]);
			while ((ptr = nextElement(&iter)) != NULL) {
				if (compareIndividuals(ptr, person) == 0) {
					dontAdd = 'Y';
				}
			}
			if ((getLength(*generationList[i]) != 0) && (dontAdd == 'N')) {
				insertBack(&returnList, (void*)(generationList[i]));
			}
			else {
				clearDynamicList(generationList[i]);
			}
		}
	}
	free(generationList);
	return returnList;
}

List getAncestorListN(const GEDCOMobject* familyRecord, const Individual* person, int maxGen) {
	List ancestorList =  initializeList(&printGeneration, &deleteGeneration, &compareGenerations);
	List** parentList;
	Individual* ptr;
	ListIterator iter;
	char dontAdd = 'N';
	int curGen = 0;
	if ((familyRecord == NULL) || (person == NULL)) { //Check if parameters are invalid. Return empty list if any parameters are invalid
		return ancestorList;
	}
	
	addressArray checkedPeople[100];
	int cpIndex = 0;
	
	if (maxGen == 0) { //if maxGen = 0, return all descendants
		parentList = malloc(20*sizeof(List));
		if (parentList == NULL) {
			return ancestorList;
		}
		for (int i = 0; i < 20; i++) {
			parentList[i] = initializeDynamicList(&printIndividual, &deleteIndividual, &compareIndividuals);
		}
		helperAllAncestorListN(&ancestorList, parentList, person, curGen);
		for (int i = 0; i < 20; i++) {
			if (getLength(*parentList[i]) != 0) {
				insertBack(&ancestorList, (void*)(parentList[i]));
			}
			else {
				clearDynamicList(parentList[i]);
			}
		}
	}
	
	else if (maxGen >= 1) {
		parentList = malloc(maxGen*sizeof(List));
		if (parentList == NULL) {
			return ancestorList;
		}
		for (int i = 0; i < maxGen; i++) {
			parentList[i] = initializeDynamicList(&printIndividual, &deleteIndividual, &compareIndividuals);
		}
		helperGetAncestorListNFix(&ancestorList, parentList, person, maxGen, curGen, checkedPeople, &cpIndex);
		//printf("cpIndex: %d\n", cpIndex);
		//printf("Test:%d\n", getLength(**parentList));
		for (int i = 0; i < maxGen; i++) {	
			iter = createIterator(*parentList[i]);
			while ((ptr = nextElement(&iter)) != NULL) {
				if (compareIndividuals(ptr, person) == 0) {
					dontAdd = 'Y';
				}
			}
			if ((getLength(*parentList[i]) != 0) && (dontAdd == 'N')) {
				insertBack(&ancestorList, (void*)(parentList[i]));
			}
			else {
				clearDynamicList(parentList[i]);
			}
			dontAdd = 'N';
		}
	}
	free(parentList);
	return ancestorList;
}

void deleteGeneration(void* toBeDeleted) {
	List* listPtr = (List*)toBeDeleted;
	ListIterator iter = createIterator(*listPtr);
	List* genPtr;
	while ((genPtr = nextElement(&iter)) != NULL) {
		clearDynamicList(genPtr);
	}
}

int compareGenerations(const void* first,const void* second) {
	List* firstList = (List*) first; //descendant list
	ListIterator iterFirst = createIterator(*firstList);
	Individual* firstPerson;
	int firstLength = getLength(*firstList);
	
    List* secondList = (List*) second; //new generation list
	ListIterator iterSecond = createIterator(*secondList);
	Individual* secondPerson;
	int secondLength = getLength(*secondList);
	
	if (firstLength != secondLength) {
		return 1;
	}
	
	while (((firstPerson = nextElement(&iterFirst)) != NULL) && ((secondPerson = nextElement(&iterSecond)) != NULL)) {
		if ((strcmp(firstPerson->givenName, secondPerson->givenName) != 0) && (strcmp(firstPerson->surname, secondPerson->surname) != 0)) {
			return 1;
		}
	}
	return 0;
}
char* printGeneration(void* toBePrinted) {
	ListIterator iter;
	List* listPtr = (List*)toBePrinted;
	char* temp;
	char* str = malloc(255*sizeof(char));
	int newLen = 255;
	List* genPtr;
	iter = createIterator(*listPtr);
	while ((genPtr = nextElement(&iter)) != NULL) {
		temp = toString(*genPtr);
		newLen = strlen(str) + 500 + strlen(temp);
		str = (char*) realloc(str, newLen);
		strcpy(str, temp);
		free(temp);
	}
	return str;
}

char* indToJSON(const Individual* ind) { 
	char* stringJSON = malloc(255*sizeof(char));
	if (ind == NULL)
	{
		strcpy(stringJSON, "");
		return stringJSON;
	}
	
	strcpy(stringJSON, "{\"givenName\":\"");
	if (ind->givenName[0] != '\0') {
		strcat(stringJSON, ind->givenName);
		strcat(stringJSON, "\",");
	}
	else {
		strcat(stringJSON, "\",");
	}
	strcat(stringJSON, "\"surname\":\"");
	if (ind->surname[0] != '\0') {
		strcat(stringJSON, ind->surname);
		strcat(stringJSON, "\"");
	}
	else {
		strcat(stringJSON, "\"");
	}
	strcat(stringJSON, "}");
	return stringJSON;
}

Individual* JSONtoInd(const char* str) { 
	Individual* ind = NULL;
	if ((str == NULL) || (str[0] == '\0') || (str[0] != '{')) {
		return NULL;
	}
	int boolean = 0; //1 for givenName, 2 for surname
	int inQuotes = 0; //every +2 is end of a quote (2,4,6,etc)
	int giveNameIndex = 0;
	int surnameIndex = 0;
	char givenName[255];
	char surname[255];
	givenName[0] = '\0';
	surname[0] = '\0';
	
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == ':') {
			boolean += 1;
		}
		else if ((boolean == 1) && (inQuotes < 2)) {
			if (str[i] != '\"') {
				givenName[giveNameIndex] = str[i];
				giveNameIndex += 1;
				givenName[giveNameIndex] = '\0';
			}
			else {
				inQuotes += 1;
			}
		}
		else if ((boolean == 2) && (inQuotes < 4)) {
			if (str[i] != '\"') {
				surname[surnameIndex] = str[i];
				surnameIndex += 1;
				surname[surnameIndex] = '\0';
			}
			else {
				inQuotes += 1;
			}
		}
	}
	ind = malloc(sizeof (Individual));
	ind->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	ind->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
	ind->otherFields = initializeList(&printField, &deleteField, &compareFields);
	ind->givenName = malloc(120 * sizeof (char));
	ind->surname = malloc(120 * sizeof (char));
	strcpy(ind->givenName, givenName);
	strcpy(ind->surname, surname);
	return ind;
}

GEDCOMobject* JSONtoGEDCOM(const char* str) {
	GEDCOMobject* obj = NULL;
	
	int boolean = 0; //1 for source, 2 for gedcVersion, 3 for encoding, 4 for subName, 5 for subAddress
	int inQuotes = 0; //every +2 is end of a quote (2,4,6,etc)
	
	int sourceIndex = 0;
	int gedcVersionIndex = 0;
	int encodingIndex = 0;
	int subNameIndex = 0;
	int subAddressIndex = 0;
	
	char source[255];
	char gedcVersion[255];
	char* encoding = malloc(255*sizeof(char));
	char subName[255];
	char subAddress[255];
	
	if (str == NULL)
	{
		return NULL;
	}
	
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == ':') {
			boolean += 1;
		}
		else if ((boolean == 1) && (inQuotes < 2)) {
			if (str[i] != '\"') {
				source[sourceIndex] = str[i];
				sourceIndex += 1;
				source[sourceIndex] = '\0';
			}
			else {
				inQuotes += 1;
			}
		}
		else if ((boolean == 2) && (inQuotes < 4)) {
			if (str[i] != '\"') {
				gedcVersion[gedcVersionIndex] = str[i];
				gedcVersionIndex += 1;
				gedcVersion[gedcVersionIndex] = '\0';
			}
			else {
				inQuotes += 1;
			}
		}
		else if ((boolean == 3) && (inQuotes < 6)) {
			if (str[i] != '\"') {
				encoding[encodingIndex] = str[i];
				encodingIndex += 1;
				encoding[encodingIndex] = '\0';
			}
			else {
				inQuotes += 1;
			}
		}
		else if ((boolean == 4) && (inQuotes < 8)) {
			if (str[i] != '\"') {
				subName[subNameIndex] = str[i];
				subNameIndex += 1;
				subName[subNameIndex] = '\0';
			}
			else {
				inQuotes += 1;
			}
		}
		else if ((boolean == 5) && (inQuotes < 10)) {
			if (str[i] != '\"') {
				subAddress[subAddressIndex] = str[i];
				subAddressIndex += 1;
				subAddress[subAddressIndex] = '\0';
			}
			else {
				inQuotes += 1;
			}
		}
	}
	if ((source[0] == '\0') || (gedcVersion[0] == '\0') || (encoding[0] == '\0') || (subName[0] == '\0')) {
		return NULL;
	}
	
	obj = malloc(sizeof(GEDCOMobject)); //malloc allocation for object of type GEDCOMobject
    obj->individuals = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals); //initialize list for individuals in 
    obj->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	
	
	Submitter* submObj = malloc(sizeof (Submitter) + 255);
	submObj->otherFields = initializeList(&printField, &deleteField, &compareFields);
	strcpy(submObj->submitterName, subName);
	strcpy(submObj->address, subAddress);
	obj->submitter = submObj;
	
	Header* headObj = malloc(sizeof(Header));
	headObj->otherFields = initializeList(&printField, &deleteField, &compareFields);
	strcpy(headObj->source, source);
	headObj->gedcVersion = atof(gedcVersion);
	headObj->encoding = getCharSet(encoding);
	headObj->submitter = submObj;
	obj->header = headObj;
	
	//free(encoding);
	return obj;
}

void addIndividual(GEDCOMobject* obj, const Individual* toBeAdded) {
	if ((obj == NULL) || (toBeAdded == NULL)) {
		return;
	}
	insertBack(&(obj->individuals), (void*)toBeAdded);
	return;
}

char* iListToJSON(List iList) { 
	char* str = malloc(255*(sizeof(char)));
	char* strIndi = NULL;
	ListIterator iter;
	iter = createIterator(iList);
	Individual* indi = NULL;
	int listLength = getLength(iList);
	int count = 1;
	int newLen = 0;
	if (listLength == 0) {
		strcpy(str, "[]");
		return str;
	}
	
	strcpy(str, "[");
	while ((indi = nextElement(&iter)) != NULL) {
		strIndi = indToJSON(indi);
		newLen = strlen(str) + 50 + strlen(strIndi);
		str = (char*) realloc(str, newLen);
		strcat(str, strIndi);
		free(strIndi);
		if (count < listLength) {
			strcat(str, ",");
		}
		count++;
	}
	strcat(str, "]");
	return str;
}

char* gListToJSON(List gList) {
	char* str = malloc(255*(sizeof(char)));
	char* strGen = NULL;
	ListIterator iter;
	iter = createIterator(gList);
	List* generationList = NULL;
	int listLength = getLength(gList);
	int count = 1;
	int newLen = 0;
	if (listLength == 0) {
		strcpy(str, "[]");
		return str;
	}
	
	strcpy(str, "[");
	while ((generationList = nextElement(&iter)) != NULL) {
		strGen = iListToJSON(*generationList);
		newLen = strlen(str) + 50 + strlen(strGen);
		str = (char*) realloc(str, newLen);
		strcat(str, strGen);
		free(strGen);
		if (count < listLength) {
			strcat(str, ",");
		}
		count++;
	}
	strcat(str, "]");
	return str;
}
