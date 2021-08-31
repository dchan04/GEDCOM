 /**
 * @file LinkedListAPI.h
 * @brief File containing the function definitions of a doubly linked list
 */

#ifndef _GEDCOMUTILITIES_H_
#define _GEDCOMUTILITIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GEDCOMparser.h"

typedef enum {
    NO, YES
} boolInvGedcom;

typedef enum {
    ZERO, ONE, TWO, THREE, FOUR, NONE
} levelNumber;

typedef enum {
    EMPTY, FOUND
} reqHeaderTags;

typedef enum {
    NAME, GIVN, SURN, FAMS, FAMC, evenOther, evenDate, evenPlace, EVENT, EVEN, iOther, iNONE
} indivTag;

typedef enum {
    submNAME, submAddress, addrCont, sOther, sNONE
} submTag;

typedef enum {
    HUSB, WIFE, CHIL, fOTHER, fEVENT, feventOTHER, fDATE, fPLACE, fNONE
} famTag;

typedef enum {
    HEAD, SUBMITTER, FAMILY, INDIVIDUAL, newRECORD, TRLR
} recType;

typedef enum {
    hSOUR, hSUBM, hGEDC, hCHAR, hOTHER, oNONE
} levelOneHeadTag;

typedef enum {
    hGEDCvers, hGEDCform, tOther, tNONE
} levelTwoHeadTag;

typedef enum {
    UNINITIALIZED, INITIALIZED
} init;

typedef enum {
    FAMSt, FAMCt, HUSBt, WIFEt, CHILt, SUBMt, NONEt
} secondPass;

typedef struct data{
    void* pointer[5];
	int ptrLength;
    int key;
	char recordType;
	char XREF[10];
	
} keyData;

typedef struct addresses{
    void* pointer;
} addressArray;


/* hashKey
 * Description: Converts the int XREF key into hash key. hash key = (XREF Key)%50
 * in: Unhashed XREF key of type int
 * out: Hashed XREF key
 */
int hashKey(int key);

/* insert
 * Description: Inserts the ptr to a Record/Header into the hash table using the hash key and a char containing info on the record type ('U' for submitter, 'F' for family, or 'I' for individual)
 * in: Hash key, Record Type ('U' for submitter, 'F' for family, or 'I' for individual), ptr to an XREF record, pointer to the hash table created in createGEDCOM() function.
 * out:
 */
void insert(int k, char rType, void* ptr, keyData* hashArray, char* key);

/* search
 * Description: Using the Hash key, search the hash table.
 * in: Hash key, Char which tells us record type ('U' for submitter, 'F' for family, or 'I' for individual), pointer to the hash table created in createGEDCOM() function.
 * out: If found, return hashTable[hashKey]. 
 * 		If not found, return hashTable[51] which is only used if hash key cannot by found in hashTable (a way of returning NULL for example, not actually NULL)
 */
void* search(int key, char recType, keyData* hashTable, char* xref);

/* getKey
 * Description: Converts the string key into an integer key and returns the integer key (Which is used for the hash table)
 * in: XREF_ID (The references in the .ged files. ex.@F01@)
 * out: Hash Key of type int. (Note to self: this integer value might be before the module operator (%) is performed on it)
 */
int getKey(char* key);


/* deleteDummy
 * Description: a dummy delete function for Lists whos references designated as "weak". The function does nothing
 * in: void pointer to the List
 * out:
 */
void deleteDummy(void* toBeDeleted);

/* resetString
 * Description: Resets a char array by replacing every char with '\0'. Used for resetting the char array
 * in: Char array, Max size of char array
 * out: Char array with every index replaced with '\0'
 */
void resetString(char* array, int size);

void clearDynamicList(List* list);

/* getCharSet
 * Description: Converts the string of the found GEDC vers from .ged file into enum of type CharSet
 * in: String value which contains one of the following (ANSEL, UTF8, UNICODE, ASCII).
 * out: The enum equivalent of the string value of type CharSet
 */
CharSet getCharSet(char* chSet);

/* findDescendants
 * Description: A getDescendants helper function that recursively finds descendants of an Individual. Manipulates the descendant's list by adding the descendants of the individual
 * in: Pointer to the list containing descendants, Individual who's descedants we are finding
 * out: 
 */
void findDescendants(List* descendentList, const Individual* person);


/* GEDCOMerror
 * Description: returns GEDCOMerror
 * in: variable of type ErrorCode, int line number
 * out: GEDCOMerror with err and line
 */
GEDCOMerror returnErr(ErrorCode err, int line);

/* charSetString
 * Description: Returns a string version of the charSet encoding
 * in: charSet Enum variable
 * out: String version of the charSet Enum
 */
char* charSetString(CharSet cSet);

List* initializeDynamicList(char* (*printFunction)(void* toBePrinted),void (*deleteFunction)(void* toBeDeleted),int (*compareFunction)(const void* first,const void* second));

void helperAllDescendantsList(List* descendantList, List** generationList ,const Individual* person, int curGen);
void helperDescendantListNFix(List* descendantList, List** generationList , const Individual* person, unsigned int maxGen, int curGen);

void helperGetAncestorListNFix(List* ancestorList, List** parentList ,const Individual* person, unsigned int maxGen, int curGen, addressArray* checkedPeople, int* cpIndex);
void helperAllAncestorListN(List* ancestorList, List** parentList ,const Individual* person, int curGen);
char* createGEDCOMFile(char* fileName, char* JSON);
char* gedcomToJSON(char* fileName);
char* callAddIndividual(char* fileName, char* JSONIndiv);
bool compareBool(const void* first,const void* second);
char* getDescendantWrapper(char* givenName, char* surname, char* fileName, int maxGen);
char* getAncestorsWrapper(char* givenName, char* surname, char* fileName, int maxGen);
char* getListOfIndividuals(char* fileName);
#endif
