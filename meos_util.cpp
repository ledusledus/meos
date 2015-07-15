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

#include "stdafx.h"
#include <vector>
#include <math.h>
#include "meos_util.h"
#include "localizer.h"
#include "oFreeImport.h"

StringCache globalStringCache;

DWORD mainThreadId = -1;
StringCache &StringCache::getInstance() {
  DWORD id = GetCurrentThreadId();
  if (mainThreadId == -1)
    mainThreadId = id;
  else if (mainThreadId != id)
    throw std::exception("Thread access error");
  return globalStringCache;
}

string getLocalTime()
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  return convertSystemTime(st);
}

string getLocalDate()
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  return convertSystemDate(st);
}


int getThisYear() {
  static int thisYear = 0;
  if (thisYear == 0) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    thisYear = st.wYear;
  }
  return thisYear;
}


/** Extend a year from 03 -> 2003, 97 -> 1997 etc */
int extendYear(int year) {
  if (year<0)
    return year;

  if (year>=100)
    return year;

  int thisYear = getThisYear();

  int cLast = thisYear%100;

  if (cLast == 0 && year == 0)
    return thisYear;

  if (year > thisYear%100)
    return (thisYear - cLast) - 100 + year;
  else
    return (thisYear - cLast) + year;
}

string getLocalTimeFileName()
{
  SYSTEMTIME st;
  GetLocalTime(&st);

  char bf[32];
  sprintf_s(bf, "%d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay,
    st.wHour, st.wMinute, st.wSecond);

  return bf;
}


string getLocalTimeOnly()
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  return convertSystemTimeOnly(st);
}


int getRelativeDay() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);

  ULARGE_INTEGER u;
  u.HighPart = ft.dwHighDateTime;
  u.LowPart = ft.dwLowDateTime;
  __int64 qp = u.QuadPart;
  qp /= __int64(10) * 1000 * 1000 * 3600 * 24;
  qp -=  400*365;
  return int(qp);
}

__int64 SystemTimeToInt64Second(const SYSTEMTIME &st) {
  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);

  ULARGE_INTEGER u;
  u.HighPart = ft.dwHighDateTime;
  u.LowPart = ft.dwLowDateTime;
  __int64 qp = u.QuadPart;
  qp /= __int64(10) * 1000 * 1000;
  return qp;
}

SYSTEMTIME Int64SecondToSystemTime(__int64 time) {
  SYSTEMTIME st;
  FILETIME ft;

  ULARGE_INTEGER u;
  u.QuadPart = time * __int64(10) * 1000 * 1000;
  ft.dwHighDateTime = u.HighPart;
  ft.dwLowDateTime = u.LowPart;

  FileTimeToSystemTime(&ft, &st);

  return st;
}
//2014-11-03 07:02:00
string convertSystemTime(const SYSTEMTIME &st)
{
  char bf[32];
  sprintf_s(bf, "%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay,
    st.wHour, st.wMinute, st.wSecond);

  return bf;
}


