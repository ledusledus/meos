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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden

************************************************************************/
#include "stdafx.h"

#include <algorithm>

#include "methodeditor.h"
#include "generalresult.h"
#include "gdioutput.h"
#include "meosexception.h"
#include "gdistructures.h"
#include "meos_util.h"
#include "localizer.h"
#include "gdifonts.h"
#include "oEvent.h"
#include "tabbase.h"
#include "CommDlg.h"
#include "random.h"
#include "metalist.h"

MethodEditor::MethodEditor(oEvent *oe_) {
  oe = oe_;
  inputNumber = 0;
  currentResult = 0;
  currentIndex = -1;
  dirtyInt = false;
  wasLoadedBuiltIn = false;
}

MethodEditor::~MethodEditor() {
  setCurrentResult(0, "");
}

void MethodEditor::setCurrentResult(DynamicResult *lst, const string &fileSrc) {
  delete currentResult;
  currentResult = lst;
  fileNameSource = fileSrc;
}

int methodCB(gdioutput *gdi, int type, void *data) {
  void *clz = gdi->getData("MethodEditorClz");
  MethodEditor *le = (MethodEditor *)clz;
  BaseInfo *bi = (BaseInfo *)data;
  if (le)
    return le->methodCb(*gdi, type, *bi);

  throw meosException("Unexpected error");
}

void MethodEditor::show(gdioutput &gdi) {
  oe->loadGeneralResults(true);

  gdi.setRestorePoint("BeginMethodEdit");

  gdi.pushX();
  gdi.setCX(gdi.getCX() + gdi.scaleLength(6));
  if (currentResult)
    gdi.addString("", boldLarge, MakeDash("Result Module - X#") + currentResult->getName(true));
  else
    gdi.addString("", boldLarge, "Edit Result Modules");

  gdi.setOnClearCb(methodCB);
  gdi.setData("MethodEditorClz", this);
  gdi.dropLine(0.5);
  gdi.fillRight();

  gdi.addButton("OpenFile", "Importera fr�n fil", methodCB);
  gdi.addButton("OpenInside", "�ppna", methodCB);

  if (currentResult) {
    gdi.addButton("SaveFile", "Exportera till fil...", methodCB);
    gdi.addButton("SaveInside", "Spara", methodCB);
    gdi.addButton("Remove", "Ta bort", methodCB);

    makeDirty(gdi, NoTouch);
  }

  gdi.addButton("NewRules", "New Result Module", methodCB);
  gdi.addButton("Close", "St�ng", methodCB);

#ifdef _DEBUG
  gdi.addButton("WriteDoc", "#WriteDoc", methodCB);
#endif
  gdi.popX();
  gdi.dropLine(2);
  gdi.fillDown();
  
  if (currentResult) {
    gdi.dropLine(0.5);
    if (currentResult->getTag().empty())
      currentResult->setTag(uniqueTag("result"));

    gdi.addInput("Name", currentResult->getName(false), 20, methodCB, "Name of result module:");
    
    string tag = currentResult->getTag();
    vector<int> listIx;
    oe->getListContainer().getListsByResultModule(tag, listIx);
    
    string udtag = DynamicResult::undecorateTag(tag);
    gdi.addInput("Tag", udtag, 20, methodCB, "Result module identifier:");
    if (!listIx.empty()) {
      gdi.disableInput("Tag");
      gdi.getBaseInfo("Tag").setExtra(1);
      string lists = oe->getListContainer().getList(listIx.front()).getListName();
      if (listIx.size() > 1)
        lists += ", ...";
      gdi.addString("", 0, "Resultatmodulen anv�nds i X.#" + lists);
    }

    string desc = currentResult->getDescription();
    if (wasLoadedBuiltIn)
      desc = lang.tl(desc);

    gdi.addInputBox("Desc", 300, 70, desc, methodCB, "Description:");

    gdi.dropLine();
    gdi.fillRight();
    gdi.addSelection("Method", 200, 200, methodCB, "Edit rule for:");
    vector< pair<DynamicResult::DynamicMethods, string> > mt;
    currentResult->getMethodTypes(mt);

    for (size_t k = 0; k < mt.size(); k++)
      gdi.addItem("Method", lang.tl(mt[k].second), mt[k].first);

    gdi.dropLine();
    gdi.addButton("SaveSource", "Save changes", methodCB);
    gdi.addButton("CancelSource", "Cancel", methodCB);
    gdi.addButton("TestRule", "Test Result Module", methodCB);
    gdi.disableInput("SaveSource");
    gdi.disableInput("CancelSource");
    gdi.dropLine(2);
    gdi.popX();
    gdi.fillDown();

    gdi.setRestorePoint("TestRule");
  }

  gdi.refresh();
}

