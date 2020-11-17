#include "GEDCOMutilities.h"

int hashKey(int key) {
	return key%50;
}

void insert(int k, char rType, void* ptr, keyData* hashTable, char* key) {
	int hashIndex = hashKey(k);
	while ((hashTable[hashIndex].recordType != '\0') && (hashTable[hashIndex].key != 0)) {
		++hashIndex;
		hashIndex = hashKey(hashIndex);
	}
	hashTable[hashIndex].recordType = rType;
	hashTable[hashIndex].pointer[0] = ptr;
	hashTable[hashIndex].key = k;
	strcpy(hashTable[hashIndex].XREF, key);
	hashTable[hashIndex].ptrLength += 1;
}

void* search(int key, char recType, keyData* hashTable, char* xref) {
	int hashIndex = hashKey(key);
	while(hashTable[hashIndex].key != 0) {
		if ((hashTable[hashIndex].key == key)&&(hashTable[hashIndex].recordType == recType)) {
			for (int i = 0; i < hashTable[hashIndex].ptrLength; i++) {
				if (strcmp(hashTable[hashIndex].XREF, xref) == 0) {
					return hashTable[hashIndex].pointer[i];
				}
			}
		}
		++hashIndex;
		hashIndex = hashKey(hashIndex);
	}
	hashTable[51].key = 0;
	hashTable[51].recordType = 'e';
	return hashTable[51].pointer[0];
}

int getKey(char* key) {
	//GET KEY
	int hashedKey = 0;
	char skipZero = 'y';
	for (int i = 0; i < strlen(key); i++) {
		if ((key[i] == '0') && (skipZero == 'y') && (i > 1)) {
			//skip
		}
		else if ((key[i] != '0') && (skipZero == 'y') && (i > 1)) {
			skipZero = 'n'; //stop skipping
			hashedKey = hashedKey + key[i];
		}
		else {
			hashedKey = hashedKey + key[i];
		}
	}
	return hashedKey;
}

void resetString(char* array, int size) {
    for (int i = 0; i < size; i++) {
        array[i] = '\0';
    }
}

CharSet getCharSet(char* chSet) { //Gets char set ENUM
    if (strcmp((chSet), "ANSEL") == 0) {
        return ANSEL;
    } else if (strcmp((chSet), "UTF-8") == 0) {
        return UTF8;
    } else if (strcmp((chSet), "UNICODE") == 0) {
        return UNICODE;
    }
    return ASCII;
}

void deleteDummy(void* toBeDeleted) {
}

void clearDynamicList(List* list){
	
    if (list == NULL){
		return;
	}
	
	if (list->head == NULL && list->tail == NULL){
		return;
	}
	
	Node* tmp;
	
	while (list->head != NULL){
		list->deleteData(list->head->data);
		tmp = list->head;
		list->head = list->head->next;
		free(tmp);
	}
	
	list->head = NULL;
	list->tail = NULL;
}

void findDescendants(List* descendantList, const Individual* person) {
	Family* family = NULL;
	Individual* child = NULL;
	//Individual* checkIndiv = NULL;
	ListIterator famList = createIterator(person->families);
	ListIterator childList;
	//ListIterator checkList;
	char cont = 'Y';
    while ((family = nextElement(&famList)) != NULL){
		if ((person == family->husband) || (person == family->wife)) {
			childList = createIterator(family->children);
			while ((child = nextElement(&childList)) != NULL) {
				/*checkList = createIterator(*descendantList); 
				while ((checkIndiv = nextElement(&checkList)) != NULL) {
					if ((strcmp(checkIndiv->givenName, child->givenName) == 0) && (strcmp(checkIndiv->surname, child->surname) == 0)) {
						cont = 'N';
					}
				}*/
				if (cont != 'N') {
					Individual* descendent = malloc(sizeof (Individual));
					descendent->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
					descendent->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
					descendent->otherFields = initializeList(&printField, &deleteField, &compareFields);
					descendent->givenName = malloc(120 * sizeof (char));
					descendent->surname = malloc(120 * sizeof (char));
					strcpy(descendent->givenName, "\0");
					strcpy(descendent->surname, "\0");
					if (child->givenName != NULL) {
						strcpy(descendent->givenName,child->givenName);
					}
					if (child->surname != NULL) {
						strcpy(descendent->surname,child->surname);
					}
					insertBack((descendantList), (void*) (descendent));
					findDescendants(descendantList, child);
				}
				cont = 'Y';
			}	
		}
	}
}

