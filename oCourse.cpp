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

// oCourse.cpp: implementation of the oCourse class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "oCourse.h"
#include "oEvent.h"
#include "SportIdent.h"
#include <limits>
#include "Localizer.h"
#include "meos_util.h"
#include "meosexception.h"
#include <cassert>
#include "gdioutput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#include "intkeymapimpl.hpp"

oCourse::oCourse(oEvent *poe) : oBase(poe)
{
  getDI().initData();
  nControls=0;
  Length=0;
  clearCache();
  Id=oe->getFreeCourseId();
}

oCourse::oCourse(oEvent *poe, int id) : oBase(poe)
{
  getDI().initData();
  nControls=0;
  Length=0;
  clearCache();
  if (id == 0)
    id = oe->getFreeCourseId();
  Id=id;
  oe->qFreeCourseId = max(id, oe->qFreeCourseId);
}

oCourse::~oCourse()
{
}

string oCourse::getInfo() const
{
  return "Bana " + Name;
}

bool oCourse::Write(xmlparser &xml)
{
  if (Removed) return true;

  xml.startTag("Course");

  xml.write("Id", Id);
  xml.write("Updated", Modified.getStamp());
  xml.write("Name", Name);
  xml.write("Length", Length);
  xml.write("Controls", getControls());
  xml.write("Legs", getLegLengths());

  getDI().write(xml);
  xml.endTag();

  return true;
}

void oCourse::Set(const xmlobject *xo)
{
  xmlList xl;
  xo->getObjects(xl);

  xmlList::const_iterator it;

  for(it=xl.begin(); it != xl.end(); ++it){
    if (it->is("Id")){
      Id=it->getInt();
    }
    else if (it->is("Length")){
      Length=it->getInt();
    }
    else if (it->is("Name")){
      Name=it->get();
    }
    else if (it->is("Controls")){
      importControls(it->get());
    }
    else if (it->is("Legs")) {
      importLegLengths(it->get());
    }
    else if (it->is("oData")){
      getDI().set(*it);
    }
    else if (it->is("Updated")){
      Modified.setStamp(it->get());
    }
  }
}

string oCourse::getLegLengths() const
{
  string str;
  for(size_t m=0; m<legLengths.size(); m++){
    if (m>0)
      str += ";";
    str+=itos(legLengths[m]);
  }
  return str;
}

string oCourse::getControls() const
{
  string str="";
  char bf[16];
  for(int m=0;m<nControls;m++){
    sprintf_s(bf, 16, "%d;", Controls[m]->Id);
    str+=bf;
  }

  return str;
}

string oCourse::getControlsUI() const
{
  string str="";
  char bf[16];
  int m;

  for(m=0;m<nControls-1;m++){
    sprintf_s(bf, 16, "%d, ", Controls[m]->Id);
    str+=bf;
  }

  if (m<nControls){
    sprintf_s(bf, 16, "%d", Controls[m]->Id);
    str+=bf;
  }

  return str;
}

vector<string> oCourse::getCourseReadable(int limit) const
{
  vector<string> res;

  string str="S-";
  int m;

  vector<pControl> rg;
  bool needFinish = false;
  bool rogaining = hasRogaining();
  for (m=0; m<nControls; m++) {
    if (Controls[m]->isRogaining(rogaining))
      rg.push_back(Controls[m]);
    else {
      str += Controls[m]->getLongString() + "-";
      needFinish = true;
    }
    if (str.length() >= size_t(limit)) {
      res.push_back(str);
      str.clear();
    }
  }

  if (needFinish)
    str += "M";

  if (!str.empty()) {
    if (str.length()<5 && !res.empty())
      res.back().append(str);
    else
      res.push_back(str);
    str.clear();
  }

  if (!rg.empty()) {
    str = lang.tl("Rogaining: ");
    for (size_t k = 0; k<rg.size(); k++) {
      if (k>0)
        str += ", ";

      if (str.length() >= size_t(limit)) {
        res.push_back(str);
        str.clear();
      }
      str += rg[k]->getLongString();
    }
    if (!str.empty()) {
      res.push_back(str);
    }
  }

  return res;
}