string convertSystemTimeOnly(const SYSTEMTIME &st)
{
  char bf[32];
  sprintf_s(bf, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

  return bf;
}


string convertSystemDate(const SYSTEMTIME &st)
{
  char bf[32];
  sprintf_s(bf, "%d-%02d-%02d", st.wYear, st.wMonth, st.wDay);

  return bf;
}

//Absolute time string to SYSTEM TIME
int convertDateYMS(const string &m, SYSTEMTIME &st)
{
  memset(&st, 0, sizeof(st));

  if (m.length()==0)
    return -1;

  int len=m.length();
  for (int k=0;k<len;k++) {
    BYTE b=m[k];
    if (b == 'T')
      break;
    if ( !(b=='-' || b==' ' || (b>='0' && b<='9')) )
      return -1;
  }

  int year=atoi(m.c_str());
  if (year<1000 || year>3000)
    return -1;

  int month=0;
  int day=0;
  int kp=m.find_first_of('-');

  if (kp!=string::npos) {
    string mtext=m.substr(kp+1);
    month=atoi(mtext.c_str());

    if (month<1 || month>12)
      month=0;

    kp=mtext.find_last_of('-');

    if (kp!=string::npos) {
      day=atoi(mtext.substr(kp+1).c_str());
      if (day<1 || day>31)
        day=0;
    }
  }
  st.wYear = year;
  st.wMonth = month;
  st.wDay = day;


  int t = year*100*100+month*100+day;
  if (t<0) return 0;

  return t;
}


//Absolute time string to absolute time int
int convertDateYMS(const string &m)
{
  SYSTEMTIME st;
  return convertDateYMS(m, st);
}


bool myIsSpace(BYTE b) {
  return isspace(b) || BYTE(b)==BYTE(160);
}

//Absolute time string to absolute time int
int convertAbsoluteTimeHMS(const string &m)
{
  int len = m.length();

  if (len==0 || m[0]=='-')
    return -1;

  // Support for notation 2D 14:30:00 or 2T14:30:00 for several days
  int tpart = m.find_first_of("T");
  if (tpart == m.npos)
    tpart = m.find_first_of("D");
  if (tpart == m.npos)
    tpart = m.find_first_of("d");
  if (tpart == m.npos)
    tpart = m.find_first_of("t");

  if (tpart != m.npos) {
    tpart = convertAbsoluteTimeHMS(m.substr(tpart+1));
    if (tpart != -1) {
      int days = atoi(m.c_str());
      if (days <= 0)
        return -1;
      return days * 3600 * 24 + tpart;
    }
    return -1;
  }

  int plusIndex = -1;
  for (int k=0;k<len;k++) {
    BYTE b=m[k];
    if ( !(myIsSpace(b) || b==':' || (b>='0' && b<='9')) ) {
      if (b=='+' && plusIndex ==-1 && k>0)
        plusIndex = k;
      else
        return -1;
    }
  }

  if (plusIndex>0) {
    int t = convertAbsoluteTimeHMS(m.substr(plusIndex+1));
    int d = atoi(m.c_str());

    if (d>0 && t>=0)
      return d*24*3600 + t;
    else
      return -1;
  }

  int hour=atoi(m.c_str());

  if (hour<0 || hour>23)
    return -1;

  int minute=0;
  int second=0;
  int kp=m.find_first_of(':');

  if (kp!=string::npos) {
    string mtext=m.substr(kp+1);
    minute=atoi(mtext.c_str());

    if (minute<0 || minute>60)
      minute=0;

    kp=mtext.find_last_of(':');

    if (kp!=string::npos) {
      second=atoi(mtext.substr(kp+1).c_str());
      if (second<0 || second>60)
        second=0;
    }
  }
  int t=hour*3600+minute*60+second;
  if (t<0) return 0;

  return t;
}


//Absolute time string to absolute time int
int convertAbsoluteTimeISO(const string &m)
{
  int len = m.length();

  if (len==0 || m[0]=='-')
    return -1;

  string hStr, mStr, sStr;

  string tmp =  trim(m);

  if (tmp.length() < 3)
    return -1;

  hStr = tmp.substr(0, 2);
  if (!(tmp[2] >= '0' && tmp[2]<='9'))
    tmp = tmp.substr(3);
  else
    tmp = tmp.substr(2);

  if (tmp.length() < 3)
    return -1;

  mStr = tmp.substr(0, 2);

  if (!(tmp[2] >= '0' && tmp[2]<='9'))
    tmp = tmp.substr(3);
  else
    tmp = tmp.substr(2);

  if (tmp.length() < 2)
    return -1;

  sStr = tmp.substr(0, 2);

  int hour = atoi(hStr.c_str());
  if (hour<0 || hour>23)
    return -1;

  int minute = atoi(mStr.c_str());

  if (minute<0 || minute>60)
    return -1;

  int second = atoi(sStr.c_str());

  if (second<0 || second>60)
    return -1;

  int t = hour*3600 + minute*60 + second;

  return t;
}


// Parse +-MM:SS or +-HH:MM:SS
int convertAbsoluteTimeMS(const string &m)
{
  if (m.length()==0)
    return NOTIME;

  int minute=0;
  int second=0;

  int signpos=m.find_first_of('-');
  string mtext;

  int sign=1;

  if (signpos!=string::npos) {
    sign=-1;
    mtext=m.substr(signpos+1);
  }
  else
    mtext=m;

  minute=atoi(mtext.c_str());

  if (minute<0 || minute>60*24)
    minute=0;

  int kp=mtext.find_first_of(':');

  if (kp!=string::npos) {
    mtext = mtext.substr(kp+1);
    second = atoi(mtext.c_str());
    if (second<0 || second>60)
      second=0;
  }

  int t=minute*60+second;

  kp=mtext.find_first_of(':');
  if (kp!=string::npos) {
    //Allow also for format +-HH:MM:SS
    mtext = mtext.substr(kp+1);
    second=atoi(mtext.c_str());
    if (second<0 || second>60)
      second=0;
    else
      t = t*60 + second;
  }
  return sign*t;
}

//Generate +-MM:SS or +-HH:MM:SS
const string &getTimeMS(int m) {
  char bf[32];
  int am = abs(m);
  if (am < 3600)
    sprintf_s(bf, "-%02d:%02d", am/60, am%60);
  else if (am < 3600*48)
    sprintf_s(bf, "-%02d:%02d:%02d", am/3600, (am/60)%60, am%60);
  else {
    m = 0;
    sprintf_s(bf, "-");
  }
  string &res = StringCache::getInstance().get();
  if (m<0)
    res = bf; // with minus
  else
    res = bf + 1;

  return res;
}

const string &formatTime(int rt) {
  string &res = StringCache::getInstance().get();
  if (rt>0 && rt<3600*72) {
    char bf[16];
    if (rt>=3600)
      sprintf_s(bf, 16, "%d:%02d:%02d", rt/3600,(rt/60)%60, rt%60);
    else
      sprintf_s(bf, 16, "%d:%02d", (rt/60), rt%60);

    res = bf;
    return res;
  }
  char ret[2] = {BYTE(0x96), 0};
  res = ret;
  return res;
}

const string &formatTimeHMS(int rt) {

  string &res = StringCache::getInstance().get();
  if (rt>=0) {
    char bf[32];
    sprintf_s(bf, 16, "%02d:%02d:%02d", rt/3600,(rt/60)%60, rt%60);

    res = bf;
    return res;
  }
  char ret[2] = {BYTE(0x96), 0};
  res = ret;
  return res;
}

string formatTimeIOF(int rt, int zeroTime)
{
  if (rt>0 && rt<(3600*48)) {
    rt+=zeroTime;
    char bf[16];
    sprintf_s(bf, 16, "%02d:%02d:%02d", rt/3600,(rt/60)%60, rt%60);

    return bf;
  }
  return "--:--:--";
}

size_t find(const string &str, const string &separator, size_t startpos)
{
  size_t seplen = separator.length();

  for (size_t m = startpos; m<str.length(); m++) {
    for (size_t n = 0; n<seplen; n++)
      if (str[m] == separator[n])
        return m;
  }
  return str.npos;
}

const vector<string> & split(const string &line, const string &separators, vector<string> &split_vector) {
  split_vector.clear();

  if (line.empty())
    return split_vector;

  size_t startpos=0;
  size_t nextp=find(line, separators, startpos);
  split_vector.push_back(line.substr(startpos, nextp-startpos));

  while(nextp!=line.npos) {
    startpos=nextp+1;
    nextp=find(line, separators, startpos);
    split_vector.push_back(line.substr(startpos, nextp-startpos));
  }

  return split_vector;
}

const string &unsplit(const vector<string> &split_vector, const string &separators, string &line) {
  size_t s = split_vector.size() * separators.size();
  for (size_t k = 0; k < split_vector.size(); k++) {
    s += split_vector[k].size();
  }
  
  line.clear();
  line.reserve(s);
  for (size_t k = 0; k < split_vector.size(); k++) {
    if (k != 0)
      line += separators;
    line += split_vector[k];
  }
  return line;
}


const string &MakeDash(const string &t) {
  return MakeDash(t.c_str());
}

const string &MakeDash(const char *t) {
  string &out = StringCache::getInstance().get();
  out = t;
  for (size_t i=0;i<out.length(); i++) {
    if (t[i]=='-')
      out[i]=BYTE(0x96);
  }
  return out;
}

string FormatRank(int rank)
{
  char r[16];
  sprintf_s(r, "(%04d)", rank);
  return r;
}

const string &itos(int i)
{
  char bf[32];
  _itoa_s(i, bf, 10);
  string &res = StringCache::getInstance().get();
  res = bf;
  return res;
}

string itos(unsigned int i)
{
  char bf[32];
  _ultoa_s(i, bf, 10);
  return bf;
}

string itos(unsigned long i)
{
  char bf[32];
  _ultoa_s(i, bf, 10);
  return bf;
}

string itos(__int64 i)
{
  char bf[32];
  _i64toa_s(i, bf, 32, 10);
  return bf;
}

bool filterMatchString(const string &c, const char *filt_lc)
{
  if (filt_lc[0] == 0)
    return true;
  char key[2048];
  strcpy_s(key, c.c_str());
  CharLowerBuff(key, c.length());

  return strstr(key, filt_lc)!=0;
}

int countWords(const char *p) {
  int nwords=0;
  const unsigned char *ep=LPBYTE(p);
  while (*ep) {
    if (!isspace(*ep)) {
      nwords++;
      while ( *ep && !isspace(*ep) )
        ep++;
    }
    while (*ep && isspace(*ep))
        ep++;
  }
  return nwords;
}

string trim(const string &s)
{
  const char *ptr=s.c_str();
  int len=s.length();

  int i=0;
  while(i<len && (isspace(BYTE(ptr[i])) || BYTE(ptr[i])==BYTE(160))) i++;

  int k=len-1;

  while(k>=0 && (isspace(BYTE(ptr[k])) || BYTE(ptr[i])==BYTE(160))) k--;

  if (i == 0 && k == len-1)
    return s;
  else if (k>=i && i<len)
    return s.substr(i, k-i+1);
  else return "";
}

bool fileExist(const char *file)
{
  return GetFileAttributes(file) != INVALID_FILE_ATTRIBUTES;
}

bool stringMatch(const string &a, const string &b) {
  string aa = trim(a);
  string bb = trim(b);

  return CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, aa.c_str(), aa.length(), bb.c_str(), bb.length())==2;
}