GEDCOMerror returnErr(ErrorCode err, int line) {
	GEDCOMerror gedError;
	gedError.type = err;
	gedError.line = line;
	return gedError;
}

char* charSetString(CharSet cSet) {
	switch (cSet) {
		case ANSEL:
			return "ANSEL\n";
			break;
		case UTF8:
			return "UTF-8\n";
			break;
		case UNICODE:
			return "UNICODE\n";
			break;
		case ASCII:
			return "ASCII\n";
			break;
	}
	return " ";
}

List* initializeDynamicList(char* (*printFunction)(void* toBePrinted),void (*deleteFunction)(void* toBeDeleted),int (*compareFunction)(const void* first,const void* second)){
	List* mallocList = (List*)malloc(sizeof(List));
	
    //Asserts create a partial function...
    assert(printFunction != NULL);
    assert(deleteFunction != NULL);
    assert(compareFunction != NULL);
	
	mallocList->head = NULL;
	mallocList->tail = NULL;
	mallocList->length = 0;
	mallocList->deleteData = deleteFunction;
	mallocList->compare = compareFunction;
	mallocList->printData = printFunction;
	
	return mallocList;
}

void helperDescendantListNFix(List* descendantList, List** generationList ,const Individual* person, unsigned int maxGen, int curGen) {
	if ((person == NULL) || (maxGen == curGen)){
		return;
	}
	
	//pointers
	Family* famPtr;
	Individual* childPtr;
	
	//iterators
	ListIterator famList = createIterator(person->families);
	ListIterator childList;
	
	while ((famPtr = nextElement(&famList)) != NULL)		
	{
		if (((famPtr->husband == person) || (famPtr->wife == person)) && (getLength(famPtr->children) != 0))
		{
			childList = createIterator(famPtr->children);
			while ((childPtr = nextElement(&childList)) != NULL)
			{
				Individual* descendent = malloc(sizeof (Individual));
				descendent->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
				descendent->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
				descendent->otherFields = initializeList(&printField, &deleteField, &compareFields);
				descendent->givenName = malloc(120 * sizeof (char));
				descendent->surname = malloc(120 * sizeof (char));
				strcpy(descendent->givenName, "\0");
				strcpy(descendent->surname, "\0");
				
				if (childPtr->givenName != NULL) 
				{
					strcpy(descendent->givenName,childPtr->givenName);
				}
				if (childPtr->surname != NULL) 
				{
					strcpy(descendent->surname,childPtr->surname);
				}
				
				if (descendent->surname[0] == '\0') 
				{
					insertBack((generationList[curGen]), (void*)(descendent));
				}
				else 
				{
					insertSorted((generationList[curGen]), (void*)(descendent));
				}
				
				curGen = curGen + 1;
				helperDescendantListNFix(descendantList, generationList, childPtr, maxGen, curGen);
				curGen = curGen - 1;
			}
		}
	}
	return;
}

void helperAllDescendantsList(List* descendantList, List** generationList ,const Individual* person, int curGen) {
	if (person == NULL){
		return;
	}
	
	//pointers
	Family* famPtr = NULL;;
	Individual* childPtr = NULL;
	
	//iterators
	ListIterator famList = createIterator(person->families);
	ListIterator childList;
	
	while ((famPtr = nextElement(&famList)) != NULL)		
	{
		if (((famPtr->husband == person) || (famPtr->wife == person)) && (getLength(famPtr->children) != 0))
		{
			childList = createIterator(famPtr->children);
			while ((childPtr = nextElement(&childList)) != NULL)
			{
				Individual* descendent = malloc(sizeof (Individual));
				descendent->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
				descendent->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
				descendent->otherFields = initializeList(&printField, &deleteField, &compareFields);
				descendent->givenName = malloc(120 * sizeof (char));
				descendent->surname = malloc(120 * sizeof (char));
				strcpy(descendent->givenName, "\0");
				strcpy(descendent->surname, "\0");
				
				if (childPtr->givenName != NULL) 
				{
					strcpy(descendent->givenName,childPtr->givenName);
				}
				if (childPtr->surname != NULL) 
				{
					strcpy(descendent->surname,childPtr->surname);
				}
				
				if (descendent->surname[0] == '\0') 
				{
					insertBack((generationList[curGen]), (void*)(descendent));
				}
				else 
				{
					insertSorted((generationList[curGen]), (void*)(descendent));
				}
				
				curGen = curGen + 1;
				helperAllDescendantsList(descendantList, generationList, childPtr, curGen);
				curGen = curGen - 1;
			}
		}
	}
	return;
}