pControl oCourse::addControl(int Id)
{
  pControl pc = doAddControl(Id);
  updateChanged();
  return pc;
}

pControl oCourse::doAddControl(int Id)
{
  if (nControls<NControlsMax) {
    pControl c=oe->getControl(Id, true);
    if (c==0)
      throw meosException("Felaktig kontroll");
    Controls[nControls++]=c;
    return c;
  }
  else
    throw meosException("För många kontroller.");
}

void oCourse::splitControls(const string &ctrls, vector<int> &nr) {
  const char *str=ctrls.c_str();

  nr.clear();

  while (*str) {
    int cid=atoi(str);

    while(*str && (*str!=';' && *str!=',' && *str!=' ')) str++;
    while(*str && (*str==';' || *str==',' || *str==' ')) str++;

    if (cid>0)
      nr.push_back(cid);
  }
}

void oCourse::importControls(const string &ctrls) {
  int oldNC = nControls;
  vector<int> oldC;
  for (int k = 0; k<nControls; k++)
    oldC.push_back(Controls[k] ? Controls[k]->getId() : 0);

  nControls = 0;

  vector<int> newC;
  splitControls(ctrls, newC);

  for (size_t k = 0; k< newC.size(); k++)
    doAddControl(newC[k]);

  bool changed = nControls != oldNC;

  for (int k = 0; !changed && k<nControls; k++)
    changed |= oldC[k] != Controls[k]->getId();

  if (changed) {
    updateChanged();

    oe->punchIndex.clear();
  }
}

void oCourse::importLegLengths(const string &legs)
{
  vector<string> splits;
  split(legs, ";", splits);

  legLengths.resize(splits.size());
  for (size_t k = 0; k<legLengths.size(); k++)
    legLengths[k] = atoi(splits[k].c_str());
}

oControl *oCourse::getControl(int index) const
{
  if (index>=0 && index<nControls)
    return Controls[index];
  else return 0;
}

bool oCourse::fillCourse(gdioutput &gdi, const string &name)
{
  int finishIx = useLastAsFinish() ? nControls - 1 : -1;
  int startIx = useFirstAsStart() ? 0 : -1;

  oPunchList::iterator it;
  bool rogaining = hasRogaining();
  gdi.clearList(name);
  int offset = 1;
  if (startIx == -1)
    gdi.addItem(name, lang.tl("Start"), -1);
  for (int k=0;k<nControls;k++) {
    string c = Controls[k]->getString();
    if (c.length() > 32)
      c= c.substr(0, 32) + "...";
    if (k == startIx)
      c += " (" + lang.tl("Start") + ")";
    else if (k == finishIx)
      c += " (" + lang.tl("Mål") + ")";

    int multi = Controls[k]->getNumMulti();
    int submulti = 0;
    char bf[256];
    if (Controls[k]->isRogaining(rogaining)) {
      sprintf_s(bf, 64, "R\t%s", c.c_str());
      offset--;
    }
    else if (multi == 1) {
      sprintf_s(bf, 64, "%d\t%s", k+offset, c.c_str());
    }
    else
      sprintf_s(bf, 64, "%d%c\t%s", k+offset, 'A', c.c_str());
    gdi.addItem(name, bf, k);
    while (multi>1) {
      submulti++;
      sprintf_s(bf, 64, "%d%c\t-:-", k+offset, 'A'+submulti);
      gdi.addItem(name, bf, -1);
      multi--;
    }
    offset += submulti;
  }
  if (finishIx == -1)
    gdi.addItem(name, lang.tl("Mål"), -1);

  return true;
}

void oCourse::getControls(vector<pControl> &pc)
{
  pc.clear();
  pc.reserve(nControls);
  for(int k=0;k<nControls;k++){
    pc.push_back(Controls[k]);
  }
}

