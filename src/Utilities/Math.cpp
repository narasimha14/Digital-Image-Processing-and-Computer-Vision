/* 
 * Copyright (c) 2004 Clemson University.
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

#include "Math.h"
#include "../../external/WinGsl/WinGsl.h"
#include <time.h>  // time
#include <assert.h>
#include <afx.h>
#include <afxwin.h>

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

// ================> begin local functions (available only to this translation unit)
namespace{
  bool g_rand_seeded = false;
  bool g_rand_seeded2 = false;
  class gslRandomNumberGenerator
  {
  public:
    gslRandomNumberGenerator() : m_rng(NULL) 
    {
      // mt19937 is the default random number generator of GSL
      m_rng = gsl_rng_alloc (gsl_rng_mt19937);
      // If you want to be able to set the default using the environment variables, then call these two lines instead:
      //      gsl_rng_env_setup();
      //      m_rng = gsl_rng_alloc (gsl_rng_default);
    }
    ~gslRandomNumberGenerator() { gsl_rng_free(m_rng); }
    operator gsl_rng*() const { return m_rng; }
  private:
    gsl_rng* m_rng;
  };
  gslRandomNumberGenerator g_rng;  // global random number generator
};
// ================< end local functions

//namespace blepo_ex {

int blepo_ex::GetRand(unsigned int modulus)
{
  // seed the random number generator, if it hasn't been yet
  if (!g_rand_seeded) {
   	srand(static_cast<unsigned int>(time(NULL)));
  	g_rand_seeded = true;
  }

  // return the random number
  assert(modulus <= RAND_MAX+1);
	return rand() % modulus;
}

int blepo_ex::GetRand(int low, int high)
{
  assert(high>low);
  return low + GetRand(static_cast<unsigned int>(high-low));
}

double blepo_ex::GetRandDbl()
{
  // seed the random number generator, if it hasn't been yet
  if (!g_rand_seeded2) {
    gsl_rng_set (g_rng, static_cast<unsigned int>(time(NULL)));
  	g_rand_seeded2 = true;
  }

  return gsl_rng_uniform(g_rng);
}

const double blepo_ex::Pi = 3.141592653589793238462643383279502884197169399375105;
const float blepo_ex::Pif = 3.141592653589793238f;

//}; // end namespace blepo_ex