void helperGetAncestorListNFix(List* ancestorList, List** parentList ,const Individual* person, unsigned int maxGen, int curGen, addressArray* checkedPeople, int* cpIndex) {
	if ((person == NULL) || (maxGen == curGen)){
		return;
	}
	//pointers
	Family* famPtr = NULL;
	char skipHusband = 'N';
	char skipWife = 'N';
	
	//iterators
	ListIterator famList = createIterator(person->families);
	while ((famPtr = nextElement(&famList)) != NULL)		
	{
		for (int i = 0; i < *cpIndex; i++) {
			if (famPtr->wife == checkedPeople[i].pointer) {
				skipWife = 'Y';
			}
			else if (famPtr->husband == checkedPeople[i].pointer) {
				skipHusband = 'Y';
			}
		}
		if ((famPtr->wife != person) && (famPtr->husband != person)) {
			if ((famPtr->wife != NULL) && (skipWife != 'Y')) {
				Individual* wife = malloc(sizeof (Individual));
				wife->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
				wife->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
				wife->otherFields = initializeList(&printField, &deleteField, &compareFields);
				wife->givenName = malloc(120 * sizeof (char));
				wife->surname = malloc(120 * sizeof (char));
				strcpy(wife->givenName, "\0");
				strcpy(wife->surname, "\0");
				if (wife->givenName != NULL) 
				{
					strcpy(wife->givenName,famPtr->wife->givenName);
				}
				if (wife->surname != NULL) 
				{
					strcpy(wife->surname,famPtr->wife->surname);
				}
				if (wife->surname[0] == '\0') 
				{
					insertBack((parentList[curGen]), (void*)(wife));
				}
				else 
				{
					insertSorted((parentList[curGen]), (void*)(wife));
				}
				checkedPeople[*cpIndex].pointer = famPtr->wife;
				*cpIndex = *cpIndex + 1;
				curGen = curGen + 1;
				helperGetAncestorListNFix(ancestorList, parentList, famPtr->wife, maxGen, curGen, checkedPeople, cpIndex);
				curGen = curGen - 1;
			}
			
			if ((famPtr->husband != NULL) && (skipHusband != 'Y')) {
				Individual* husband = malloc(sizeof (Individual));
				husband->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
				husband->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
				husband->otherFields = initializeList(&printField, &deleteField, &compareFields);
				husband->givenName = malloc(120 * sizeof (char));
				husband->surname = malloc(120 * sizeof (char));
				strcpy(husband->givenName, "\0");
				strcpy(husband->surname, "\0");
				if (husband->givenName != NULL) 
				{
					strcpy(husband->givenName,famPtr->husband->givenName);
				}
				if (husband->surname != NULL) 
				{
					strcpy(husband->surname,famPtr->husband->surname);
				}
				if (husband->surname[0] == '\0') 
				{
					insertBack((parentList[curGen]), (void*)(husband));
				}
				else 
				{
					insertSorted((parentList[curGen]), (void*)(husband));
				}
				checkedPeople[*cpIndex].pointer = famPtr->husband;
				*cpIndex = *cpIndex + 1;
				curGen = curGen + 1;
				helperGetAncestorListNFix(ancestorList, parentList, famPtr->husband, maxGen, curGen, checkedPeople, cpIndex);
				curGen = curGen - 1;
			}
			skipWife = 'N';
			skipHusband = 'N';
		}
	}
	return;
}
void helperAllAncestorListN(List* ancestorList, List** parentList ,const Individual* person, int curGen) {
	if (person == NULL){
		return;
	}
	//pointers
	Family* famPtr = NULL;
	
	//iterators
	ListIterator famList = createIterator(person->families);
	while ((famPtr = nextElement(&famList)) != NULL)		
	{
		if ((famPtr->wife == person) || (famPtr->husband == person)) {
			return;
		}
		if (famPtr->wife != NULL) {
			Individual* wife = malloc(sizeof (Individual));
			wife->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
			wife->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
			wife->otherFields = initializeList(&printField, &deleteField, &compareFields);
			wife->givenName = malloc(120 * sizeof (char));
			wife->surname = malloc(120 * sizeof (char));
			strcpy(wife->givenName, "\0");
			strcpy(wife->surname, "\0");
			if (wife->givenName != NULL) 
			{
				strcpy(wife->givenName,famPtr->wife->givenName);
			}
			if (wife->surname != NULL) 
			{
				strcpy(wife->surname,famPtr->wife->surname);
			}
			if (wife->surname[0] == '\0') 
			{
				insertBack((parentList[curGen]), (void*)(wife));
			}
			else 
			{
				insertSorted((parentList[curGen]), (void*)(wife));
			}
			curGen = curGen + 1;
			helperAllAncestorListN(ancestorList, parentList, famPtr->wife, curGen);
			curGen = curGen - 1;
		}
		
		if (famPtr->husband != NULL) {
			Individual* husband = malloc(sizeof (Individual));
			husband->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
			husband->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
			husband->otherFields = initializeList(&printField, &deleteField, &compareFields);
			husband->givenName = malloc(120 * sizeof (char));
			husband->surname = malloc(120 * sizeof (char));
			strcpy(husband->givenName, "\0");
			strcpy(husband->surname, "\0");
			if (husband->givenName != NULL) 
			{
				strcpy(husband->givenName,famPtr->husband->givenName);
			}
			if (husband->surname != NULL) 
			{
				strcpy(husband->surname,famPtr->husband->surname);
			}
			if (husband->surname[0] == '\0') 
			{
				insertBack((parentList[curGen]), (void*)(husband));
			}
			else 
			{
				insertSorted((parentList[curGen]), (void*)(husband));
			}
			curGen = curGen + 1;
			helperAllAncestorListN(ancestorList, parentList, famPtr->husband, curGen);
			curGen = curGen - 1;
		}
	}
	return;
}