int oCourse::distance(const SICard &card)
{
  int matches=0;

  set<int> rogaining;
  vector< map<int, int> > allowedControls;
  allowedControls.reserve(nControls);
  set<int> commonCode;
  if (hasRogaining()) {
    for (int k=0;k<nControls;k++) {
      if (Controls[k]->isRogaining(true)) {
        for (int j = 0; j < Controls[k]->nNumbers; j++)
          rogaining.insert(Controls[k]->Numbers[j]);
      }
    }
  }

  int toMatch = 0;
  size_t orderIndex = 0;
  for (int k=0;k<nControls;k++) {
    if (Controls[k]->isRogaining(hasRogaining()) || 
        Controls[k]->getStatus() == oControl::StatusBad || 
        Controls[k]->getStatus() == oControl::StatusOptional)
      continue;

    if (Controls[k]->getStatus() == oControl::StatusMultiple) {
      for (int j = 0; j < Controls[k]->nNumbers; j++) {
        if (allowedControls.size() <= orderIndex)
          allowedControls.resize(orderIndex+1);
        for (int i = 0; i < Controls[k]->nNumbers; i++) {
          ++allowedControls[orderIndex][Controls[k]->Numbers[i]];
        }
        orderIndex++;
        toMatch++;
      }
    }
    else {
      if (allowedControls.size() <= orderIndex)
        allowedControls.resize(orderIndex+1);

      for (int j = 0; j < Controls[k]->nNumbers; j++) {
        ++allowedControls[orderIndex][Controls[k]->Numbers[j]];
      }
      orderIndex++;
      toMatch++;
    }

    if (getCommonControl() == Controls[k]->getId()) {
      orderIndex = 0;
      commonCode.insert(Controls[k]->Numbers, Controls[k]->Numbers+Controls[k]->nNumbers);
    }
  }

  size_t matchIndex = 0;
  for (unsigned k=0; k<card.nPunch && matches < toMatch; k++) {
    for (unsigned j = k; j < card.nPunch; j++) {
      if (matchIndex < allowedControls.size() &&
             allowedControls[matchIndex].count(card.Punch[j].Code) &&
             allowedControls[matchIndex][card.Punch[j].Code] > 0) {
        --allowedControls[matchIndex][card.Punch[j].Code];
        k = j;
        matches++;
        break;
      }
    }
    matchIndex++;
    if (commonCode.count(card.Punch[k].Code))
      matchIndex = 0;
  }

  if (matches==toMatch) {
    //This course is OK. Extra controls?
    return card.nPunch-toMatch; //Positive return
  }
  else {
    return matches-toMatch; //Negative return;
  }
  return 0;
}

string oCourse::getLengthS() const
{
  return itos(getLength());
}

void oCourse::setName(const string &n)
{
  if (Name!=n){
    Name=n;
    updateChanged();
  }
}

void oCourse::setLength(int le)
{
  if (le<0 || le > 1000000)
    le = 0;

  if (Length!=le){
    Length=le;
    updateChanged();
  }
}

oDataContainer &oCourse::getDataBuffers(pvoid &data, pvoid &olddata, pvectorstr &strData) const {
  data = (pvoid)oData;
  olddata = (pvoid)oDataOld;
  strData = 0;
  return *oe->oCourseData;
}

pCourse oEvent::getCourseCreate(int Id)
{
  oCourseList::iterator it;
  for (it=Courses.begin(); it != Courses.end(); ++it) {
    if (it->Id==Id)
      return &*it;
  }
  if (Id>0) {
    oCourse c(this, Id);
    c.setName(getAutoCourseName());
    return addCourse(c);
  }
  else {
    return addCourse(getAutoCourseName());
  }
}

pCourse oEvent::getCourse(int Id) const
{
  if (Id==0)
    return 0;

  pCourse value;
  if (courseIdIndex.lookup(Id,value))
    return value;

  return 0;
}