const string &encodeXML(const string &in)
{
  static string out;
  const char *bf = in.c_str();
  int len = in.length();
  bool needEncode = false;
  for (int k=0;k<len ;k++)
    needEncode |=  (bf[k]=='&') | (bf[k]=='>') | (bf[k]=='<') | (bf[k]=='"') | (bf[k]==0) | (bf[k]=='\n') | (bf[k]=='\r');

  if (!needEncode)
    return in;
  out.clear();
  for (int k=0;k<len ;k++) {
    if (bf[k]=='&')
      out+="&amp;";
    else if (bf[k]=='<')
      out+="&lt;";
    else if (bf[k]=='>')
      out+="&gt;";
    else if (bf[k]=='"')
      out+="&quot;";
    else if (bf[k]=='\n')
      out+="&#10;";
    else if (bf[k]=='\r')
      out+="&#13;";
    else if (bf[k] == 0)
      out+=' ';
    else
      out+=bf[k];
  }
  return out;
}

const string &decodeXML(const string &in)
{
  const char *bf = in.c_str();
  int len = in.length();
  bool needDecode = false;
  for (int k=0;k<len ;k++)
    needDecode |=  (bf[k]=='&');

  if (!needDecode)
    return in;

  static string out;
  out.clear();
  for (int k=0;k<len ;k++) {
    if (bf[k]=='&') {
      if ( memcmp(&bf[k], "&amp;", 5)==0 )
        out+="&", k+=4;
      else if  ( memcmp(&bf[k], "&lt;", 4)==0 )
        out+="<", k+=3;
      else if  ( memcmp(&bf[k], "&gt;", 4)==0 )
        out+=">", k+=3;
      else if  ( memcmp(&bf[k], "&quot;", 6)==0 )
        out+="\"", k+=5;
      else if  ( memcmp(&bf[k], "&#10;", 5)==0 )
        out+="\n", k+=4;
      else if  ( memcmp(&bf[k], "&#13;", 5)==0 )
        out+="\r", k+=4;
      else
        out+=bf[k];
    }
    else
      out+=bf[k];
  }

  return out;
}