char* createGEDCOMFile(char* fileName, char* JSON) {
	char* message = malloc(100*sizeof(char));
	strcpy(message, "");
	//Create GEDCOM obj
	GEDCOMobject* obj;
	obj = JSONtoGEDCOM(JSON);
	
	//Validate GEDCOM obj
	GEDCOMerror validateObj;
	validateObj.type = validateGEDCOM(obj);
	validateObj.line = -1;
	
	//writeGECOM
	GEDCOMerror writeObj;
	if (validateObj.type == OK) {
		writeObj = writeGEDCOM(fileName,obj);
		if (writeObj.type == OK) {
			strcpy(message, "OK");
		}
		else {
			strcpy(message, "WRITE_ERROR");
		}
	}
	else 
	{
		if (validateObj.type == INV_GEDCOM) {
			strcpy(message, "INV_GEDCOM");
		}
		else if (validateObj.type == INV_HEADER) {
			strcpy(message, "INV_HEADER");
		}
		else if (validateObj.type == INV_RECORD) {
			strcpy(message, "INV_RECORD");
		}
	}
	return message;
}

char* gedcomToJSON(char* fileName) {
	char* JSONString = malloc(255 * sizeof(char));
	char enumString[10];
	char floatString[10];
	char str[10];
	int count = 0;
	
	//create GEDCOM object
	GEDCOMobject* obj = NULL;
	GEDCOMerror errType;
	errType = createGEDCOM(fileName, &obj);
	if (errType.type != OK) {
		strcpy(JSONString, "{\"message\":\"createGEDCOM error\"}");
		return JSONString;
	}
	
	//Verify GEDCOM object (if valid or invalid)
	GEDCOMerror validateObj;
	validateObj.type = validateGEDCOM(obj);
	validateObj.line = -1;
	if (validateObj.type == INV_GEDCOM) { //return invalid GEDCOM message
		strcpy(JSONString, "{\"message\":\"INV_GEDCOM\"}");
		return JSONString;
		
	}
	else if (validateObj.type == INV_HEADER) { //return invalid HEADER message
		strcpy(JSONString, "{\"message\":\"INV_HEADER\"}");
		return JSONString;
	}
	else if (validateObj.type == INV_RECORD) { //return invalid RECORD message
		strcpy(JSONString, "{\"message\":\"INV_RECORD\"}");
		return JSONString;
	}
	
	//if createGEDCOM is successful, extract information
	strcpy(JSONString, "{\"fileName\":\"");
	strcat(JSONString, fileName);
	strcat(JSONString, "\",");
	
	strcat(JSONString, "\"source\":\"");
	strcat(JSONString, obj->header->source);
	strcat(JSONString, "\",");
	
	strcat(JSONString, "\"GEDCversion\":\"");
	snprintf(floatString, sizeof (JSONString), "%.1f", obj->header->gedcVersion);
	strcat(JSONString, floatString);
	strcat(JSONString, "\",");
	
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
	strcat(JSONString, "\"encoding\":\"");
	strcat(JSONString, enumString);
	strcat(JSONString, "\",");
	
	strcat(JSONString, "\"subName\":\"");
	strcat(JSONString, obj->submitter->submitterName);
	strcat(JSONString, "\",");
	
	strcat(JSONString, "\"subAddress\":\"");
	strcat(JSONString, obj->submitter->address);
	strcat(JSONString, "\",");
	
	strcat(JSONString, "\"numIndividuals\":\"");
	count = getLength(obj->individuals);
	sprintf(str, "%d", count);
	strcat(JSONString, str);
	strcat(JSONString, "\",");
	
	resetString(str, 10);
	
	strcat(JSONString, "\"numFamilies\":\"");
	count = getLength(obj->families);
	sprintf(str, "%d", count);
	strcat(JSONString,  str);
	strcat(JSONString, "\",");
	
	strcat(JSONString, "\"message\":\"OK");
	strcat(JSONString, "\"");
	strcat(JSONString, "}");
	
	//Delete gedcom object to free memory. No longer needed
	deleteGEDCOM(obj);
	
	//Return message depending on success or fail in the GEDCOM object created
	return JSONString;
}

