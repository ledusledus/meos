/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2015 Melin Software HB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden

************************************************************************/

// oBase.cpp: implementation of the oBase class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "meos.h"
#include "oBase.h"
#include "oCard.h"

#include "oEvent.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

oBase::oBase(oEvent *poe)
{
  Removed = false;
  oe = poe;
  Id = 0;
  changed = false;
  reChanged = false;
  counter = 0;
  Modified.update();
  correctionNeeded = true;
}

oBase::~oBase()
{
}

bool oBase::synchronize(bool writeOnly)
{
  if (oe && changed) {
    changedObject();
    oe->dataRevision++;
  }
  if (oe && oe->HasDBConnection && (changed || !writeOnly)) {
    correctionNeeded = false;
    return oe->msSynchronize(this);
  }
  else {
    if (changed) {
      if (!oe->HasPendingDBConnection) // True if we are trying to reconnect to mysql
        changed = false;
    }
  }
  return true;
}

void oBase::clearCombo(HWND hWnd)
{
  SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
}

void oBase::addToCombo(HWND hWnd, const string &str, int data)
{
  int index=SendMessage(hWnd, CB_ADDSTRING, 0, LPARAM(str.c_str()));
  SendMessage(hWnd, CB_SETITEMDATA, index, data);
}

void oBase::setExtIdentifier(__int64 id)
{
  getDI().setInt64("ExtId", id);
}

__int64 oBase::getExtIdentifier() const
{
  return getDCI().getInt64("ExtId");
}

string oBase::getTimeStamp() const {
  if (oe && oe->isClient() && !sqlUpdated.empty())
    return sqlUpdated;
  else return Modified.getStampString();
}

void oBase::changeId(int newId) {
  Id = newId;
}

oDataInterface oBase::getDI(void) {
  pvoid data;
  pvoid olddata;
  pvectorstr strData;
  oDataContainer &dc = getDataBuffers(data, olddata, strData);
  return dc.getInterface(data, getDISize(), this);
}

oDataConstInterface oBase::getDCI(void) const
{
  pvoid data;
  pvoid olddata;
  pvectorstr strData;
  oDataContainer &dc = getDataBuffers(data, olddata, strData);
  return dc.getConstInterface(data, getDISize(), this);
}

void oBase::updateChanged() {
  Modified.update();
  changed=true;
}