void inplaceDecodeXML(char *in)
{
  char *bf = in;
  int outp = 0;

  for (int k=0;bf[k] ;k++) {
    if (bf[k] != '&')
      bf[outp++] = bf[k];
    else {
      if ( memcmp(&bf[k], "&amp;", 5)==0 )
        bf[outp++] = '&', k+=4;
      else if  ( memcmp(&bf[k], "&lt;", 4)==0 )
        bf[outp++] = '<', k+=3;
      else if  ( memcmp(&bf[k], "&gt;", 4)==0 )
        bf[outp++] = '>', k+=3;
      else if  ( memcmp(&bf[k], "&quot;", 6)==0 )
        bf[outp++] = '"', k+=5;
      else if  ( memcmp(&bf[k], "&#10;", 5)==0 )
        bf[outp++] = '\n', k+=4;
      else if  ( memcmp(&bf[k], "&#13;", 5)==0 )
        bf[outp++] = '\r', k+=4;
      else
        bf[outp++] = bf[k];
    }
  }
  bf[outp] = 0;
}

const char *decodeXML(const char *in)
{
  const char *bf = in;
  bool needDecode = false;
  for (int k=0; bf[k] ;k++)
    needDecode |=  (bf[k]=='&');

  if (!needDecode)
    return in;

  static string out;
  out.clear();
  for (int k=0;bf[k] ;k++) {
    if (bf[k]=='&') {
      if ( memcmp(&bf[k], "&amp;", 5)==0 )
        out+="&", k+=4;
      else if  ( memcmp(&bf[k], "&lt;", 4)==0 )
        out+="<", k+=3;
      else if  ( memcmp(&bf[k], "&gt;", 4)==0 )
        out+=">", k+=3;
      else if  ( memcmp(&bf[k], "&quot;", 6)==0 )
        out+="\"", k+=5;
      else if  ( memcmp(&bf[k], "&#10;", 5)==0 )
        out+="\n", k+=4;
      else if  ( memcmp(&bf[k], "&#13;", 5)==0 )
        out+="\r", k+=4;
      else
        out+=bf[k];
    }
    else
      out+=bf[k];
  }

  return out.c_str();
}

void static setChar(unsigned char *map, unsigned char pos, unsigned char value)
{
  map[pos] = value;
}

int toLowerStripped(int c) {
  const unsigned cc = c&0xFF;
  if (cc>='A' && cc<='Z')
    return cc + ('a' - 'A');
  else if (cc<128)
    return cc;

  static unsigned char map[256] = {0, 0};
  if (map[1] == 0) {
    for (unsigned i = 0; i < 256; i++)
      map[i] = unsigned char(i);

    setChar(map, 'Å', 'å');
    setChar(map, 'Ä', 'ä');
    setChar(map, 'Ö', 'ö');

    setChar(map, 'É', 'e');
    setChar(map, 'é', 'e');
    setChar(map, 'è', 'e');
    setChar(map, 'È', 'e');
    setChar(map, 'ë', 'e');
    setChar(map, 'Ë', 'e');
    setChar(map, 'ê', 'e');
    setChar(map, 'Ê', 'e');

    setChar(map, 'û', 'u');
    setChar(map, 'Û', 'u');
    setChar(map, 'ü', 'u');
    setChar(map, 'Ü', 'u');
    setChar(map, 'ú', 'u');
    setChar(map, 'Ú', 'u');
    setChar(map, 'ù', 'u');
    setChar(map, 'Ù', 'u');

    setChar(map, 'ñ', 'n');
    setChar(map, 'Ñ', 'n');

    setChar(map, 'á', 'a');
    setChar(map, 'Á', 'a');
    setChar(map, 'à', 'a');
    setChar(map, 'À', 'a');
    setChar(map, 'â', 'a');
    setChar(map, 'Â', 'a');
    setChar(map, 'ã', 'a');
    setChar(map, 'Ã', 'a');

    setChar(map, 'ï', 'i');
    setChar(map, 'Ï', 'i');
    setChar(map, 'î', 'i');
    setChar(map, 'Î', 'i');
    setChar(map, 'í', 'i');
    setChar(map, 'Í', 'i');
    setChar(map, 'ì', 'i');
    setChar(map, 'Ì', 'i');

    setChar(map, 'ó', 'o');
    setChar(map, 'Ó', 'o');
    setChar(map, 'ò', 'o');
    setChar(map, 'Ò', 'o');
    setChar(map, 'õ', 'o');
    setChar(map, 'Õ', 'o');
    setChar(map, 'ô', 'o');
    setChar(map, 'Ô', 'o');

    setChar(map, 'ý', 'y');
    setChar(map, 'Ý', 'Y');
    setChar(map, 'ÿ', 'y');

    setChar(map, 'Æ', 'ä');
    setChar(map, 'æ', 'ä');

    setChar(map, 'Ø', 'ö');
    setChar(map, 'ø', 'ö');

    setChar(map, 'Ç', 'c');
    setChar(map, 'ç', 'c');
  }
  int a = map[cc];
  return a;
}

