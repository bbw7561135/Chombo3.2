#ifdef CH_LANG_CC
/*
 *      _______              __
 *     / ___/ /  ___  __ _  / /  ___
 *    / /__/ _ \/ _ \/  V \/ _ \/ _ \
 *    \___/_//_/\___/_/_/_/_.__/\___/
 *    Please refer to Copyright.txt, in Chombo's root directory.
 */
#endif

#ifndef _STLREADER_H_
#define _STLREADER_H_

#include <iostream>
#include <fstream>
using std::istream;
using std::ifstream;

#include "MayDay.H"
#include "RefCountedPtr.H"

#include "STLMesh.H"

#include "NamespaceHeader.H"

class STLReader
{
public:
  /// Constructor - read from standard input
  STLReader() {}

  /// Destructor
  virtual ~STLReader() {}

  /// Return header information
  virtual string* GetHeader() const = 0;

  /// Return number of elements
  virtual void GetNtri(int& a_ntri) const = 0;

  /// Return whether number of elements from header matches file
  virtual void GetNtriMatch(bool& a_ntriMatch) const = 0;

  /// Return pointer to the mesh
  virtual RefCountedPtr<STLMesh> GetMesh() const = 0;

protected:
  virtual void ReadData(istream& a_file,
                        const int offset) = 0;

  int  m_ntri;      // number of triangles read in
  bool m_ntriMatch; // true if m_ntri equals the number of triangles declared in the header

  // actual data - shared by all copies
  RefCountedPtr<STLMesh> m_stlmesh; // pointer to the mesh

private:
  void operator=(const STLReader& a_inputReader)
  {
    MayDay::Abort("STLReader doesn't allow assignment");
  }
};

#include "NamespaceFooter.H"
#endif
