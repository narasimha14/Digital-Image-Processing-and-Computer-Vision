/* 
 * Copyright (c) 2004,2005,2008 Clemson University.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __BLEPO_EQUIVALENCE_TABLE_H__
#define __BLEPO_EQUIVALENCE_TABLE_H__

//#include "ImageAlgorithms.h"
//#include "Utilities/Math.h"
#include <vector>

namespace blepo {

/**
  Equivalence table for connected components.  Equivalences are stored in a directional
  fashion, with labels with higher numbers always pointing to labels with lower numbers.

  Helpful description of algorithm can be found at http://en.wikipedia.org/wiki/Disjoint-set_data_structure

  @author Stan Birchfield (STB)
*/

class EquivalenceTable
{
public:
  EquivalenceTable() {}

  /// Call this whenever two labels have been found to be equivalent
  void AddEquivalence(int label1, int label2)
  {
    if (label1 == label2)  return;

    int a = GetEquivalentLabelRecursive( label1 );
    int b = GetEquivalentLabelRecursive( label2 );

    if      (a == b)  return;
    else if (a >  b)  { EnsureAllocated(a);  m_table[a] = b; }
    else              { EnsureAllocated(b);  m_table[b] = a; }
  }

  /// traverse links; after this step each label either points to itself
  /// or points to another label that points to itself
  void TraverseLinks()
  {
    for (int i = 0 ; i < (int) m_table.size() ; i++)
    {
      m_table[i] = GetEquivalentLabelRecursive( m_table[i] );
    }
  }

  /// Condense table by removing gaps.
  /// After calling this function, the labels are sequential:  0, 1, 2, ...
  /// Returns the number of labels (i.e., the maximum label + 1)
  int RemoveGaps()
  {
    int next = 0;
    for (int a = 0 ; a < (int) m_table.size() ; a++) 
    {
      int b = m_table[a];
      m_table[a] = (a == b) ? next++ : m_table[b];
    }
    return next;
  }

  /// Return the equivalent label.
  /// Call TraverseLinks() before calling this function.
  int GetEquivalentLabel(int label) const 
  {
    assert( label >= 0 );
    return (label < (int) m_table.size()) ? m_table[label] : label;
  }

  /// Ensure that table is initialized at least up to and including 'label'
  void EnsureAllocated(int label)
  {
    assert( label >= 0 );
    int n = (int) m_table.size();
    for (int i=n ; i<=label ; i++)  m_table.push_back(i);
  }

  /// Return the smallest equivalent label, recursively
  int GetEquivalentLabelRecursive(int label) const 
  {
    assert( label >= 0 );

    // cast away constness so that internal data structure can be updated for efficiency
    EquivalenceTable* me = (const_cast<EquivalenceTable*>(this));  
    me->EnsureAllocated( label );

    if (label == m_table[label])  return label;
    else
    {
      me->m_table[label] = GetEquivalentLabelRecursive( m_table[label] );
      return m_table[label];
    }
  }

private:
  std::vector<int> m_table;
};

/*
// This code works, but is not as simple as it could be.  -- STB 9/1/08
class EquivalenceTable
{
public:
  EquivalenceTable() {}

  /// Call this whenever two labels have been found to be equivalent
  void AddEquivalence(int label1, int label2)
  {
    if (label1 == label2)  return;

    // sort labels
    int a = blepo_ex::Max(label1, label2);
    int b = blepo_ex::Min(label1, label2);
    
    // allocate memory if needed, and initialize m_table[i] = i
    EnsureAllocated(a);

    // add equivalence; 
    // if 'a' already pointed to another label higher than 'b', then recurse
    int a_equiv = m_table[a];
    m_table[a] = blepo_ex::Min(a_equiv, b);
    if (a_equiv != a)  AddEquivalence(a_equiv, b);
  }

  /// traverse links; after this step each label either points to itself
  /// or points to another label that points to itself
  void TraverseLinks()
  {
    for (int i=0 ; i<(int)m_table.size() ; i++)
    {
      m_table[i] = GetEquivalentLabelRecursive(m_table[i]);
    }
  }

  /// Condense table by removing gaps.
  /// After calling this function, the labels are sequential:  0, 1, 2, ...
  /// Returns the number of labels (i.e., the maximum label + 1)
  int RemoveGaps()
  {
    int next = 0;
    for (int a = 0 ; a < (int)m_table.size() ; a++) 
    {
      int b = m_table[a];
      m_table[a] = (a == b) ? next++ : m_table[b];
    }
    return next;
  }

  /// Return the equivalent label.
  /// Call TraverseLinks() before calling this function.
  int GetEquivalentLabel(int label) const 
  {
    assert( label >= 0 );
    return (label < (int) m_table.size()) ? m_table[label] : label;
  }

  /// Ensure that table is initialized at least up to and including 'label'
  void EnsureAllocated(int label)
  {
    int n = m_table.size();
    for (int i=n ; i<=label ; i++)  m_table.push_back(i);
  }

  /// Return the smallest equivalent label, recursively
  int GetEquivalentLabelRecursive(int label) const 
  {
    assert( label >= 0 );
    int a = (label < (int) m_table.size()) ? m_table[label] : label;
    return (a == label) ? a : GetEquivalentLabelRecursive(a);
  }

private:
  std::vector<int> m_table;
};
*/
};  // end namespace blepo

#endif // __BLEPO_EQUIVALENCE_TABLE_H__