const char *canonizeName(const char *name)
{
  static char out[128];
  int outp = 0;
  int k = 0;
  for (k=0; k<63 && name[k]; k++)
    if (name[k] != ' ')
      break;

  bool first = true;
  while (k<63 && name[k]) {
    if (!first)
      out[outp++] = ' ';

    while(name[k]!= ' ' && k<63 && name[k]) {
      if (name[k] == '-')
        out[outp++] = ' ';
      else
        out[outp++] = toLowerStripped(name[k]);
      k++;
    }

    first = false;
    while(name[k] == ' ' && k<64)
      k++;
  }

  out[outp] = 0;
  return out;
}

const int notFound = 1000000;

int charDist(const char *b, int len, int origin, char c)
{
  int i;
  int bound = max(1, min(len/2, 4));
  for (int k = 0;k<bound; k++) {
    i = origin - k;
    if (i>0 && b[i] == c)
      return -k;
    i = origin + k;
    if (i<len && b[i] == c)
      return k;
  }
  return notFound;
}

static double stringDistance(const char *a, int al, const char *b, int bl)
{
  al = min (al, 256);
  int d1[256];
  int avg = 0;
  int missing = 0;
  int ndiff = 1; // Do not allow zero
  for (int k=0; k<al; k++) {
    int d = charDist(b, bl, k, a[k]);
    if (d == notFound) {
      missing++;
      d1[k] = 0;
    }
    else {
      d1[k] = d;
      avg += d;
      ndiff ++;
    }
  }
  if (missing>min(3, max(1, al/3)))
    return 1;

  double mfactor = double(missing*0.8);
  double center = double(avg)/double(ndiff);

  double dist = 0;
  for (int k=0; k<al; k++) {
    double ld = min<double>(fabs(d1[k] - center), abs(d1[k]));
    dist += ld*ld;
  }


  return (sqrt(dist)+mfactor*mfactor)/double(al);
}

double stringDistance(const char *a, const char *b)
{
  int al = strlen(a);
  int bl = strlen(b);

  double d1 = stringDistance(a, al, b, bl);
  if (d1 >= 1)
    return 1.0;

  double d2 = stringDistance(b, bl, a, al);
  if (d2 >= 1)
    return 1.0;

  return (max(d1, d2) + d1 + d2)/3.0;
}

int getNumberSuffix(const string &str)
{
  int pos = str.length();

  while (pos>1 && (isspace(str[pos-1]) || isdigit(str[pos-1]))) {
    pos--;
  }

  if (pos == str.length())
    return 0;
  return atoi(str.c_str() + pos);
}

int extractAnyNumber(const string &str, string &prefix, string &suffix)
{
  const unsigned char *ptr = (const unsigned char*)str.c_str();
  for (size_t k = 0; k<str.length(); k++) {
    if (isdigit(ptr[k])) {
      prefix = str.substr(0, k);
      int num = atoi(str.c_str() + k);
      while(k<str.length() && isdigit(str[++k]));
      suffix = str.substr(k);

      return num;
    }
  }
  return -1;
}