pCourse oEvent::getCourse(const string &n) const
{
  oCourseList::const_iterator it;

  for (it=Courses.begin(); it != Courses.end(); ++it) {
    if (it->Name==n)
      return pCourse(&*it);
  }
  return 0;
}

void oEvent::fillCourses(gdioutput &gdi, const string &id, bool simple)
{
  vector< pair<string, size_t> > d;
  oe->fillCourses(d, simple);
  gdi.addItem(id, d);
}

const vector< pair<string, size_t> > &oEvent::fillCourses(vector< pair<string, size_t> > &out, bool simple)
{
  out.clear();
  oCourseList::iterator it;
  synchronizeList(oLCourseId);

  Courses.sort();
  string b;
  for (it=Courses.begin(); it != Courses.end(); ++it) {
    if (!it->Removed){

      if (simple) //gdi.addItem(name, it->Name, it->Id);
        out.push_back(make_pair(it->Name, it->Id));
      else {
        b = it->Name + "\t(" + itos(it->nControls) + ")";
        //sprintf_s(bf, 16, "\t(%d)", it->nControls);
        //b+=bf;
        if (!it->getCourseProblems().empty())
          b = "[!] " + b;
        //gdi.addItem(name, b, it->Id);
        out.push_back(make_pair(b, it->Id));
      }
    }
  }
  return out;
}

void oCourse::setNumberMaps(int block)
{
  getDI().setInt("NumberMaps", block);
}

int oCourse::getNumberMaps() const
{
  return getDCI().getInt("NumberMaps");
}

void oCourse::setStart(const string &start, bool sync)
{
  if (getDI().setString("StartName", start)) {
    if (sync)
      synchronize();
    oClassList::iterator it;
    for (it=oe->Classes.begin();it!=oe->Classes.end();++it) {
      if (it->getCourse()==this) {
        it->setStart(start);
        if (sync)
          it->synchronize();
      }
    }
  }
}

string oCourse::getStart() const
{
  return getDCI().getString("StartName");
}

void oEvent::calculateNumRemainingMaps()
{
  synchronizeList(oLCourseId, true, false);
  synchronizeList(oLRunnerId, false, true);

  for (oCourseList::iterator cit=Courses.begin();
    cit!=Courses.end();++cit) {
    int numMaps = cit->getNumberMaps();
    if (numMaps == 0)
      cit->tMapsRemaining = numeric_limits<int>::min();
    else
      cit->tMapsRemaining = numMaps;
  }

  list<oRunner>::const_iterator it;
  for (it=Runners.begin(); it != Runners.end(); ++it)
    if (!it->isRemoved() && it->getStatus()!=StatusDNS){
      pCourse pc = it->getCourse(false);

      if (pc && pc->tMapsRemaining != numeric_limits<int>::min())
        pc->tMapsRemaining--;
    }
}

int oCourse::getIdSum(int nC) {

  int id = 0;
  for (int k = 0; k<min(nC, nControls); k++)
    id = 31 * id + (Controls[k] ? Controls[k]->getId() : 0);

  if (id == 0)
    return getId();

  return id;
}

void oCourse::setLegLengths(const vector<int> &legs) {
  if (legs.size() == nControls +1 || legs.empty()) {
    bool diff = legs.size() != legLengths.size();
    if (!diff) {
      for (size_t k = 0; k<legs.size(); k++)
        if (legs[k] != legLengths[k])
          diff = true;
    }
    if (diff) {
      updateChanged();
      legLengths = legs;
    }
  }
  else
    throw std::exception("Invalid parameter value");
}

double oCourse::getPartOfCourse(int start, int end) const
{
  if (end == 0)
    end = nControls;

  if (legLengths.size() != nControls +1 || start <= end ||
      unsigned(start) >= legLengths.size() ||
      unsigned(end) >= legLengths.size() || Length==0)
    return 0.0;

  int dist = 0;
  for (int k = start; k<end; k++)
    dist += legLengths[k];

  return max(1.0, double(dist) / double(Length));
}


