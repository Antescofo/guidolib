/*
  GUIDO Library
  Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz
  Copyright (C) 2002-2017 Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

#include <iostream>
#include <sstream>
#include <cassert>

#include "ARNote.h"
#include "ARTrill.h"
#include "ARCluster.h"
#include "ARDefine.h"
#include "TimeUnwrap.h"
#include "nvstring.h"

using namespace std;

int gd_noteName2pc(const char * name);
const char * gd_pc2noteName(int fPitch);


ARNote::ARNote(const TYPE_DURATION & durationOfNote)
	:	ARMusicalEvent(durationOfNote), fName("empty"), fPitch(UNKNOWN), fOctave(MIN_REGISTER),
    fAccidentals(0), fIntensity(MIN_INTENSITY), fOrnament(NULL), fCluster(NULL), fOwnCluster(false), fIsLonelyInCluster(false),
    fClusterHaveToBeDrawn(false), fSubElementsHaveToBeDrawn(true), fAuto(false), fTremolo(0), fNoteAppearance(""), fOctava(0)
{
}

ARNote::ARNote(const TYPE_TIMEPOSITION & relativeTimePositionOfNote, const TYPE_DURATION & durationOfNote)
	:	ARMusicalEvent( relativeTimePositionOfNote, durationOfNote), fName("noname"), fPitch(UNKNOWN),
		fOctave(MIN_REGISTER), fAccidentals(0), fIntensity(MIN_INTENSITY), fOrnament(NULL), fCluster(NULL),
        fOwnCluster(false), fIsLonelyInCluster(false), fClusterHaveToBeDrawn(false), fSubElementsHaveToBeDrawn(true), fAuto(false), fTremolo(0),
        fNoteAppearance(""), fOctava(0)
{
}

ARNote::ARNote( const std::string & name, int accidentals, int octave, int numerator, int denominator, int intensity )
	:	ARMusicalEvent(numerator, denominator), fName( name ), fPitch ( UNKNOWN ),
		fOctave( octave ),	fAccidentals( accidentals ), fIntensity( intensity ),
		fOrnament(NULL), fCluster(NULL), fOwnCluster(false), fIsLonelyInCluster(false), fClusterHaveToBeDrawn(false), 
		fSubElementsHaveToBeDrawn(true), fAuto(false), fTremolo(0), fNoteAppearance(""), fOctava(0)
{
	assert(fAccidentals>=MIN_ACCIDENTALS);
	assert(fAccidentals<=MAX_ACCIDENTALS);
	fName = NVstring::to_lower(fName.c_str());
	fPitch = gd_noteName2pc(fName.c_str());
}

ARNote::ARNote(const ARNote & arnote, bool istied)
	:	ARMusicalEvent( (const ARMusicalEvent &) arnote),
		fName(arnote.fName), fOrnament(NULL),  fCluster(NULL), fOwnCluster(false), fIsLonelyInCluster(false),
        fClusterHaveToBeDrawn(false), fSubElementsHaveToBeDrawn(true), fAuto(true), fTremolo(0), fOctava(0)
{
	fPitch = arnote.fPitch;
	fOctave = arnote.fOctave;
	fAccidentals = arnote.fAccidentals;
	fAlter = arnote.getAlter();
	fIntensity = arnote.fIntensity;
    fVoiceNum = arnote.getVoiceNum(); // Added to fix a bug during chord copy (in doAutoBarlines)
	fOctava = arnote.getOctava();
	const ARTrill* trill = arnote.getOrnament();
	if (trill) {
		ARTrill* copy = new ARTrill(-1, trill);
		copy->setContinue();
		if (istied) copy->setIsAuto(true);
		setOrnament(copy);
	}
}

ARNote::~ARNote()
{
	if (fTrillOwner)	delete fOrnament;
	if (fOwnCluster)	delete fCluster;
}

ARMusicalObject * ARNote::Copy() const
{
	return new ARNote(*this);
}

static int iround (float value) {
	int integer = int(value);
	float remain = value - integer;
	return (remain < 0.5) ? integer : integer + 1;
}

int	ARNote::detune2Quarters(float detune)
{
	return iround(detune*2);	// detune in rounded quarter tones
}

void ARNote::browse(TimeUnwrap& mapper) const
{
	mapper.AtPos (this, TimeUnwrap::kNote);
}

void ARNote::transpose(int *pitch, int *octave, int accidental, int fChromaticSteps) {
    
    int rangechange = fChromaticSteps / 12;
    int fTableShift = fChromaticSteps % 12;
    if (fTableShift < 0) fTableShift = 12 + fTableShift;    // works only on positive values

    int sharps = 0;
    int curstep = 0;
    while (curstep != fTableShift) {
        curstep += 7;             // add a fifth
        curstep %= 12;            // modulus an octave
        sharps++;
    }
    fTableShift = (sharps >= 6 ? sharps - 12 : sharps);    // simplest key is chosen here

    // construct table of fifths
    std::vector<std::pair<int,int> >    fFifthCycle;    // the fifth cycle table
    for (int i=-2; i<=2; i++) {
        fFifthCycle.push_back(make_pair(NOTE_F, i));
        fFifthCycle.push_back(make_pair(NOTE_C, i));
        fFifthCycle.push_back(make_pair(NOTE_G, i));
        fFifthCycle.push_back(make_pair(NOTE_D, i));
        fFifthCycle.push_back(make_pair(NOTE_A, i));
        fFifthCycle.push_back(make_pair(NOTE_E, i));
        fFifthCycle.push_back(make_pair(NOTE_H, i));
    }
    int alter = 0;
    int npitch = *pitch;
    int octaveChange = 0;

    
    switch (*pitch) {
        case NOTE_CIS:
        case NOTE_DIS:
        case NOTE_FIS:
        case NOTE_GIS:
        case NOTE_AIS:
            npitch = *pitch - 7; // ex. CIS to C, DIS to D etc
            alter = 1;
            break;
        default:
            break;
    }
    alter += accidental;

    
    // retrieve first the normaized pitch integer class
    int pitch1 = npitch;
    // then browse the fifth cycle table
    for (int i=0; i < fFifthCycle.size(); i++) {
        // until we find the same pitch spelling (ie including name and accident)
        if ((fFifthCycle[i].second == alter) && (fFifthCycle[i].first == npitch)) {
            // then we shift into the table
            i += fTableShift;
            // make possible adjustments
            if (i > fFifthCycle.size()) i -= 12;
            else if (i < 0) i += 12;
            // and retrieve the resulting transposed pitch
            npitch = fFifthCycle[i].first;
            alter = fFifthCycle[i].second;
            // check now fro octave changes
            int pitch2 = npitch;
            // if pitch is lower but transposition is up: then increase octave
            if ((pitch2 < pitch1) && (fChromaticSteps > 0))
                octaveChange++;
            // if pitch is higher but transposition is down: then decrease octave
            else if ((pitch2 > pitch1) && (fChromaticSteps < 0))
                octaveChange--;
            *pitch = npitch;
            *octave += octaveChange;
            return;
        }
    }
}

int ARNote::getMidiPitch() const
{
	int oct = 12 * (fOctave+4);
	if (oct < 0) return 0;

	int pitch = -1;
	switch (fPitch) {
		case NOTE_C:
		case NOTE_D:
		case NOTE_E:	pitch = (fPitch - NOTE_C) * 2;
			break;
		case NOTE_F:
		case NOTE_G:
		case NOTE_A:
		case NOTE_H:	pitch = (fPitch - NOTE_C) * 2 - 1;
			break;
		
		case NOTE_CIS: 
		case NOTE_DIS:  pitch = (fPitch - NOTE_CIS) * 2 + 1;
			break;

		case NOTE_FIS:
		case NOTE_GIS:
		case NOTE_AIS:  pitch = (fPitch - NOTE_CIS) * 2 + 3;
			break;
		default:
			return pitch;
	}
	return oct + pitch + fAccidentals;
}

void ARNote::addFlat()
{
	--fAccidentals;
	assert(fAccidentals>=MIN_ACCIDENTALS);
}

void ARNote::addSharp()
{
	++fAccidentals;
	assert(fAccidentals<=MAX_ACCIDENTALS);
}

void ARNote::offsetpitch(int steps)
{
  // this just tries to offset the fPitch ...
	if (fPitch >= NOTE_C && fPitch <= NOTE_H)
	{
		int tmppitch = fPitch - NOTE_C;
		tmppitch += steps;
		int regchange = 0;
		while (tmppitch > 6)
		{
			tmppitch -= 7;
			++regchange;
		}
		while (tmppitch < 0)
		{
			tmppitch += 7;
			--regchange;
		}

		fPitch = tmppitch + NOTE_C;
		if (regchange)
			fOctave += regchange;
		fName = gd_pc2noteName(fPitch);
	}
	else
	{
	// we do not care right now ...
	}
}

void ARNote::setPitch(TYPE_PITCH newpitch)
{
	fPitch = newpitch;
	fName = gd_pc2noteName(fPitch);
}

void ARNote::setAccidentals(int theAccidentals)
{
	assert(fAccidentals>=MIN_ACCIDENTALS);
	assert(fAccidentals<=MAX_ACCIDENTALS);
	fAccidentals=theAccidentals;
}

void ARNote::setOrnament(const ARTrill * newOrnament, bool trillOwner)
{
	if (fTrillOwner)
		delete fOrnament;
//	fOrnament = newOrnament ? new ARTrill(-1, newOrnament) : 0;
	fOrnament = newOrnament;
	fTrillOwner = trillOwner;
}

ARCluster *ARNote::setCluster(ARCluster *inCluster,
                              bool inClusterHaveToBeDrawn,
                              bool inHaveToBeCreated)
{
    if (!fClusterHaveToBeDrawn && inClusterHaveToBeDrawn)
        fClusterHaveToBeDrawn = true;

    inCluster->setVoiceNum(getVoiceNum());

    if (inHaveToBeCreated) {
        fCluster = new ARCluster(inCluster);
		fOwnCluster = true; 
	}    
	else
        fCluster = inCluster;

    return fCluster;
}

bool ARNote::CanBeMerged(const ARMusicalEvent * ev2)
{
	if (ARMusicalEvent::CanBeMerged(ev2))
	{
		// type is OK (checked in ARMusicalEvent)
		const ARNote *arn = dynamic_cast<const ARNote *>(ev2);
		if (!arn) return false;
		
		if (arn->fPitch == this->fPitch 
			&& arn->fOctave == this->fOctave)
			return true;
	}
	return false;
}

void ARNote::setDuration(const TYPE_DURATION & newdur)
{
	ARMusicalEvent::setDuration(newdur);
	if (newdur == DURATION_0)
		mPoints = 0;
}

// this compares the name, fPitch, fOctave and fAccidentals
// returns 1 if it matches ...
int ARNote::CompareNameOctavePitch(const ARNote & nt)
{
	if (fName == nt.fName
		&& fPitch == nt.fPitch
		&& fOctave == nt.fOctave 
		&& fAccidentals == nt.fAccidentals)
		return 1;

	return 0;
}

void ARNote::forceNoteAppearance(NVstring noteAppearance) {
    fNoteAppearance = noteAppearance;
}

void ARNote::print(std::ostream& os) const
{
	os << getGMNName();
}

string ARNote::getGMNName () const
{
    const char* accidental = "";
    switch (getAccidentals()) {
		case -2:
			accidental = "&&";
			break;
		case -1:
			accidental = "&";
			break;
		case 1:
			accidental = "#";
			break;
		case 2:
			accidental = "##";
			break;
		default:
			break;
    }
	stringstream s;
	if (!isEmptyNote())
		s << getName() << accidental << getOctave() << "*" << getDuration();
    else
        s << getName() << "*" << getDuration();
	return s.str();
}

string ARNote::getPitchName () const
{
    std::string accidental = "";
    switch (getAccidentals()) {
        case -1:
            accidental = "b";
            break;
        case 1:
            accidental = "#";
            break;
        case 2:
            accidental = "##";
        case -2:
            accidental = "bb";
        default:
            break;
    }
    if (!isEmptyNote()) {
        stringstream s;
        s << getName() << accidental << getOctave()+3;
        return s.str();
    }
    else
        return "";
}