char* callAddIndividual(char* fileName, char* JSONIndiv){
	char* message = malloc(100 * sizeof(char));
	Individual* ind = NULL;
	strcpy(message, "");
	
	//create GEDCOM object
	GEDCOMobject* obj = NULL;
	GEDCOMerror errType;
	errType = createGEDCOM(fileName, &obj);
	if (errType.type != OK) {
		strcpy(message, "{\"message\":\"createGEDCOM error\"}");
		return message;
	}
	
	//Verify GEDCOM object (if valid or invalid)
	GEDCOMerror validateObj;
	validateObj.type = validateGEDCOM(obj);
	validateObj.line = -1;
	if (validateObj.type == INV_GEDCOM) { //return invalid GEDCOM message
		strcpy(message, "{\"message\":\"INV_GEDCOM\"}");
		return message;
		
	}
	else if (validateObj.type == INV_HEADER) { //return invalid HEADER message
		strcpy(message, "{\"message\":\"INV_HEADER\"}");
		return message;
	}
	else if (validateObj.type == INV_RECORD) { //return invalid RECORD message
		strcpy(message, "{\"message\":\"INV_RECORD\"}");
		return message;
	}
	
	//Create and add new individual
	ind = JSONtoInd(JSONIndiv);
	addIndividual(obj, ind);
	
	//writeGECOM
	GEDCOMerror writeObj;
	if (validateObj.type == OK) {
		writeObj = writeGEDCOM(fileName,obj);
		if (writeObj.type == OK) {
			strcpy(message, "{\"message\":\"OK\"}");
		}
		else {
			strcpy(message, "WRITE_ERROR");
			strcpy(message, "{\"message\":\"WRITE_ERROR\"}");
		}
	}
	//delete new individual to free memory
	//Delete gedcom object to free memory. No longer needed
	deleteGEDCOM(obj);
	return message;
}
bool compareBool(const void* first,const void* second) {
    if(first == second){
        return true;
    }
    
    if(first == NULL || second == NULL)
    {
        return false;
    }

    bool boolresult;
    int result = compareIndividuals(first, second);
    if(result == 0)
    {
        boolresult = true;
    }
    else
    {
        boolresult = false;
    }

    return boolresult;
}