bool MethodEditor::checkTag(const string &tag, bool throwError) const {
  vector< pair<string, string> > tagNameList;
  oe->getGeneralResults(false, tagNameList, false);
  for (size_t k = 0; k < tag.length(); k++) {
    char c = tag[k];
    if (c == '-' || c == '_' || c == '%')
      continue;
    if (!isalnum(c)) {
      if (throwError)
        throw meosException("Invalid character in tag X.#" + tag);
      else
        return false;
    }
  }
  if (tag.empty()) {
    if (throwError)
      throw meosException("Empty tag is invalid");
    else
      return false;
  }
  for (size_t k = 0; k < tagNameList.size(); k++) {
    if (_strcmpi(tagNameList[k].first.c_str(), tag.c_str()) == 0) {
      if (throwError)
        throw meosException("The tag X is in use.#" + tag);
      else
        return false;
    }
  }
  return true;
}

string MethodEditor::uniqueTag(const string &tag) const {
  vector< pair<string, string> > tagNameList;
  oe->getGeneralResults(false, tagNameList, false);
  set<string> tags;
  for (size_t k = 0; k < tagNameList.size(); k++)
    tags.insert(tagNameList[k].first);

  int a = GetRandomNumber(65536);
  srand(GetTickCount());
  int b = rand() % 65536;
  char un[64];
  sprintf_s(un, "-%04X-%04X-", a, b);
  int iter = 0;
  while (tags.count(tag + un + itos(++iter)));
  return tag + un + itos(iter);
}

