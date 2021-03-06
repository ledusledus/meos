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

// oControl.h: interface for the oControl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OCONTROL_H__E86192B9_78D2_4EEF_AAE1_3BD4A8EB16F0__INCLUDED_)
#define AFX_OCONTROL_H__E86192B9_78D2_4EEF_AAE1_3BD4A8EB16F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "xmlparser.h"
#include "oBase.h"
#include <limits>

class oControl;

typedef oControl* pControl;
class oDataInterface;
class oDataConstInterface;
class Table;

class oControl : public oBase
{
public:
  /** Returns the number of duplicates of this control in any course. Valid after a call to oEvent::getControls with
      the calculate flags set to true. */
  int getNumberDuplicates() const;

  void getCourseControls(vector<int> &cc) const;

  static pair<int, int> getIdIndexFromCourseControlId(int courseControlId);
  static int getCourseControlIdFromIdIndex(int controlId, int index);

  enum ControlStatus {StatusOK=0, StatusBad=1, StatusMultiple=2,
                      StatusStart = 4, StatusFinish = 5, StatusRogaining = 6, StatusNoTiming = 7, StatusOptional = 8};
  bool operator<(const oControl &b) const {return minNumber()<b.minNumber();}

protected:
  int nNumbers;
  int Numbers[32];
  bool checkedNumbers[32];

  ControlStatus Status;
  string Name;
  bool decodeNumbers(string s);

  static const int dataSize = 32;
  int getDISize() const {return dataSize;}
  BYTE oData[dataSize];
  BYTE oDataOld[dataSize];

  int tMissedTimeTotal;
  int tMissedTimeMax;
  int tNumVisitors;
  int tMissedTimeMedian;

  /// Table methods
  void addTableRow(Table &table) const;
  bool inputData(int id, const string &input,
                 int inputId, string &output, bool noUpdate);

  /// Table methods
  void fillInput(int id, vector< pair<string, size_t> > &elements, size_t &selected);

  /** Get internal data buffers for DI */
  oDataContainer &getDataBuffers(pvoid &data, pvoid &olddata, pvectorstr &strData) const;

  struct TCache {
    TCache() : minTime(0), timeAdjust(0), dataRevision(-1) {}
    int minTime;
    int timeAdjust;
    int dataRevision;
  };

  // Is set to true if there is a free punch tied to the control
  bool tHasFreePunchLabel;
  mutable int tNumberDuplicates;
  mutable TCache tCache;
  void setupCache() const;

  void changedObject();

public:

  // Returns true if controls is considered a radio control.
  bool isValidRadio() const;
  // Specify true to mark the controls as a radio control, otherwise no radio
  void setRadio(bool r);

  void remove();
  bool canRemove() const;

  string getInfo() const;

  bool isSingleStatusOK() const {return Status == StatusOK || Status == StatusNoTiming;}

  int getMissedTimeTotal() const {return tMissedTimeTotal;}
  int getMissedTimeMax() const {return tMissedTimeMax;}
  int getMissedTimeMedian() const {return tMissedTimeMedian;}
  int getNumVisitors() const {return tNumVisitors;}

  inline int minNumber() const {
    int m = numeric_limits<int>::max();
    for (int k=0;k<nNumbers;k++)
      m = min(Numbers[k], m);
    return m;
  }

  inline int maxNumber() const {
    int m = 0;
    for (int k=0;k<nNumbers;k++)
      m = max(Numbers[k], m);
    return m;
  }

  //Add unchecked controls to the list
  void addUncheckedPunches(vector<int> &mp, bool supportRogaining) const;
  //Start checking if all punches needed for this control exist
  void startCheckControl();
  //Get the number of a missing punch
  int getMissingNumber() const;
  /** Returns true if the check of this control is completed
   @param supportRogaining true if rogaining controls are supported
  */
  bool controlCompleted(bool supportRogaiing) const;

  string codeNumbers(char sep=';') const;
  bool setNumbers(const string &numbers);

  ControlStatus getStatus() const {return Status;}
  const string getStatusS() const;

  bool hasName() const {return !Name.empty();}
  string getName() const;

  bool isRogaining(bool useRogaining) const {return useRogaining && (Status == StatusRogaining);}

  void setStatus(ControlStatus st);
  void setName(string name);

  //Returns true if control has number and checks it.
  bool hasNumber(int i);
  //Return true if it has number i and it is unchecked.
  //Checks the number
  bool hasNumberUnchecked(int i);
  // Uncheck a given number
  bool uncheckNumber(int i);

  string getString();
  string getLongString();

  //For a control that requires several punches,
  //return the number of required punches.
  int getNumMulti();

  int getTimeAdjust() const;
  string getTimeAdjustS() const;
  void setTimeAdjust(int v);
  void setTimeAdjust(const string &t);

  int getMinTime() const;
  string getMinTimeS() const;
  void setMinTime(int v);
  void setMinTime(const string &t);

  int getRogainingPoints() const;
  string getRogainingPointsS() const;
  void setRogainingPoints(int v);
  void setRogainingPoints(const string &t);

  /// Return first code number (or zero)
  int getFirstNumber() const;
  void getNumbers(vector<int> &numbers) const;

  void set(const xmlobject *xo);
  void set(int pId, int pNumber, string pName);
  bool write(xmlparser &xml);
  oControl(oEvent *poe);
  oControl(oEvent *poe, int id);

  virtual ~oControl();

  friend class oRunner;
  friend class oCourse;
  friend class oEvent;
  friend class oClass;
  friend class MeosSQL;
  friend class TabAuto;
  friend class TabSpeaker;
};

#endif // !defined(AFX_OCONTROL_H__E86192B9_78D2_4EEF_AAE1_3BD4A8EB16F0__INCLUDED_)