const string &oCourse::getControlOrdinal(int controlIndex) const
{
  if ( (controlIndex + 1 == nControls && useLastAsFinish())  || controlIndex == nControls)
    return lang.tl("Mål");

  if (oe->dataRevision != cacheDataRevision)
    clearCache();

  if (size_t(controlIndex) < cachedControlOrdinal.size() && !cachedControlOrdinal[controlIndex].empty())
    return cachedControlOrdinal[controlIndex];

  if (controlIndex > nControls)
    throw meosException("Invalid index");
  cachedControlOrdinal.resize(nControls);

  int o = useFirstAsStart() ? 0 : 1;
  bool rogaining = hasRogaining();

  for (int k = 0; k<controlIndex && k<nControls; k++) {
    if (Controls[k] && !Controls[k]->isRogaining(rogaining))
      o++;
  }
  cachedControlOrdinal[controlIndex] = itos(o);
  return cachedControlOrdinal[controlIndex];
}

void oCourse::setRogainingPointsPerMinute(int p)
{
  getDI().setInt("RReduction", p);
}

int oCourse::getRogainingPointsPerMinute() const
{
  return getDCI().getInt("RReduction");
}

int oCourse::calculateReduction(int overTime) const
{
  int reduction = 0;
  if (overTime > 0) {
    int method = getDCI().getInt("RReductionMethod");
    if (method == 0) // Linear model
      reduction = (59 + overTime * getRogainingPointsPerMinute()) / 60;
    else // Time (minute) discrete model
      reduction = ((59 + overTime) / 60) * getRogainingPointsPerMinute();
  }
  return reduction;
}


void oCourse::setMinimumRogainingPoints(int p)
{
  cachedHasRogaining = 0;
  getDI().setInt("RPointLimit", p);
}

int oCourse::getMinimumRogainingPoints() const
{
  return getDCI().getInt("RPointLimit");
}

void oCourse::setMaximumRogainingTime(int p)
{
  cachedHasRogaining = 0;
  if (p == NOTIME)
    p = 0;
  getDI().setInt("RTimeLimit", p);
}

int oCourse::getMaximumRogainingTime() const
{
  return getDCI().getInt("RTimeLimit");
}

bool oCourse::hasRogaining() const {
  if (oe->dataRevision != cacheDataRevision)
    clearCache();

  if (cachedHasRogaining>0)
    return cachedHasRogaining == 2;

  bool r = getMaximumRogainingTime() > 0 || getMinimumRogainingPoints() > 0;
  cachedHasRogaining = r ? 2 : 1;
  return r;
}

void oCourse::clearCache() const {
  cachedHasRogaining = 0;
  cachedControlOrdinal.clear();
  cacheDataRevision = oe->dataRevision;
}

string oCourse::getCourseProblems() const
{
  int max_time = getMaximumRogainingTime();
  int min_point = getMinimumRogainingPoints();

  if (max_time > 0) {
    for (int k = 0; k<nControls; k++) {
      if (Controls[k]->isRogaining(true))
        return "";
    }
    return "Banan saknar rogainingkontroller.";
  }
  else if (min_point > 0) {
    int max_p = 0;
    for (int k = 0; k<nControls; k++) {
      if (Controls[k]->isRogaining(true))
        max_p += Controls[k]->getRogainingPoints();
    }

    if (max_p < min_point) {
      return "Banans kontroller ger för få poäng för att täcka poängkravet.";
    }
  }
  return "";
}

void oCourse::remove()
{
  if (oe)
    oe->removeCourse(Id);
}

bool oCourse::canRemove() const
{
  return !oe->isCourseUsed(Id);
}