int MethodEditor::methodCb(gdioutput &gdi, int type, BaseInfo &data) {
  if (type == GUI_BUTTON) {
    ButtonInfo bi = dynamic_cast<ButtonInfo &>(data);

    if (bi.id == "Cancel") {
      gdi.restore("EditList");
      gdi.enableInput("EditList");
    }
    else if (bi.id == "CloseWindow") {
      gdi.closeWindow();
    }
    else if (bi.id == "WriteDoc") {
      ofstream fout("methoddoc.txt");
      DynamicResult dr;
      dr.declareSymbols(DynamicResult::MRScore, true);
      vector< pair<string, size_t> > symbs;
      dr.getSymbols(symbs);
      fout << "#head{" << lang.tl("Deltagare") << "}\n#table{2}" << endl;
      for (size_t k = 0; k < symbs.size(); k++) {
        string name, desc;
        dr.getSymbolInfo(symbs[k].second, name, desc);
        fout << "{#mono{" << name << "}}{" << lang.tl(desc) << "}" << endl;
      }

      dr.declareSymbols(DynamicResult::MTScore, true);
      dr.getSymbols(symbs);
      fout << "#head{" << lang.tl("Lag") << "}\n#table{2}" << endl;
      for (size_t k = 0; k < symbs.size(); k++) {
        string name, desc;
        dr.getSymbolInfo(symbs[k].second, name, desc);
        fout << "{#mono{" << name << "}}{" << lang.tl(desc) << "}" << endl;
      }
    }
    else if (bi.id == "NewRules") {
      if (!checkSave(gdi))
        return 0;
      gdi.clearPage(false);
      gdi.setData("MethodEditorClz", this);
      gdi.addString("", boldLarge, "New Set of Result Rules");

      setCurrentResult(new DynamicResult(), "");
      wasLoadedBuiltIn = false;
      currentResult->setName(lang.tl("Result Calculation"));
      currentIndex = -1;
      makeDirty(gdi,  ClearDirty);
      show(gdi);
      gdi.setInputStatus("Remove", resultIsInstalled());
    }
    else if (bi.id == "SaveFile") {
      if (!currentResult)
        return 0;
      checkChangedSave(gdi);
      
      string fileName;

      int ix = 0;
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("xml-data", "*.rules"));
      fileName = gdi.browseForSave(ext, "rules", ix);
      if (fileName.empty())
        return 0;

      saveSettings(gdi);
      string path = fileNameSource.empty() ? getInternalPath(currentResult->getTag()) : fileNameSource;
      currentResult->save(path);
      fileNameSource = path;
      oe->loadGeneralResults(true);
      makeDirty(gdi, ClearDirty);

      currentResult->save(fileName);
      //savedFileName = fileName;
      return 1;
    }
    else if (bi.id == "SaveInside") {
      if (!currentResult)
        return 0;
      saveSettings(gdi);
      checkChangedSave(gdi);

      string path = fileNameSource.empty() ? getInternalPath(currentResult->getTag()) : fileNameSource;
      currentResult->save(path);
      fileNameSource = path;

      int nl = oe->getListContainer().getNumLists();
      string utag = DynamicResult::undecorateTag(currentResult->getTag());
      bool doUpdate = false;
      for (int j = 0; j < nl && !doUpdate; j++) {
        if (!oe->getListContainer().isExternal(j))
          continue;

        const string &mtag = oe->getListContainer().getList(j).getResultModule();
        if (mtag.empty() || currentResult->getTag() == mtag)
          continue;

        if (utag == DynamicResult::undecorateTag(mtag)) {
          doUpdate = gdi.ask("Vill du uppdatera resultatlistorna i den �ppande t�vlingen?");
          break;
        }
      }
      
      oe->synchronize(false);
      oe->getListContainer().updateResultModule(*currentResult, doUpdate);

      oe->loadGeneralResults(true);

      oe->synchronize(true);

      makeDirty(gdi, ClearDirty);
      gdi.setInputStatus("Remove", resultIsInstalled());
      return 1;
    }
    else if (bi.id == "Remove") {
      if (!currentResult)
        return 0;
      if (gdi.ask("Vill du ta bort 'X'?#" + currentResult->getName(true))) {
        string path = fileNameSource;//getInternalPath(currentResult->getTag());
        string rm = path + ".removed";
        DeleteFile(rm.c_str());
        rename(path.c_str(), rm.c_str());
        oe->loadGeneralResults(true);
        makeDirty(gdi, ClearDirty);
        setCurrentResult(0, "");
        gdi.clearPage(false);
        show(gdi);
      }
      return 1;
    }
    else if (bi.id == "OpenFile") {
      if (!checkSave(gdi))
        return 0;

      vector< pair<string, string> > ext;
      ext.push_back(make_pair("xml-data", "*.rules"));
      string fileName = gdi.browseForOpen(ext, "rules");
      if (fileName.empty())
        return 0;

      DynamicResult *tmp = new DynamicResult();
      try {
        tmp->load(fileName);
      }
      catch(...) {
        delete tmp;
        throw;
      }

      wasLoadedBuiltIn = false;
      setCurrentResult(tmp, "");
      currentIndex = -1;
      gdi.clearPage(false);

      //savedFileName = fileName;
      makeDirty(gdi, ClearDirty);
      show(gdi);
    }
    else if (bi.id == "OpenInside") {
      if (!checkSave(gdi))
        return 0;

      gdi.clearPage(true);
      gdi.setOnClearCb(methodCB);
      gdi.setData("MethodEditorClz", this);

      gdi.pushX();
      vector< pair<string, size_t> > lists;
      //oe->getListContainer().getLists(lists);
      vector< pair<string, string> > tagNameList;
      oe->getGeneralResults(true, tagNameList, true);
      for (size_t k = 0; k < tagNameList.size(); k++) {
        string tag = tagNameList[k].first;
        string utag = DynamicResult::undecorateTag(tag);
        vector<int> listIx;
        oe->getListContainer().getListsByResultModule(tag, listIx);
        string n = tagNameList[k].second + " (" + utag + ")";
        
        if (listIx.size() > 0) {
          n += " *";
        }

        lists.push_back(make_pair(n, k));
      }
      sort(lists.begin(), lists.end());
      gdi.fillRight();
      gdi.addSelection("OpenList", 350, 400, methodCB, "Choose result module:", "Rader markerade med (*) kommer fr�n en lista i t�vlingen.");
      gdi.addItem("OpenList", lists);
      gdi.autoGrow("OpenList");
      gdi.selectFirstItem("OpenList");

      gdi.dropLine();
      gdi.addButton("DoOpen", "�ppna", methodCB);
      gdi.addButton("DoOpenCopy", "Open a Copy", methodCB);
    
      if (!lists.empty()) {
        string srcFile;
        bool ro = dynamic_cast<DynamicResult &>(oe->getGeneralResult(tagNameList[lists.front().second].first, srcFile)).isReadOnly();
        gdi.setInputStatus("DoOpen", !ro);
      }
      else {
        gdi.disableInput("DoOpen");
        gdi.disableInput("DoOpenCopy");
      }

      gdi.addButton("CancelReload", "Avbryt", methodCB);
      gdi.dropLine(4);
      gdi.popX();
    }
    else if (bi.id == "DoOpen" || bi.id == "DoOpenCopy") {
      ListBoxInfo lbi;
      DynamicResult *dr = 0;
      if (gdi.getSelectedItem("OpenList", &lbi)) {
        vector< pair<string, string> > tagNameList;
        oe->getGeneralResults(true, tagNameList, false);
        if (lbi.data < tagNameList.size()) {
          string srcFile;
          DynamicResult &drIn = dynamic_cast<DynamicResult &>(oe->getGeneralResult(tagNameList[lbi.data].first, srcFile));
          wasLoadedBuiltIn = drIn.isReadOnly();
          dr = new DynamicResult(drIn);
          if (bi.id == "DoOpenCopy") {
            dr->setTag(uniqueTag("result"));
            dr->setName(lang.tl("Copy of ") + dr->getName(false));
            setCurrentResult(dr, "");
          }
          else
            setCurrentResult(dr, srcFile);

        }

        if (bi.id == "DoOpen")
          makeDirty(gdi, ClearDirty);
        else
          makeDirty(gdi, MakeDirty);
      
        gdi.clearPage(false);
        show(gdi);
        if (dr) dr->compile(true);
      }
    }
    else if (bi.id == "CancelReload") {
      gdi.clearPage(false);
      show(gdi);
    }
    else if (bi.id == "Close") {
      if (!checkSave(gdi))
        return 0;

      setCurrentResult(0, "");
      makeDirty(gdi, ClearDirty);
      currentIndex = -1;
      gdi.getTabs().get(TListTab)->loadPage(gdi);
      return 0;
    }
    else if (bi.id == "SaveSource") {
      DynamicResult::DynamicMethods dm = DynamicResult::DynamicMethods(bi.getExtraInt());
      string src = gdi.getText("Source");
      currentResult->setMethodSource(dm, src);
      gdi.setText("Source", src);
    }
    else if (bi.id == "CancelSource") {
      checkChangedSave(gdi);
      gdi.restore("NoSourceEdit", true);
      gdi.selectItemByData("Method", -1);
      gdi.disableInput("SaveSource");
      gdi.disableInput("CancelSource");
    }
    else if (bi.id == "TestRule") {
      checkChangedSave(gdi);

      gdi.restore("TestRule", false);
      gdi.setRestorePoint("TestRule");
      gdi.disableInput("SaveSource");
      gdi.disableInput("CancelSource");
      gdi.selectItemByData("Method", -1);
      
      int y = gdi.getCY();
      vector<pRunner> r;
      vector<pTeam> t;
      oe->getRunners(0, 0, r, true);
      oe->getTeams(0, t, true);
      vector<pRunner> rr;
      vector<pTeam> tr;
      for (size_t k = 0; k < r.size(); k++) {
        if (r[k]->getStatus() != StatusUnknown)
          rr.push_back(r[k]);
      }
      for (size_t k = 0; k < t.size(); k++) {
        if (t[k]->getStatus() != StatusUnknown) 
            tr.push_back(t[k]);
        else {
          for (int j = 0; j < t[k]->getNumRunners(); j++) {
            if (t[k]->getRunner(j) && t[k]->getRunner(j)->getStatus() != StatusUnknown) {
              tr.push_back(t[k]);
              break;
            }
          }
        }
      }
      gdi.fillDown();
      if (tr.size() + rr.size() == 0) {
        gdi.addString("", 1, "T�vlingen inneh�ller inga resultat").setColor(colorRed);
      }
      else
        gdi.addString("", 1, "Applying rules to the current competition");
      gdi.dropLine(0.5);
      int xp = gdi.getCX();
      int yp = gdi.getCY();
      int diff = gdi.scaleLength(3);
      const int w[5] = {200, 70, 70, 70, 85};
      set<string> errors;
      currentResult->prepareCalculations(*oe, tr.size()>0, inputNumber);

      for (size_t k = 0; k < rr.size(); k++) {
        int txp = xp;
        int wi = 0;
        gdi.addStringUT(yp, txp, 0, rr[k]->getCompleteIdentification(), w[wi]-diff);
        txp += w[wi++];
        currentResult->prepareCalculations(*rr[k]);
        int rt = 0, pt = 0;
        RunnerStatus st = StatusUnknown;
        {
          string err;
          string str;
          try {
            st = currentResult->deduceStatus(*rr[k]);
            str = oe->formatStatus(st);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            RECT rc = ti.textRect;
            gdi.addToolTip("", err, 0, &rc);
          }
          txp += w[wi++];
        }
        {
          string err;
          string str;
          try {
            rt = currentResult->deduceTime(*rr[k], rr[k]->getStartTime());
            str = formatTime(rt);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            gdi.addToolTip("", err, 0, &ti.textRect);
          }
          txp += w[wi++];
        }

        {
          string err;
          string str;
          try {
            pt = currentResult->deducePoints(*rr[k]);
            str = itos(pt);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            gdi.addToolTip("", err, 0, &ti.textRect);
          }
          txp += w[wi++];
        }

        {
          string err;
          string str;
          try {
            int score = currentResult->score(*rr[k], st, rt, pt);
            str = itos(score);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            gdi.addToolTip("", err, 0, &ti.textRect);
          }
          txp += w[wi++];
        }
        rr[k]->setTempResultZero(oAbstractRunner::TempResult(st, rr[k]->getStartTime(), rt, pt));

        gdi.addString("Debug", yp, txp, 0, "Debug...", 0, methodCB).setColor(colorGreen).setExtra(rr[k]->getId());
        yp += gdi.getLineHeight();
      }

      yp += gdi.getLineHeight();

      if (tr.size() > 0) {
        gdi.addString("", yp, xp, 1, "Lag");
        yp += gdi.getLineHeight();
      }
      else {
      }

      for (size_t k = 0; k < tr.size(); k++) {
        int txp = xp;
        int wi = 0;
        gdi.addStringUT(yp, txp, 0, tr[k]->getName(), w[wi]-diff);
        txp += w[wi++];
        currentResult->prepareCalculations(*tr[k]);
        int rt = 0, pt = 0;
        RunnerStatus st = StatusUnknown;
        {
          string err;
          string str;
          try {
            st = currentResult->deduceStatus(*tr[k]);
            str = oe->formatStatus(st);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            gdi.addToolTip("", err, 0, &ti.textRect);
          }
          txp += w[wi++];
        }
        {
          string err;
          string str;
          try {
            rt = currentResult->deduceTime(*tr[k]);
            str = formatTime(rt);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            gdi.addToolTip("", err, 0, &ti.textRect);
          }
          txp += w[wi++];
        }

        {
          string err;
          string str;
          try {
            pt = currentResult->deducePoints(*tr[k]);
            str = itos(pt);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            gdi.addToolTip("", err, 0, &ti.textRect);
          }
          txp += w[wi++];
        }

        {
          string err;
          string str;
          try {
            int score = currentResult->score(*tr[k], st, rt, pt);
            str = itos(score);
          }
          catch (meosException &ex) {
            err = ex.what();
            errors.insert(ex.what());
            str = "Error";
          }
          TextInfo &ti = gdi.addStringUT(yp, txp, 0, str, w[wi]-diff);
          if (!err.empty()) {
            ti.setColor(colorRed);
            gdi.addToolTip("", err, 0, &ti.textRect);
          }
          txp += w[wi++];
        }

        gdi.addString("Debug", yp, txp, 0, "Debug...", 0, methodCB).setColor(colorGreen).setExtra(-tr[k]->getId());

        yp += gdi.getLineHeight();
      }

      gdi.scrollTo(0, y);
      gdi.refresh();
    }
  }
  else if (type == GUI_LISTBOX) {
    ListBoxInfo &lbi = dynamic_cast<ListBoxInfo &>(data);
    if (lbi.id == "Method") {
      checkChangedSave(gdi);

      DynamicResult::DynamicMethods m = DynamicResult::DynamicMethods(lbi.data);
      const string &src = currentResult->getMethodSource(m);
      gdi.enableInput("SaveSource");
      gdi.enableInput("CancelSource");
      gdi.getBaseInfo("SaveSource").setExtra(lbi.data);

      if (!gdi.hasField("Source")) {
        gdi.fillRight();
        gdi.restore("TestRule", false);
        gdi.pushX();
        gdi.setRestorePoint("NoSourceEdit");
        gdi.addInputBox("Source", 450, 300,
                        src,
                        methodCB, "Source code:").setFont(gdi, monoText);
        gdi.fillDown();
        gdi.setCX(gdi.getCX() + gdi.getLineHeight());
        gdi.addListBox("Symbols", 450, 300-20, methodCB, "Available symbols:");
        gdi.setTabStops("Symbols", 180);
        gdi.addString("SymbInfo", gdi.getCY(), gdi.getCX(), 0, "", 350, 0);
        gdi.popX();
        gdi.setCY(gdi.getHeight());
        gdi.dropLine();
        gdi.scrollToBottom();
      }
      else {
        gdi.setText("Source", src);
      }

      currentResult->declareSymbols(m, true);
      vector< pair<string, size_t> > symb;
      currentResult->getSymbols(symb);
      gdi.addItem("Symbols", symb);
    }
    else if (lbi.id == "Symbols") {
      string name, desc;
      currentResult->getSymbolInfo(lbi.data, name, desc);
      gdi.setText("SymbInfo", name + ":"  + lang.tl(desc) +".", true);
    }
    else if (lbi.id == "OpenList") {
      vector< pair<string, string> > tagNameList;
      oe->getGeneralResults(true, tagNameList, false);
      string srcFile;
      if (lbi.data < tagNameList.size()) {
        bool ro = dynamic_cast<DynamicResult &>(oe->getGeneralResult(tagNameList[lbi.data].first, srcFile)).isReadOnly();
        gdi.setInputStatus("DoOpen", !ro);
      }
      else
        gdi.setInputStatus("DoOpen", true);
    }
  }
  else if (type == GUI_LISTBOXSELECT) {
    ListBoxInfo &lbi = dynamic_cast<ListBoxInfo &>(data);
    if (lbi.id == "Symbols") {
      string name, desc;
      currentResult->getSymbolInfo(lbi.data, name, desc);
      gdi.replaceSelection("Source", name);
    }
  }
  else if (type==GUI_CLEAR) {
    return checkSave(gdi);
  }
  else if (type == GUI_INPUT) {
    InputInfo &ii = dynamic_cast<InputInfo &>(data);
    if (ii.changed())
      makeDirty(gdi, MakeDirty);
    
  }
  else if (type == GUI_LINK) {
     TextInfo &ti = dynamic_cast<TextInfo &>(data);
   
     if (ti.id == "Debug") {
       int id = ti.getExtraInt();
       if (id > 0) {
         debug(gdi, id, false);
       }
       else if (id < 0) {
         debug(gdi, -id, true);
       }
     }
  }

  return 0;
}