char* getDescendantWrapper(char* givenName, char* surname, char* fileName, int maxGen) {
	char* gListJSON; 
	
	//create GEDCOM object
	GEDCOMobject* obj = NULL;
	createGEDCOM(fileName, &obj);

	//Verify GEDCOM object (if valid or invalid)
	GEDCOMerror validateObj;
	validateObj.type = validateGEDCOM(obj);
	validateObj.line = -1;
	if (validateObj.type == INV_GEDCOM) { //return invalid GEDCOM message
		gListJSON = malloc(5*sizeof(char));
		strcpy(gListJSON, "[]");
		return gListJSON;
		
	}
	else if (validateObj.type == INV_HEADER) { //return invalid HEADER message
		gListJSON = malloc(5*sizeof(char));
		strcpy(gListJSON, "[]");
		return gListJSON;
	}
	else if (validateObj.type == INV_RECORD) { //return invalid RECORD message
		gListJSON = malloc(5*sizeof(char));
		strcpy(gListJSON, "[]");
		return gListJSON;
	}
	
	//find Person
	Individual* refIndividual = malloc(sizeof (Individual));
	refIndividual->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	refIndividual->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
	refIndividual->otherFields = initializeList(&printField, &deleteField, &compareFields);
	refIndividual->givenName = malloc(120 * sizeof (char));
	refIndividual->surname = malloc(120 * sizeof (char));
	strcpy(refIndividual->givenName, givenName);
	strcpy(refIndividual->surname, surname);
	Individual* individual = findPerson(obj, compareBool, refIndividual);
	//deleteIndividual((void*)refIndividual);
	
	//call getDescendantListN
	List getDescendantN = getDescendantListN(obj, individual, maxGen);
	//deleteIndividual((void*)individual);
	
	//pass generation list to gListToJSON() function
	gListJSON = gListToJSON(getDescendantN);
	//Delete gedcom object to free memory. No longer needed
	deleteGEDCOM(obj);
	return gListJSON;
}

char* getAncestorsWrapper(char* givenName, char* surname, char* fileName, int maxGen) {
	char* gListJSON; 

	//create GEDCOM object
	GEDCOMobject* obj = NULL;
	createGEDCOM(fileName, &obj);

	//Verify GEDCOM object (if valid or invalid)
	GEDCOMerror validateObj;
	validateObj.type = validateGEDCOM(obj);
	validateObj.line = -1;
	if (validateObj.type == INV_GEDCOM) { //return invalid GEDCOM message
		gListJSON = malloc(5*sizeof(char));
		strcpy(gListJSON, "[]");
		return gListJSON;
		
	}
	else if (validateObj.type == INV_HEADER) { //return invalid HEADER message
		gListJSON = malloc(5*sizeof(char));
		strcpy(gListJSON, "[]");
		return gListJSON;
	}
	else if (validateObj.type == INV_RECORD) { //return invalid RECORD message
		gListJSON = malloc(5*sizeof(char));
		strcpy(gListJSON, "[]");
		return gListJSON;
	}
	
	//find Person
	Individual* refIndividual = malloc(sizeof (Individual));
	refIndividual->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	refIndividual->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
	refIndividual->otherFields = initializeList(&printField, &deleteField, &compareFields);
	refIndividual->givenName = malloc(120 * sizeof (char));
	refIndividual->surname = malloc(120 * sizeof (char));
	strcpy(refIndividual->givenName, givenName);
	strcpy(refIndividual->surname, surname);
	Individual* individual = findPerson(obj, compareBool, refIndividual);
	//deleteIndividual((void*)refIndividual);
	
	//call getDescendantListN
	List getAncestorN = getAncestorListN(obj, individual, maxGen);
	//deleteIndividual((void*)individual);
	
	//pass generation list to gListToJSON() function
	gListJSON = gListToJSON(getAncestorN);
	//clearList(&getAncestorN);
	//Delete gedcom object to free memory. No longer needed
	deleteGEDCOM(obj);
	return gListJSON;
}

char* getListOfIndividuals(char* fileName) {
	char* individualList;
	
	//create object
	//create GEDCOM object
	GEDCOMobject* obj = NULL;
	GEDCOMerror errType;
	errType = createGEDCOM(fileName, &obj);
	if (errType.type != OK) {
		individualList = malloc(50*sizeof(char));
		strcpy(individualList, "{\"message\":\"createGEDCOM error\"}");
		return individualList;
	}
	individualList = iListToJSON(obj->individuals);
	deleteGEDCOM(obj);
	
	return individualList;
}

