#ifndef _TSO_H_
#define _TSO_H_
// 11 adds sim start time
#define MIN_TSOFILEVERSION 8
#define TSOFILEVERSION   11
#define TSOSTORAGEMETHOD 3    /* default if not specified */
#define MAXSPECIEID 16
/*
**-------------------------------------------------------------------
** Source Index data
**-------------------------------------------------------------------
*/
typedef struct {
	int id;
	int fileIndex;
	__file_pos_t fileOffset;
	int dataLength;
	PTEVAIndexData sourceData;
} DataIndex, *PDataIndex;
/*
**-------------------------------------------------------------------
** Group header data
**-------------------------------------------------------------------
*/
typedef struct {
    char groupType;
    short count;
} GroupHeader, *PGroupHeader;
/*
**-------------------------------------------------------------------
** TSO file data
**-------------------------------------------------------------------
*/
typedef struct {
	char header[32];
	FILE *stream;
	FILE **streams;
	FILE *indexStream;
	FILE *headerStream;
	char *tsodir;
	char *baseName;
	int count;
	int numFiles;
	PDataIndex *index;
	int curIndex;
	char **names;
	int curFile;
	// 0 = all data - time x nodes
	// 1 = nodes x time - only nodes w/non-zero
	// 2 = nodes x time - only nodes w/non-zero, grouped
	int storageMethod;
	// 1 = orignal version
	// 2 = added source start/stop/strength to each simulation output
	// 3 = added file header
	int fileVersion;
	float *linkdata;
	int nBytesForNodeID;
	fpos_t pos_cdata;
	float *nodedata;
	// whether or not to fill strings in binary file with spaces
	// or leave them as null characters
	int spaceFillStrings;
} TSO, *PTSO;
#endif