void MethodEditor::saveSettings(gdioutput &gdi) {
  string name = gdi.getText("Name");
  string tag;
  const bool updateTag = gdi.getBaseInfo("Tag").getExtraInt() == 0;
  if (updateTag)
    tag = gdi.getText("Tag");
  else
    tag = currentResult->getTag();

  string desc = gdi.getText("Desc");

  if (_strcmpi(currentResult->getTag().c_str(), tag.c_str()) != 0) {
    checkTag(tag, true);
    string oldPath = fileNameSource.empty() ? getInternalPath(currentResult->getTag()) : fileNameSource;
    string path = getInternalPath(tag);
    rename(oldPath.c_str(), path.c_str());
    fileNameSource = path;
  }

  currentResult->setTag(tag);
  currentResult->setName(name);
  currentResult->setDescription(desc);
  gdi.setText("Name", name);
  gdi.setText("Tag", tag);
  gdi.setText("Desc", desc);
}

void MethodEditor::checkUnsaved(gdioutput &gdi) {
 /*if (gdi.hasData("IsEditing")) {
    if (gdi.isInputChanged("")) {
      gdi.setData("NoRedraw", 1);
      gdi.sendCtrlMessage("Apply");
    }
  }
  if (gdi.hasData("IsEditingList")) {
    if (gdi.isInputChanged("")) {
      gdi.setData("NoRedraw", 1);
      gdi.sendCtrlMessage("ApplyListProp");
    }
  }*/
}


