/*

	erd2scada
	
	library containing functions for opening canary xml files, getting scada tag info, 
	converting to epanet index numbers, etc.

*/



#include "erd2scada.h"



/*
	open xml file, read in tags and set up structure.
*/
pSCADAinfo LIBEXPORT initializeSCADAinfo(char* xmlConfigFile) {
	
	int size, i;
	int numTags = 0;
	
	pSCADAinfo mySCADAinfo = calloc(1,sizeof(SCADAinfo));
	
	// read in xml file, send xpath queries, etc.
	xmlInitParser();
	LIBXML_TEST_VERSION
	
	xmlNodePtr cur;
	xmlAttr* curAttr;
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	xmlNodeSetPtr nodes;
	
	//debug
	//fprintf(stdout,"parsing xml config file: %s...\n",xmlFile);
	
	// parse the xml document
	doc = xmlParseFile(xmlConfigFile);
	// create xpath evaluation context
	xpathCtx = xmlXPathNewContext(doc);
	
	// evaluate xpath expression
	xpathObj = xmlXPathEvalExpression("/canary-database/general-settings/signal[@scada-id]", xpathCtx);
	
	nodes = xpathObj->nodesetval;
	
	
	// loop for each result in the set
	size = (nodes) ? nodes->nodeNr : 0;
	
	//debug
	//fprintf(stdout,"nodes found for scada-id: %i\n",size);
	
	//allocate memory for this many tags...
	mySCADAinfo->tags = calloc(size, sizeof(pSCADAtag));
	
	
	for(i=0; i < size; ++i) {
		cur = nodes->nodeTab[i];
		curAttr = cur->properties;
		
		//allocate memory for this tag
		mySCADAinfo->tags[i] = calloc(1,sizeof(SCADAtag));
		
		// now we set up the attributes in the SCADAtag structure, reading from the xml config.
		
		do {
			// curAttr->name == attribute name
			// curAttr->children->content == attribute value
			//fprintf(stdout,"%s -- %s\n",curAttr->name,curAttr->children->content);
			

			if(strcmp(curAttr->name,"epanet-ID") == 0)
				strncpy(mySCADAinfo->tags[i]->epanetID, curAttr->children->content, MAX_TAG_LEN);
				//mySCADAinfo->tags[i]->epanetIndex = atoi(curAttr->children->content);
				
			if(strcmp(curAttr->name,"scada-id") == 0) {
				strncpy(mySCADAinfo->tags[i]->scadaID,curAttr->children->content,MAX_TAG_LEN);
				// increment number of tags to count the one we just recorded.
				// let's record all tags, just to be safe.  perhaps all we need 
				// are the tags for flow and pressure, but let's leave that open.
				numTags++;
			}
			
			if(strcmp(curAttr->name,"short-id") == 0)
				strncpy(mySCADAinfo->tags[i]->shortID,curAttr->children->content,MAX_TAG_LEN);
			
			if(strcmp(curAttr->name,"signal-type") == 0) {
				if(strcmp(curAttr->children->content,"WQ") == 0)
					mySCADAinfo->tags[i]->signalType = WQ;
				else if(strcmp(curAttr->children->content,"FLOW") == 0)
					mySCADAinfo->tags[i]->signalType = FLOW;
				else if(strcmp(curAttr->children->content,"PRESSURE") == 0)
					mySCADAinfo->tags[i]->signalType = PRESSURE;
				else if(strcmp(curAttr->children->content,"VALVESTAT") == 0)
					mySCADAinfo->tags[i]->signalType = VALVESTAT;
				else
					mySCADAinfo->tags[i]->signalType = OTHER;
			}
			
			if(strcmp(curAttr->name,"units") == 0)
				strncpy(mySCADAinfo->tags[i]->units,curAttr->children->content,MAX_TAG_LEN);
				
			if(strcmp(curAttr->name,"description") == 0)
				strncpy(mySCADAinfo->tags[i]->description,curAttr->children->content,MAX_TAG_LEN);
			
			curAttr = curAttr->next;
		}
		while(curAttr != NULL);
		
		mySCADAinfo->numTags = numTags;
	}
	
	
	
	return mySCADAinfo;
}






int LIBEXPORT getTagIndexByScada(char* myScadaID, pSCADAinfo mySCADAinfo) {
	
	int i;
	
	for(i = 0; i < mySCADAinfo->numTags; i++) {
		if(strcmp(mySCADAinfo->tags[i]->scadaID, myScadaID) == 0)
			return i;
	}
	
	return -1;
}