/** Matches H21 L with H21 Lång and H21L */
bool compareClassName(const string &a, const string &b)
{
  const char *as, *bs;
  if (a.length()>b.length()) {
    as = a.c_str();
    bs = b.c_str();
  }
  else {
    bs = a.c_str();
    as = b.c_str();
  }

  if (b.length() == 0 || a.length() == 0)
    return false; // Dont process empty names

  int firstAChar = -1;
  int firstBChar = -1;

  int lasttype = -1;
  int lastatype = -1;
  while (*bs) {
    int bchar = toLowerStripped(*bs);
    if (isspace(bchar) || bchar=='-' || bchar == 160) {
      bs++;
      lasttype = 0; // Space
      continue;
    }
    if (firstBChar == -1)
      firstBChar = bchar;

    if (bchar>='0' && bchar<='9')
      lasttype = 1; // Digit
    else
      lasttype = 2; // Other

    while (*as) {
      int achar = toLowerStripped(*as);
      int atype;
      if (isspace(achar) || achar=='-' || achar == 160)
        atype = 0; // Space
      else if (achar>='0' && achar<='9')
        atype = 1; // Digit
      else
        atype = 2; // Other

      if (atype != 0 && firstAChar == -1) {
        firstAChar = achar;
        if (firstAChar != firstBChar)
          return false; // First letter must match
      }
      if (achar == bchar) {
        lastatype  = atype;
        break; // Match!
      }

      if (lastatype != -1) {
        if ((lastatype == 0 && atype != 0) || atype == 1)
          return false; // Dont match missed word (space appear) or missing digits
        if (lastatype == 1 && atype == 2)
          return false; // Dont match 21 and 21L
      }
      lastatype = atype;
      ++as;
    }

    if (*as == 0)
      return false;
    ++as;
    ++bs;
  }

  int tail_len = strlen(as);

  // Check remaining
  while (*as) {
    int achar = toLowerStripped(*as);
    if (isspace(achar) || achar=='-' || achar == 160) {
      as++;
      continue;
    }

    if (lasttype <= 1) // The short name ended with a digit.
      return false;   // We cannot allow more chars.

    if (lasttype == 2 && tail_len == 1) // Do not match shortened names such has H and HL
      return false;

    if (achar>='0' && achar<='9')
      return false; // Never allow more digits

    ++as;
  }
  return true;
}

string getErrorMessage(int code) {
  LPVOID msg;
  int s = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                code,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &msg, 0, NULL);
  if (s==0 || !msg) {
    if (code != 0) {
      char ch[128];
      sprintf_s(ch, "Error code: %d", code);
      return ch;
    }
    return "";
  }

  string str = LPCTSTR(msg);
  if (str.empty() && code>0) {
    char ch[128];
    sprintf_s(ch, "Error code: %d", code);
    str = ch;
  }

  LocalFree(msg);
  return str;
}

#define  HLSMAX   252 /* H,L, and S vary over 0-HLSMAX */
#define  RGBMAX   255   /* R,G, and B vary over 0-RGBMAX */
                        /* HLSMAX BEST IF DIVISIBLE BY 6 */
                        /* RGBMAX, HLSMAX must each fit in a byte. */

/* Hue is undefined if Saturation is 0 (grey-scale) */
#define UNDEFINED (HLSMAX*2/3)