void MethodEditor::makeDirty(gdioutput &gdi, DirtyFlag inside) {
  if (inside == MakeDirty)
    dirtyInt = true;
  else if (inside == ClearDirty)
    dirtyInt = false;

  if (gdi.hasField("SaveInside")) {
    gdi.setInputStatus("SaveInside", dirtyInt);
    gdi.setInputStatus("Remove", resultIsInstalled());
  }
}

bool MethodEditor::checkSave(gdioutput &gdi) {
  if (dirtyInt) {
    gdioutput::AskAnswer answer = gdi.askCancel("Vill du spara �ndringar?");
    if (answer == gdioutput::AnswerCancel)
      return false;

    if (answer == gdioutput::AnswerYes) {
      gdi.sendCtrlMessage("SaveInside");
    }
    makeDirty(gdi, ClearDirty);
  }

  return true;
}

void MethodEditor::checkChangedSave(gdioutput &gdi) {
  if (gdi.hasField("Source")) {
    gdi.getText("Source");
    if (dynamic_cast<InputInfo &>(gdi.getBaseInfo("Source")).changed() &&
        gdi.ask("Save changes in rule code?")) {      
      DynamicResult::DynamicMethods dm = DynamicResult::DynamicMethods((int)gdi.getExtra("SaveSource"));
      string src = gdi.getText("Source");
      currentResult->setMethodSource(dm, src);
      gdi.setText("Source", src);
    }
  }
}

