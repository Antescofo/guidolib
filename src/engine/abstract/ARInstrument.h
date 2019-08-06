#ifndef ARInstrument_H
#define ARInstrument_H

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

#include <string>

#include "ARFontAble.h"


/*@mkdoc

@group:Text

@tagname:\instrument
@tagalias:\instr
@tagtype:P
@tagnotation:instrument marking
@tagdesc
@tagend

@params:
@param:name:string:the instrument name:*none*:false
@param:transp:string:a transposition string:*none*:true
@param:autopos:boolean:automatic position control:off:true
@fontparams:
@paramdes:
- **name** is the instrument name (e.g. "Violin I")
- **transp** is provided for transposing instruments. For example, with a Trumpet in B flat, you should set **transp** to "B&". When **transp** is set, a transposing key is automatically inserted and the notes are automatically transposed to the corresponding interval.
- **autopos** when "on", put the instrument string centered to the left of the corresponding staff. The default position if on top, left of the staff.
@paramend

*/

/** \brief not yet documented
*/
class ARInstrument : public ARFontAble
{
	public:
				 ARInstrument(bool autopos);
		virtual ~ARInstrument() {}

		virtual void setTagParameters (const TagParameterMap& params);

		virtual int	 getOrder() const				{ return kInstrumentOrder; }
		virtual const char*	getParamsStr() const	{ return kARInstrumentParams; };
		virtual const char*	getTagName() const		{ return "ARInstrument"; };
		virtual std::string getGMNName() const		{ return "\\instrument"; };
		virtual bool IsStateTag() const				{ return true; }

		const std::string& getName() const			{ return fName; }
		const std::string& getTransp() const		{ return fTransp; }
		bool			   autoPos() const			{ return fAutoPos; }

	private:
		std::string fName;
		std::string fTransp;
		bool		fAutoPos;
};

#endif
