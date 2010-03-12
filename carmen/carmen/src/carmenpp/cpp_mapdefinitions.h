#ifndef CARMEN_CPP_MAP_DEFINITIONS_H
#define CARMEN_CPP_MAP_DEFINITIONS_H

#include <carmen/cpp_genericmap.h>


class CharCell {
public:
  CharCell(char v=-1) { set(v); }
  CharCell(const CharCell& x) { *this = x; }
  void set(char v) { val = v; }
  
  carmen_inline operator char() const { return val;}
  carmen_inline CharCell& operator= (const char& v)  { val = v; return *this;}
  
  char val;
};


class IntCell {
public:
  IntCell(int v=-1) { set(v); }
  IntCell(const IntCell& x) { *this = x; }
  void set(int v) { val = v; }
  
  carmen_inline operator int() const { return val;}
  carmen_inline IntCell& operator= (const int& v)  { val = v; return *this;}
  
  int val;
};

class FloatCell {
public:
  FloatCell(float v = -1.0) { set(v); }
  FloatCell(const FloatCell& x) { *this = x; }
  void set(float v) { val = v; }

  carmen_inline operator float() const { return val;}
  carmen_inline FloatCell& operator= (const float& v)  { val = v; return *this;}
  
  float val;
};

class DoubleCell {
public:
  DoubleCell(double v = -1.0) { set(v); }
  DoubleCell(const DoubleCell& x) { *this = x; }
  void set(double v) { val = v; }

  carmen_inline operator double() const { return val;}
  carmen_inline DoubleCell& operator= (const double& v)  { val = v; return *this;}
  
  double val;
};

class RefProbCell {
public:
  RefProbCell(int hits=0, int misses=0) { set(hits, misses); }
  RefProbCell(const RefProbCell& x) {  *this = x;  }
  
  void set(int hits, int misses) { 
    this->hits = hits;  
    this->misses = misses; 
    this->val = -1; 
    if (hits+misses > 0) {
      val = ((float)hits)/((float) (hits+misses));
    }
    updated = false;
  }
	   
  void hit() { 
    hits++;
    updated = true;
  }

  void miss() { 
    misses++;
    updated = true;
    val = ((float)hits)/((float) (hits+misses));
  }

  carmen_inline operator float() {  
    if (updated) {
      val = ((float)hits)/((float) (hits+misses));
      updated = false;
    }
    return val;  
  }
  carmen_inline operator double() {      
    if (updated) {
      val = ((float)hits)/((float) (hits+misses));
      updated = false;
    }
    return val;  
  }
  
  int hits;
  int misses;
  float val;
  bool updated;
};

class DoubleRefProbCell {
public:
  DoubleRefProbCell(double hits=0, double misses=0) { set(hits, misses); }
  DoubleRefProbCell(const DoubleRefProbCell& x) {  *this = x;  }
  
  void set(double hits, double misses) { 
    this->hits = hits;  
    this->misses = misses; 
    this->val = -1; 
    if (hits+misses > 0) {
      val = hits/(hits+misses);
    }
    updated = false;
  }
	   
  void hit(double amount = 1.0) { 
    hits += amount;
    if (amount != 0)
      updated = true;
  }

  void miss(double amount = 1.0) { 
    misses += amount;
    if (amount != 0)
      updated = true;
  }

  inline operator float() {  
    if (updated) {
      val = hits/(hits+misses);
      updated = false;
    }
    return val;  
  }
  inline operator double() {      
    if (updated) {
      val = hits/(hits+misses);
      updated = false;
    }
    return val;  
  }
  
  double hits;
  double misses;
  double val;
  bool updated;
};


class IntPointCell {
public:
  IntPointCell(IntPoint v = IntPoint(-1,-1)) { set(v); }
  IntPointCell(const IntPointCell& x) { *this = x; }
  void set(IntPoint v) { val = v; }
  
  carmen_inline operator IntPoint() const { return val;}
  carmen_inline IntPointCell& operator= (const IntPoint& v)  { val = v; return *this;}
  
  IntPoint val;
};



typedef GenericMap<CharCell>     CharMap;
typedef GenericMap<IntCell>      IntMap;
typedef GenericMap<FloatCell>    FloatMap;
typedef GenericMap<DoubleCell>   DoubleMap;
typedef GenericMap<RefProbCell>  RefProbMap;
typedef GenericMap<DoubleRefProbCell>  DoubleRefProbMap;
typedef GenericMap<IntPointCell> IntPointMap;

#endif