void oCourse::changeId(int newId) {
  pCourse old = oe->courseIdIndex[Id];
  if (old == this)
    oe->courseIdIndex.remove(Id);

  oBase::changeId(newId);

  oe->courseIdIndex[newId] = this;
}

bool oCourse::useFirstAsStart() const {
  return getDCI().getInt("FirstAsStart") != 0;
}

bool oCourse::useLastAsFinish() const {
  return getDCI().getInt("LastAsFinish") != 0;
}

void oCourse::firstAsStart(bool f) {
  getDI().setInt("FirstAsStart", f ? 1:0);
}

void oCourse::lastAsFinish(bool f) {
  getDI().setInt("LastAsFinish", f ? 1:0);
}

int oCourse::getFinishPunchType() const {
  if (useLastAsFinish() && nControls > 0)
    return Controls[nControls - 1]->Numbers[0];
  else
    return oPunch::PunchFinish;
}

int oCourse::getStartPunchType() const {
  if (useFirstAsStart() && nControls > 0)
    return Controls[0]->Numbers[0];
  else
    return oPunch::PunchStart;
}

void oEvent::getCourses(vector<pCourse> &crs) const{
  crs.clear();
  for (oCourseList::const_iterator it = Courses.begin(); it != Courses.end(); ++it) {
    if (it->isRemoved())
      continue;
    crs.push_back(pCourse(&*it));
  }
}

int oCourse::getCommonControl() const {
  return getDCI().getInt("CControl");
}

void oCourse::setCommonControl(int ctrlId) {
  if (ctrlId != 0) {
    int found = 0;
    for (int k = 0; k < nControls; k++) {
      if (Controls[k]->getId() == ctrlId)
        found++;
    }
    if (found == 0)
      throw meosException("Kontroll X finns inte på banan#" + itos(ctrlId));
  }
  getDI().setInt("CControl", ctrlId);
}

