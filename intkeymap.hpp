#pragma once

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

template<class T> class intkeymap {
private:
  struct keypair {
    int key;
    T value;
  };
  T dummy;
  T tmp;
  keypair *keys;
  unsigned siz;
  unsigned used;
  intkeymap *next;
  intkeymap *parent;
  double allocFactor;
  T noValue;
  unsigned hash1;
  unsigned hash2;
  int level;
  static int optsize(int arg);

  T &rehash(int size, int key, const T &value);
  T &get(const int key);

  const intkeymap &operator=(const intkeymap &co);
  void *lookup(int key) const;
public:
  virtual ~intkeymap();
  intkeymap(int size);
  intkeymap();
  intkeymap(const intkeymap &co);

  bool empty() const;
  int size() const;
  int getAlloc() const {return siz;}
  void clear();

  void resize(int size);
  int count(int key) {
    return lookup(key, dummy) ? 1:0;
  }
  bool lookup(int key, T &value) const;

  void insert(int key, const T &value);
  void remove(int key);
  void erase(int key) {remove(key);}
  const T operator[](int key) const
    {if (lookup(key,tmp)) return tmp; else return T();}

  T &operator[](int key) {
    return get(key);
  }
};
