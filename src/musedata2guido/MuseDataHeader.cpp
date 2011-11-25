/*
	GUIDO Library
	Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
// MuseDataHeader.cpp: implementation of class CMuseDataHeader.
//
//////////////////////////////////////////////////////////////////////

#include "MuseDataHeader.h"
#include "ErrorHandling.h"
#include <string>

//#include "secureio.h"
#include <stdio.h>
#define snprintf _snprintf

//////////////////////////////////////////////////////////////////////
// construction/deconstruction
//////////////////////////////////////////////////////////////////////

CMuseDataHeader::CMuseDataHeader()
{
	parseLineNr=0;
	attributesRead=0;
}

CMuseDataHeader::~CMuseDataHeader()
{

}

int CMuseDataHeader::parseFromRecord(char *line,int lineNr)
{
	parseLineNr++;
	switch(parseLineNr){
	case 1: strncpy(rec1,line,80);break;
	case 2: strncpy(rec2,line,80);break;
	case 3: strncpy(rec3,line,80);break;
	case 4: strncpy(date_name,line,80);break;
	case 5: strncpy(work_movement,line,80);break;
	case 6: strncpy(source,line,80);break;
	case 7: strncpy(movement_title,line,80);break;
	case 8: strncpy(work_title,line,80);break;
	case 9: strncpy(name_of_part,line,80);break;
	case 10: parseMiscDest(line);break;
	case 11: return parseGroupMemberships(line);break;
	default:
		if(line[0]=='$'){
			//return parseMusicalAttributes(line,lineNr);
			return 1;
		} else {
			if (attributesRead){
//TODO: check read attributes!
				return 1;
			} //this is the first line after the header!!
			return parseGroupPart(line);
		}
	}
	return 0;
}

int CMuseDataHeader::parseMiscDest(char *)
{
	return 0;
}

int CMuseDataHeader::parseGroupMemberships(char *)
{
	return 0;
}

int CMuseDataHeader::parseMusicalAttributes(char*line,int lineNr)
{
	attributesRead=1;
	if(line[1]!=' '){
		MUSEERROR(err_EditorialLevelNotSupported,lineNr,2);
	}
	if(line[2]!=' '){
		MUSEERROR(err_FootnoteNotSupported,lineNr,3);
	}

	if(line[1]==0){return 0;}
	if(line[2]==0){return 0;}

	int col=3;
	while(line[col]!=0){
		int rv=0;
		switch(line[col]){
		//single-integer-attributes:
		case 'K':rv=parseAttribute(&line[col+1],&rdKey);break;
		case 'Q':rv=parseAttribute(&line[col+1],&rdDivisionsPerQuarter);break;
		case 'X':rv=parseAttribute(&line[col+1],&rdTransposingPart);break;
		case 'S':rv=parseAttribute(&line[col+1],&rdNumberOfStaves);break;
		case 'I':rv=parseAttribute(&line[col+1],&rdNumberOfInstruments);break;
		//clefs:
		case 'C':
			if(line[col+1]==':'){rv=parseAttribute(&line[col+1],&rdClef[0]);break;}
			if(line[col+1]=='1'){rv=parseAttribute(&line[col+2],&rdClef[0]);break;}
			if(line[col+1]=='2'){rv=parseAttribute(&line[col+2],&rdClef[1]);break;}
			rv=-1;break;
		//directives:
		case 'D':
			if(line[col+1]==':'){strncpy(rdDirective[0],&line[col+2],80);break;}
			if(line[col+1]=='1'){
				if(line[col+2]==':') {strncpy(rdDirective[0],&line[col+3],80);break;}
				else {rv=-1;break;}
			}
			if(line[col+1]=='2'){
				if(line[col+2]==':') {strncpy(rdDirective[1],&line[col+3],80);break;}
				else {rv=-1;break;}
			}
		}

		if(rv){
			MUSEERROR(err_ParseAttribute,lineNr,col);
		}

		//find next blank
		while( (line[col]!=0) && (line[col]!=' ') ){col++;}
		if(line[col]==' '){col++;}
	}
	return 0;
}

int CMuseDataHeader::parseGroupPart(char *)
{
	return 0;
}

int CMuseDataHeader::parseAttribute(char * s, int * v)
{
	if(s[0]!=':') return -1;
	if(sscanf(&s[1],"%i",v)!=1){return -1;}
	return 0;
}

int CMuseDataHeader::getrdDivisionsPerQuarter()
{
	return rdDivisionsPerQuarter;
}