pCourse oCourse::getAdapetedCourse(const oCard &card, oCourse &tmpCourse) const {
  /*adaptedToOriginalCardOrder.resize(nControls + 1);
  for (int k = 0; k < nControls + 1; k++)
    adaptedToOriginalCardOrder[k] = k;*/
  int cc = getCommonControl();
  if (cc == 0)
    return pCourse(this);

  vector<int> ccIndex;
  vector< vector<pControl> > loopKeys;
  if (!constructLoopKeys(cc, loopKeys, ccIndex))
    return pCourse(this);

  vector< vector<int> > punchSequence;

  vector<pPunch> punches;
  card.getPunches(punches);

  punchSequence.push_back(vector<int>());
  for (size_t k = 0; k < punches.size(); k++) {
    int code = punches[k]->getTypeCode();
    if (code < 10)
      continue; // Start, Finish etc.
    if (code == cc && !punchSequence.back().empty())
      punchSequence.push_back(vector<int>());
    else
      punchSequence.back().push_back(code);
  }

  vector<int> assignedKeys(loopKeys.size(), -1);

  for (size_t k = 0; k < punchSequence.size(); k++) {
    bool done = false;
    for (size_t j = 0; j < loopKeys.size(); j++) {
      if (assignedKeys[j] == -1 && matchLoopKey(punchSequence[k], loopKeys[j])) {
        assignedKeys[j] = k;
        done = true;
        break;
      }
    }
    if (!done) {
      // Check if an already assigned key matches the current loop
      int undone = -1;
      size_t start = loopKeys.size();
      for (size_t j = 0; j < loopKeys.size(); j++) {
        if (matchLoopKey(punchSequence[k], loopKeys[j])) {
          // Push that key away and try it somewhere else
          undone = assignedKeys[j];
          start = j + 1;
          assignedKeys[j] = k;
          done = true;
          break;
        }
      }

      // Try an already assigned key later
      for (size_t j = start; undone != -1 && j < loopKeys.size(); j++) {
         if (matchLoopKey(punchSequence[undone], loopKeys[j])) {
           swap(assignedKeys[j], undone);
         }
      }
    }
  }

  //set<int> loops;
  //for (size_t k = 0; k < ccIndex.size(); k++)
  //  loops.insert(k);
  vector<int> loopOrder;
  map<int, int> keyToIndex;
  assert(ccIndex.size() == assignedKeys.size());
  for (size_t k = 0; k < assignedKeys.size(); k++) {
    if (assignedKeys[k] != -1) {
      keyToIndex[assignedKeys[k]] = k;
    }
  }

  for (map<int, int>::iterator it = keyToIndex.begin(); it != keyToIndex.end(); ++it) {
    loopOrder.push_back(it->second);
  }

  int checksum = (ccIndex.size() * (ccIndex.size()-1))/2;

  // Add remaining, unmatched, loops in defined order
  for (size_t k = 0; k < ccIndex.size(); k++) {
    if (assignedKeys[k] == -1)
      loopOrder.push_back(k);
    checksum-=loopOrder[k];
  }
  assert(checksum == 0 && loopOrder.size() == ccIndex.size());

  tmpCourse.cacheDataRevision = cacheDataRevision;
  tmpCourse.cachedControlOrdinal.clear();
  tmpCourse.cachedHasRogaining = cachedHasRogaining;
  memcpy(tmpCourse.oData, oData, sizeof(oData));
  tmpCourse.nControls = 0;
  tmpCourse.Length = Length;
  tmpCourse.Name = Name;
  tmpCourse.sqlUpdated = "TMP"; // Mark as tmp to prevent accidental write to DB
  tmpCourse.tMapToOriginalOrder.clear();
  tmpCourse.tMapToOriginalOrder.reserve(nControls+1);

  if (useFirstAsStart()) {
    tmpCourse.tMapToOriginalOrder.push_back(0);
    tmpCourse.Controls[tmpCourse.nControls++] = Controls[0];
    if (0 < legLengths.size())
      tmpCourse.legLengths.push_back(legLengths[0]);
  }

  int endIx = useLastAsFinish() ? nControls - 1 : nControls;

  for (size_t k = 0; k< loopOrder.size(); k++) {
    int start = ccIndex[loopOrder[k]];
    int end = size_t(loopOrder[k] + 1) < ccIndex.size() ? ccIndex[loopOrder[k] + 1] : endIx;
    for (int i = start + 1; i < end; i++) {
      tmpCourse.tMapToOriginalOrder.push_back(i);
      tmpCourse.Controls[tmpCourse.nControls++] = Controls[i];
      if (size_t(i) < legLengths.size())
        tmpCourse.legLengths.push_back(legLengths[i]);
    }
    tmpCourse.tMapToOriginalOrder.push_back(end);
    if (k + 1 < loopOrder.size()) {
      int currentCC = ccIndex[k+1];
      //tmpCourse.tMapToOriginalOrder.push_back(currentCC);
      tmpCourse.Controls[tmpCourse.nControls++] = Controls[currentCC];
      if (size_t(end) < legLengths.size())
        tmpCourse.legLengths.push_back(legLengths[end]);
    }
  }

  if (useLastAsFinish()) {
    //tmpCourse.tMapToOriginalOrder.push_back(nControls - 1);
    tmpCourse.tMapToOriginalOrder.push_back(nControls);
    tmpCourse.Controls[tmpCourse.nControls++] = Controls[nControls - 1];
    if (size_t(nControls-1) < legLengths.size())
      tmpCourse.legLengths.push_back(legLengths[nControls-1]);
  }

  //tmpCourse.tMapToOriginalOrder.push_back(nControls);

  assert(tmpCourse.nControls == nControls);
  assert(tmpCourse.tMapToOriginalOrder.size() == nControls + 1);

  if (!legLengths.empty()) {
    tmpCourse.legLengths.push_back(legLengths.back());
    assert(tmpCourse.legLengths.size() == legLengths.size());
  }
  tmpCourse.Id = Id;
  return &tmpCourse;
}