HLS &HLS::RGBtoHLS(DWORD lRGBColor)
{
  WORD R,G,B;          /* input RGB values */
  BYTE cMax,cMin;      /* max and min RGB values */
  WORD  Rdelta,Gdelta,Bdelta; /* intermediate value: % of spread from max
                              */
  /* get R, G, and B out of DWORD */
  R = GetRValue(lRGBColor);
  G = GetGValue(lRGBColor);
  B = GetBValue(lRGBColor);
  WORD &L = lightness;
  WORD &H = hue;
  WORD &S = saturation;

  /* calculate lightness */
  cMax = (BYTE)max( max(R,G), B);
  cMin = (BYTE)min( min(R,G), B);
  L = ( ((cMax+cMin)*HLSMAX) + RGBMAX )/(2*RGBMAX);

  if (cMax == cMin) {           /* r=g=b --> achromatic case */
    S = 0;                     /* saturation */
    H = UNDEFINED;             /* hue */
  }
  else {                        /* chromatic case */
    /* saturation */
    if (L <= (HLSMAX/2))
      S = ( ((cMax-cMin)*HLSMAX) + ((cMax+cMin)/2) ) / (cMax+cMin);
    else
      S = ( ((cMax-cMin)*HLSMAX) + ((2*RGBMAX-cMax-cMin)/2) )
      / (2*RGBMAX-cMax-cMin);

    /* hue */
    Rdelta = ( ((cMax-R)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
    Gdelta = ( ((cMax-G)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
    Bdelta = ( ((cMax-B)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);

    if (R == cMax)
      H = Bdelta - Gdelta;
    else if (G == cMax)
      H = (HLSMAX/3) + Rdelta - Bdelta;
    else /* B == cMax */
      H = ((2*HLSMAX)/3) + Gdelta - Rdelta;

    if (H < 0)
      H += HLSMAX;
    if (H > HLSMAX)
      H -= HLSMAX;
  }
  return *this;
}


/* utility routine for HLStoRGB */
WORD HLS::HueToRGB(WORD n1, WORD n2, WORD hue) const
{
  /* range check: note values passed add/subtract thirds of range */
  if (hue < 0)
    hue += HLSMAX;

  if (hue > HLSMAX)
    hue -= HLSMAX;

  /* return r,g, or b value from this tridrant */
  if (hue < (HLSMAX/6))
    return ( n1 + (((n2-n1)*hue+(HLSMAX/12))/(HLSMAX/6)) );
  if (hue < (HLSMAX/2))
    return ( n2 );
  if (hue < ((HLSMAX*2)/3))
    return ( n1 +    (((n2-n1)*(((HLSMAX*2)/3)-hue)+(HLSMAX/12))/(HLSMAX/6))
    );
  else
    return ( n1 );
}

DWORD HLS::HLStoRGB() const
{
  const WORD &lum = lightness;
  const WORD &sat = saturation;

  WORD R,G,B;                /* RGB component values */
  WORD  Magic1,Magic2;       /* calculated magic numbers (really!) */

  if (sat == 0) {            /* achromatic case */
    R=G=B=(lum*RGBMAX)/HLSMAX;
    if (hue != UNDEFINED) {
      /* ERROR */
    }
  }
  else  {                    /* chromatic case */
    /* set up magic numbers */
    if (lum <= (HLSMAX/2))
      Magic2 = (lum*(HLSMAX + sat) + (HLSMAX/2))/HLSMAX;
    else
      Magic2 = lum + sat - ((lum*sat) + (HLSMAX/2))/HLSMAX;
    Magic1 = 2*lum-Magic2;

    /* get RGB, change units from HLSMAX to RGBMAX */
    R = (HueToRGB(Magic1,Magic2,hue+(HLSMAX/3))*RGBMAX +
      (HLSMAX/2))/HLSMAX;
    G = (HueToRGB(Magic1,Magic2,hue)*RGBMAX + (HLSMAX/2)) / HLSMAX;
    B = (HueToRGB(Magic1,Magic2,hue-(HLSMAX/3))*RGBMAX +
      (HLSMAX/2))/HLSMAX;
  }
  return(RGB(R,G,B));
}

void HLS::lighten(double f) {
  lightness = min<WORD>(HLSMAX, WORD(f*lightness));
}

void HLS::saturate(double s) {
  saturation = min<WORD>(HLSMAX, WORD(s*saturation));
}

void HLS::colorDegree(double d) {

}

bool isAscii(const string &s) {
  for (size_t k = 0; k<s.length(); k++)
    if (!isascii(s[k]))
      return false;
  return true;
}

bool isNumber(const string &s) {
  for (size_t k = 0; k<s.length(); k++)
    if (!isdigit(s[k]))
      return false;
  return true;
}

bool expandDirectory(const char *file, const char *filetype, vector<string> &res)
{
  WIN32_FIND_DATA fd;

  char dir[MAX_PATH];
  char fullPath[MAX_PATH];

  if (file[0] == '.') {
    GetCurrentDirectory(MAX_PATH, dir);
    strcat_s(dir, file+1);
  }
  else
    strcpy_s(dir, MAX_PATH, file);

  if (dir[strlen(dir)-1]!='\\')
    strcat_s(dir, MAX_PATH, "\\");

  strcpy_s(fullPath, MAX_PATH, dir);
  strcat_s(dir, MAX_PATH, filetype);

  HANDLE h=FindFirstFile(dir, &fd);

  if (h == INVALID_HANDLE_VALUE)
    return false;

  bool more = true;

  while (more) {
    if (fd.cFileName[0] != '.') {
      //Avoid .. and .
      char fullPathFile[MAX_PATH];
      strcpy_s(fullPathFile, MAX_PATH, fullPath);
      strcat_s(fullPathFile, MAX_PATH, fd.cFileName);
      res.push_back(fullPathFile);
    }
    more=FindNextFile(h, &fd)!=0;
  }

  FindClose(h);
  return true;
}

string encodeSex(PersonSex sex) {
  if (sex == sFemale)
    return "F";
  else if (sex == sMale)
    return "M";
  else if (sex == sBoth)
    return "B";
  else
    return "";
}

PersonSex interpretSex(const string &sex) {
  if (sex == "F")
    return sFemale;
  else if (sex == "M")
    return sMale;
  else if (sex == "B")
    return sBoth;
  if (sex == "K" || sex == "W")
    return sFemale;
  else
    return sUnknown;
}

bool matchNumber(int a, const char *b) {
  if (a == 0 && b[0])
    return false;

  char bf[32];
  _itoa_s(a, bf, 10);

  // Check matching substring
  for (int k = 0; k < 12; k++) {
    if (b[k] == 0)
      return true;
    if (bf[k] != b[k])
      return false;
  }

  return false;
}

string makeValidFileName(const string &input, bool strict) {
  string out;
  out.reserve(input.size());

  if (strict) {
    for (size_t k = 0; k < input.length(); k++) {
      int b = input[k];
      if ( (b>='0' && b<='9') || (b>='a' && b<='z') || (b>='A' && b<='Z') || b == '_' || b=='.' )
        out.push_back(b);
      else if (b == ' ' ||  b == ',')
        out.push_back('_');
      else {
        b = toLowerStripped(b);
        if ( char(b) == 'ö')
          b = 'o';
        else if (char(b) == 'ä' || char(b) == 'å' || char(b)== 'à' || char(b)== 'á' || char(b)== 'â' || char(b)== 'ã' || char(b)== 'æ')
          b = 'a';
        else if (char(b) == 'ç')
          b = 'c';
        else if (char(b) == 'è' || char(b) == 'é' || char(b) == 'ê' || char(b) == 'ë')
          b = 'e';
        else if (char(b) == 'ð')
          b = 't';
        else if (char(b) == 'ï' || char(b) == 'ì' || char(b) == 'ï' || char(b) == 'î' || char(b) == 'í')
          b = 'i';
        else if (char(b) == 'ò' || char(b) == 'ó' || char(b) == 'ô' || char(b) == 'õ' || char(b) == 'ø')
          b = 'o';
        else if (char(b) == 'ù' || char(b) == 'ú' || char(b) == 'û' || char(b) == 'ü')
          b = 'u';
        else if (char(b) == 'ý')
          b = 'y';
        else
          b = '-';

        out.push_back(b);
      }
    }
  }
  else {
     for (size_t k = 0; k < input.length(); k++) {
      unsigned b = input[k];
      if (b < 32 || b == '*' || b == '?' || b==':' || b=='/' || b == '\\')
        b = '_';
      out.push_back(b);
     }
  }
  return out;
}

void capitalize(string &str) {
  if (str.length() > 0) {
    char c = str[0] & 0xFF;

    if (c>='a' && c<='z')
      c += ('A' - 'a');
    else if (c == 'ö')
      c = 'Ö';
    else if (c == 'ä')
      c = 'Ä';
    else if (c == 'å')
      c = 'Å';
    else if (c == 'é')
      c = 'É';

    str[0] = c;
  }
}

/** Return bias in seconds. UTC = local time + bias. */
int getTimeZoneInfo(const string &date) {
  static char lastDate[16] = {0};
  static int lastValue = -1;
  // Local cacheing
  if (lastValue != -1 && lastDate == date) {
    return lastValue;
  }
  strcpy_s(lastDate, 16, date.c_str());
//  TIME_ZONE_INFORMATION tzi;
  SYSTEMTIME st;
  convertDateYMS(date, st);
  st.wHour = 12;
  SYSTEMTIME utc;
  TzSpecificLocalTimeToSystemTime(0, &st, &utc);

  int datecode = ((st.wYear * 12 + st.wMonth) * 31) + st.wDay;
  int datecodeUTC = ((utc.wYear * 12 + utc.wMonth) * 31) + utc.wDay;

  int daydiff = 0;
  if (datecodeUTC > datecode)
    daydiff = 1;
  else if (datecodeUTC < datecode)
    daydiff = -1;

  int t = st.wHour * 3600;
  int tUTC = daydiff * 24 * 3600 + utc.wHour * 3600 + utc.wMinute * 60 + utc.wSecond;

  lastValue = tUTC - t;
  return lastValue;
}

string getTimeZoneString(const string &date) {
  int a = getTimeZoneInfo(date);
  if (a == 0)
    return "+00:00";
  else if (a>0) {
    char bf[12];
    sprintf_s(bf, "-%02d:%02d", a/3600, (a/60)%60);
    return bf;
  }
  else {
    char bf[12];
    sprintf_s(bf, "+%02d:%02d", a/-3600, (a/-60)%60);
    return bf;
  }
}

bool compareBib(const string &b1, const string &b2) {
  int l1 = b1.length();
  int l2 = b2.length();
  if (l1 != l2)
    return l1 < l2;

  unsigned char maxc = 0, minc = 255;
  for (int k = 0; k < l1; k++) {
    unsigned char b = b1[k];
    maxc = max(maxc, b);
    minc = min(minc, b);
  }
  for (int k = 0; k < l2; k++) {
    unsigned char b = b2[k];
    maxc = max(maxc, b);
    minc = min(minc, b);
  }

  unsigned coeff = maxc-minc + 1;

  unsigned z1 = 0;
  for (int k = 0; k < l1; k++) {
    unsigned char b = b1[k]-minc;
    z1 = coeff * z1 + b;
  }

  unsigned z2 = 0;
  for (int k = 0; k < l2; k++) {
    unsigned char b = b2[k]-minc;
    z2 = coeff * z2 + b;
  }

  return z1 < z2;
}

/// Split a name into first name and last name
int getNameSplitPoint(const string &name) {
  int split[10];
  int nSplit = 0;

  for (unsigned k = 1; k + 1<name.size(); k++) {
    if (name[k] == ' ' || name[k] == 0xA0) {
      split[nSplit++] = k;
      if ( nSplit>=9 )
        break;
    }
  }

  if (nSplit == 1)
    return split[0] + 1;
  else if (nSplit == 0)
    return -1;
  else {
    const oWordList &givenDB = lang.get().getGivenNames();
    int sp_ix = 0;
    for (int k = 1; k<nSplit; k++) {
      string sn = name.substr(split[k-1]+1, split[k] - split[k-1]-1);
      if (!givenDB.lookup(sn.c_str()))
        break;
      sp_ix = k;
    }
    return split[sp_ix]+1;
  }
}

string getGivenName(const string &name) {
  int sp = getNameSplitPoint(name);
  if (sp == -1)
    return trim(name);
  else
    return trim(name.substr(0, sp));
}

string getFamilyName(const string &name) {
  int sp = getNameSplitPoint(name);
  if (sp == -1)
    return _EmptyString;
  else
    return trim(name.substr(sp));
}