string MethodEditor::getInternalPath(const string &tag) {
  string udTag = DynamicResult::undecorateTag(tag);
  string resFile;
  if (udTag == tag)
    resFile = tag + ".rules";
  else
    resFile = "imp_" + udTag + ".rules";

  char path[260];
  getUserFile(path, resFile.c_str());
  return path;
}

bool MethodEditor::resultIsInstalled() const {
  if (!currentResult || fileNameSource.empty())
    return false;
  
  string tag = currentResult->getTag();
  vector<int> listIx;
  oe->getListContainer().getListsByResultModule(tag, listIx);

  if (!listIx.empty())
    return false; // Used in a list in this competition

  //string path = getInternalPath(currentResult->getTag());
  return fileExist(fileNameSource.c_str());
}

void MethodEditor::debug(gdioutput &gdi_in, int id, bool isTeam) {

  oAbstractRunner *art = 0;
  if (isTeam) 
    art = oe->getTeam(id);
  else
    art = oe->getRunner(id, 0);

  if (!art || !currentResult)
    throw meosException("Internal error");

  gdioutput *gdi_new = getExtraWindow("debug", true);
  string title = lang.tl("Debug X for Y#" + currentResult->getName(false) + "#" + art->getName());
  
  if (!gdi_new)
    gdi_new = createExtraWindow("debug", title, 
                                 gdi_in.scaleLength(500) );
  else
    gdi_new->setWindowTitle(title);

  gdi_new->clearPage(false);
  gdi_new->setData("MethodEditorClz", this);
  gdioutput &gdi = *gdi_new;
  currentResult->prepareCalculations(*oe, isTeam, inputNumber);
  gdi_new->addString("", fontMediumPlus, "Debug Output");
  if (!isTeam) {
    oRunner &r = *pRunner(art);
    currentResult->prepareCalculations(r);
    int rt = 0, pt = 0;
    RunnerStatus st = StatusUnknown;
    gdi.dropLine();
    try {
      st = currentResult->deduceStatus(r);
      currentResult->debugDumpVariables(gdi, true);

      gdi.addStringUT(1, "ComputedStatus: " + oe->formatStatus(st)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      currentResult->debugDumpVariables(gdi, true);
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Status Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MDeduceRStatus))
      currentResult->debugDumpVariables(gdi, false);
    
    try {
      rt = currentResult->deduceTime(r, r.getStartTime());
      gdi.addStringUT(1, "ComputedTime: " + formatTime(rt)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Time Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MDeduceRTime))
      currentResult->debugDumpVariables(gdi, false);
    
    try {
      pt = currentResult->deducePoints(r);
      gdi.addStringUT(1, "ComputedPoints: " + itos(pt)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Points Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MDeduceRPoints))
      currentResult->debugDumpVariables(gdi, true);
    
    try {
       int score = currentResult->score(r, st, rt, pt);
       gdi.addStringUT(1, "ComputedScore: " + itos(score)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      currentResult->debugDumpVariables(gdi, true);
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Score Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MRScore))
      currentResult->debugDumpVariables(gdi, false);
  }
  else {
    oTeam &t = *pTeam(art);
    currentResult->prepareCalculations(t);
    int rt = 0, pt = 0;
    RunnerStatus st = StatusUnknown;
    gdi.dropLine();
    try {
      st = currentResult->deduceStatus(t);
      currentResult->debugDumpVariables(gdi, true);

      gdi.addStringUT(1, "ComputedStatus: " + oe->formatStatus(st)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      currentResult->debugDumpVariables(gdi, true);
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Status Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MDeduceTStatus))
      currentResult->debugDumpVariables(gdi, false);
    
    try {
      rt = currentResult->deduceTime(t);
      gdi.addStringUT(1, "ComputedTime: " + formatTime(rt)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Time Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MDeduceRTime))
      currentResult->debugDumpVariables(gdi, false);
    
    try {
      pt = currentResult->deducePoints(t);
      gdi.addStringUT(1, "ComputedPoints: " + itos(pt)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Points Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MDeduceTPoints))
      currentResult->debugDumpVariables(gdi, true);
    
    try {
       int score = currentResult->score(t, st, rt, pt);
       gdi.addStringUT(1, "ComputedScore:"  + itos(score)).setColor(colorGreen);
    }
    catch (meosException &ex) {
      currentResult->debugDumpVariables(gdi, true);
      string err = lang.tl(ex.what());
      gdi.addString("", 0, "Status Calculation Failed: X#" + err).setColor(colorRed);
    }
    if (currentResult->hasMethod(DynamicResult::MTScore))
      currentResult->debugDumpVariables(gdi, false);
  }

  gdi.addButton("CloseWindow", "St�ng", methodCB);
  gdi.refresh();
}