bool oCourse::isAdapted() const {
  return tMapToOriginalOrder.size() > 0;
}

int oCourse::getAdaptionId() const {
  int key = 0;
  for (size_t j = 0; j < tMapToOriginalOrder.size(); j++)
    key = key * 97 + tMapToOriginalOrder[j];
  return key;
}

bool oCourse::matchLoopKey(const vector<int> &punches, const vector<pControl> &key) {
  size_t ix = -1;
  for (size_t k = 0; k < key.size(); k++) {
    int code = key[k]->getFirstNumber();
    while (++ix < punches.size()) {
      if (punches[ix] == code) {
        code = -1;
        break;
      }
    }
    if (code != -1)
      return false;
  }
  return true;
}

bool oCourse::constructLoopKeys(int cc, vector< vector<pControl> > &loopKeys, vector<int> &ccIndex) const {
  int startIx = useFirstAsStart() ? 1 : 0;
  int endIx = useLastAsFinish() ? nControls : nControls-1;

  ccIndex.push_back(startIx-1);
  for (int k = startIx; k < endIx; k++) {
    if (Controls[k]->getId() == cc)
      ccIndex.push_back(k);
  }
  if (ccIndex.size() <= 1)
    return false;

  loopKeys.clear();
  loopKeys.resize(ccIndex.size());

  int keyIndex = 1;
  bool changed = true;
  bool enough = false;
  while(changed && !enough) {
    changed = false;
    for (size_t k = 0; k < ccIndex.size(); k++) {
      int keyIx = ccIndex[k] + keyIndex;
      int nextIx = (k + 1) < ccIndex.size() ? ccIndex[k+1] : nControls;
      if (keyIx < nextIx && Controls[keyIx]->isSingleStatusOK() && Controls[keyIx]->nNumbers == 1) {
        loopKeys[k].push_back(Controls[keyIx]);
        changed = true;
      }
    }
    keyIndex++;
    if (changed) {
      enough = false;
      set<__int64> hashes;
      for (size_t k = 0; k < loopKeys.size(); k++) {
        __int64 h = loopKeys[k].size();
        for (size_t j = 0; j < loopKeys[k].size(); j++) {
          h = h * 997 + loopKeys[k][j]->Numbers[0];
        }
        hashes.insert(h);
      }
      enough = hashes.size() == loopKeys.size();
    }
  }

  return enough;
}

void oCourse::changedObject() {
  if (oe)
    oe->globalModification = true;
}

int oCourse::getCourseControlId(int controlIx) const {
  if (controlIx >= nControls) {
    assert(false);
    return -1;
  }

  int id = Controls[controlIx] ? Controls[controlIx]->getId() : 0;
  if (id == 0)
    return 0;

  int count = 0;
  for (int j = 0; j < controlIx; j++) {
    if (Controls[j] && Controls[j]->Id == id)
      count++;
  }

  return oControl::getCourseControlIdFromIdIndex(id, count);
}

string oCourse::getRadioName(int courseControlId) const {
  pair<int,int> idix = oControl::getIdIndexFromCourseControlId(courseControlId);
  pControl pc = 0;
  int numRadio = 0;
  int clsix = 1;
  for (int k = 0; k < nControls; k++) {
    if (Controls[k]) {
      if (Controls[k]->isValidRadio())
        numRadio++;

      if (Controls[k]->Id == idix.first) {
        if (idix.second == 0) {
          pc = Controls[k];
          break;
        }
        else {
          clsix++;
          idix.second--;
        }
      }
    }
  }

  if (pc == 0)
    return "?";

  string name;
  if (pc->hasName()) {
    name = pc->getName();
    if (pc->getNumberDuplicates() > 1)
      name += MakeDash("-" + itos(clsix));
  }
  else {
    name = lang.tl("radio X#" + itos(numRadio));
    capitalize(name);
  }

  return name;
}
