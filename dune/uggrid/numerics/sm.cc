// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      sm.c                                                          */
/*                                                                          */
/* Purpose:   sparse matrix handling routines                               */
/*                                                                          */
/* Author:    Nicolas Neuss                                                 */
/*            email: Nicolas.Neuss@IWR.Uni-Heidelberg.De                    */
/*                                                                          */
/* History:   01.98 begin sparse matrix routines                            */
/*                                                                          */
/* Note:      The files sm.[ch], blasm.[ch] may be obtained also in a       */
/*            standalone form under the GNU General Public License.         */
/*            The use inside of UG under the actual UG license              */
/*            is allowed.                                                   */
/*                                  HD, 13.7.99,  Nicolas Neuss.            */
/*                                                                          */
/****************************************************************************/

#include <config.h>

#include <float.h>
#include <cmath>
#include <cstddef>

#include "sm.h"
#include <dune/uggrid/low/debug.h>

#define ERR_RETURN(x) REP_ERR_RETURN(x)

REP_ERR_FILE

#include <dune/uggrid/numerics/udm.h>  /* for MAX_MAT_COMP */

/****************************************************************************/
/** \brief Transforms a string to a SM array

   \param n - size (i.e. rows*cols)
   \param str - pointer to char array consisting of [*0a-z]
   \param comps - pointer to integer array

   Transforms a string to a SM array. * means a non-zero entry, 0
   means a zero entry. a-z are used for identification of positions
   (they get the same offset). It is assumed that there is enough
   space in the sparse matrix array.

   \return <ul>
   <li> 0 ok </li>
   <li> 1 wrong format </li>
   </ul>
 */
/****************************************************************************/

NS_PREFIX INT NS_DIM_PREFIX String2SMArray (SHORT n, char *str, SHORT *comps)
{
  SHORT off;
  int i;
  char c;
  SHORT pos[26];         /* offset belonging to character */

  /* reset identification field */
  for (i=0; i<26; i++)
    pos[i] = -1;

  off=0;
  for (i=0; i<n; i++)
  {
    /* skip blanks */
    while ((c=*str++) != '\0')
    {
      if (c!=' ' && c!='\t' && c!= '\n')
        break;
    }
    if (c=='\0') return(1);

    /* zero entry? */
    if (c=='0')
    {
      *comps++ = -1;
      continue;
    }

    /* standard nonzero entry? */
    if (c=='*')
    {
      *comps++ = off++;
      continue;
    }

    if ((c<'a') || (c>'z')) return(-1);

    /* identified nonzero entry */
    if (pos[c-'a']>=0)
      *comps++ = pos[c-'a'];
    else
    {
      *comps++ = off;
      pos[c-'a'] = off++;
    }
  }

  return(0);
}
