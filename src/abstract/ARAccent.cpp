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

#include <iostream>
#include "ARAccent.h"
#include "TagParameterList.h"
#include "ListOfStrings.h"

#include "TimeUnwrap.h"


ListOfTPLs ARAccent::ltpls(1);

void ARAccent::setTagParameterList(TagParameterList & tpl)
{
	if (ltpls.empty())
	{
		// create a list of string ...

		ListOfStrings lstrs; // (1); std::vector test impl
		lstrs.AddTail((""
			// "S,text,,r;U,dy,-1,o"
			));
		CreateListOfTPLs(ltpls,lstrs);
	}

	TagParameterList * rtpl = NULL;
	int ret = MatchListOfTPLsWithTPL(ltpls,tpl,& rtpl);

	if (ret>=0 && rtpl)
	{
		// we found a match!
		if (ret == 0)
		{
			// Get The TagParameters ...
			// text = 
			//	TagParameterString::cast(rtpl->RemoveHead());
			//assert(text);

		}

		delete rtpl;
	}
	else
	{
		// failure
	}

	tpl.RemoveAll();
}

// --------------------------------------------------------------------------
void ARAccent::browse(TimeUnwrap& mapper) const
{
	mapper.AtPos (this, TimeUnwrap::kAccent);
}

void ARAccent::print() const
{
}

void ARAccent::PrintName(std::ostream & os) const
{
	os << "\\accent";
	if (getRange()) os << "(";

}

void ARAccent::PrintParameters(std::ostream & ) const
{
}


