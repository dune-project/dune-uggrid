// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/************************************************************************/
/*                                                                      */
/* This file is a part of NETGEN                                        */
/*                                                                      */
/* File:   ruler3.cc                                                    */
/* Author: Joachim Schoeberl                                            */
/*                                                                      */
/************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <fstream.h>
#include <math.h>

#include <template.hh>
#include <array.hh>

#include <linalg/linalg.hh>
#include <geom/geom2d.hh>
#include <geom/geom3d.hh>
#include <limits.h>

#include <meshing/global.hh>
#include <meshing/ruler3.hh>

static double CalcElementBadness (const ARRAY<POINT3D> & points,
                                  const ELEMENT & elem)
{
  double vol, l, l4, l5, l6;

  VEC3D v1 = points[elem.PNum(2)] - points[elem.PNum(1)];
  VEC3D v2 = points[elem.PNum(3)] - points[elem.PNum(1)];
  VEC3D v3 = points[elem.PNum(4)] - points[elem.PNum(1)];

  vol = - (Cross (v1, v2) * v3);
  l4 = Dist (points[elem.PNum(2)], points[elem.PNum(3)]);
  l5 = Dist (points[elem.PNum(2)], points[elem.PNum(4)]);
  l6 = Dist (points[elem.PNum(3)], points[elem.PNum(4)]);

  l = v1.Length() + v2.Length() + v3.Length() + l4 + l5 + l6;

  //  testout << "vol = " << vol << " l = " << l << endl;
  if (vol < 1e-8) return 1e10;
  //  (*testout) << "l^3/vol = " << (l*l*l / vol) << endl;

  return l*l*l/vol;
}


int ApplyVRules (const ARRAY<vnetrule*> & rules, double tolfak,
                 ARRAY<POINT3D> & lpoints, ARRAY<ELEMENT> & lfaces,
                 ARRAY<ELEMENT> & elements,
                 ARRAY<INDEX> & delfaces, int tolerance, int rotind1,
                 float & retminerr, ARRAY<char*> & problems)
{
  const int rotsym = 3;
  int i, j, k, ri, nfok, npok, incnpok, refpi, locpi, locfi, locfr;
  int hi, minn, hpi;
  float hf, err, minerr, teterr, minteterr;
  char ok, found, hc;
  vnetrule * rule;
  VECTOR oldu, newu;
  VEC3D ui;
  POINT3D np;
  int oldnp, noldlp, noldlf;
  const ELEMENT * locface = NULL;
  int loktestmode;

  static ARRAY<int> pused, fused;
  static ARRAY<int> pmap, pfixed;
  static ARRAY<int> fmapi, fmapr;
  static ARRAY<POINT3D> transfreezone;
  static INDEX cnt = 0;

  static ARRAY<POINT3D> tempnewpoints;
  static ARRAY<ELEMENT> tempnewfaces;
  static ARRAY<int> tempdelfaces;
  static ARRAY<ELEMENT> tempelements;

  static ARRAY<int> notnewnewfaces;

  static ARRAY<int> pnearness, fnearness;

  cnt++;

  delfaces.SetSize (0);
  elements.SetSize (0);

  pnearness.SetSize (lpoints.Size());
  fnearness.SetSize (lfaces.Size());

  for (i = 1; i <= pnearness.Size(); i++)
    pnearness.Set(i, INT_MAX);

  for (j = 1; j <= 3; j++)
    pnearness.Set(lfaces.Get(1).PNum(j), 0);

  do
  {
    ok = 1;

    for (i = 1; i <= lfaces.Size(); i++)
    {
      minn = INT_MAX-1;
      for (j = 1; j <= 3; j++)
      {
        hi = pnearness.Get(lfaces.Get(i).PNum(j));
        if (hi < minn) minn = hi;
      }

      for (j = 1; j <= 3; j++)
      {
        hpi = lfaces.Get(i).PNum(j);
        if (pnearness.Get(hpi) > minn+1)
        {
          ok = 0;
          pnearness.Set(hpi, minn+1);
        }
      }
    }
  }
  while (!ok);

  for (i = 1; i <= fnearness.Size(); i++)
  {
    fnearness.Set(i, 0);
    for (j = 1; j <= 3; j++)
      fnearness.Elem(i) += pnearness.Get(lfaces.Get(i).PNum(j));
  }





  pused.SetSize (lpoints.Size());
  fused.SetSize (lfaces.Size());



  found = 0;
  minerr = tolfak * tolerance * tolerance;
  minteterr = 500 * tolerance * tolerance;

  if (testmode)
    (*testout) << "cnt = " << cnt << " class = " << tolerance << endl;


  // check every rule:

  for (ri = 1; ri <= rules.Size(); ri++)
  {
    rule = rules.Get(ri);

    if (rule->GetQuality() > tolerance)
    {
      if (testmode)
        sprintf (problems[ri], "Quality not ok");
      continue;
    }

    if (testmode)
      sprintf (problems[ri], "no mapping found");

    loktestmode = testmode || rule->TestFlag ('t');

    if (loktestmode)
      (*testout) << "Rule " << ri << " = " << rule->Name() << endl;

    pmap.SetSize (rule->GetNP());
    fmapi.SetSize (rule->GetNF());
    fmapr.SetSize (rule->GetNF());

    for (i = 1; i <= lfaces.Size(); i++)
      fused.Set (i, 0);
    for (i = 1; i <= lpoints.Size(); i++)
      pused.Set (i, 0);
    for (i = 1; i <= pmap.Size(); i++)
      pmap.Set(i, 0);
    for (i = 1; i <= fmapi.Size(); i++)
      fmapi.Set(i, 0);
    for (i = 1; i <= fmapr.Size(); i++)
      fmapr.Set(i, 3);

    fused.Set (1, 1);

    fmapi.Set (1, 1);
    fmapr.Set (1, rotind1);

    //    (*testout) << "Face1, rotind1 = " << rotind1 << endl;
    for (j = 1; j <= 3; j++)
    {
      locpi = lfaces.Get(1).PNumMod (j+rotind1);
      pmap.Set (rule->GetPointNr (1, j), locpi);
      pused.Elem(locpi)++;
      //      (*testout) << "ref: " << rule->GetPoint (rule->GetPointNr (1, j))
      //              << "   loc: " << lpoints.Get(pmap.Get(rule->GetPointNr(1, j))) << endl;
    }

    nfok = 2;
    while (nfok >= 2)
    {

      if (nfok <= rule->GetNOldF())
      {
        ok = 0;
        locfi = fmapi.Get(nfok);
        locfr = fmapr.Get(nfok);

        while (!ok)
        {
          locfr++;
          if (locfr == rotsym + 1)
          {
            locfr = 1;
            locfi++;
            if (locfi > lfaces.Size()) break;
          }


          if (fnearness.Get(locfi) > rule->GetFNearness (nfok) ||
              fused.Get(locfi) )
          {
            locfr = rotsym;
          }
          else
          {

            ok = 1;

            locface = &lfaces.Get(locfi);

            for (j = 1; j <= locface->NP() && ok; j++)
            {
              locpi = pmap.Get(rule->GetPointNr (nfok, j));

              if (locpi && locpi != locface->PNumMod(j+locfr))
                ok = 0;
            }

            for (j = 1; j <= locface->NP() && ok; j++)
            {
              refpi = rule->GetPointNr (nfok, j);

              if (pmap.Get(refpi) == 0)
              {
                locpi = locface->PNumMod (j + locfr);
                if (pused.Get(locpi) ||
                    rule->CalcPointDist (refpi, lpoints.Get(locpi)) > minerr)
                  ok = 0;
              }
            }
          }
        }


        if (ok)
        {
          fmapi.Set (nfok, locfi);
          fmapr.Set (nfok, locfr);
          fused.Set (locfi, 1);

          for (j = 1; j <= rule->GetNP (nfok); j++)
          {
            locpi = locface->PNumMod(j+locfr);

            if (rule->GetPointNr (nfok, j) <= 3 && pmap.Get(rule->GetPointNr(nfok, j)) != locpi)
              (*testout) << "change face1 point, mark1" << endl;

            pmap.Set(rule->GetPointNr (nfok, j), locpi);
            pused.Elem(locpi)++;
          }

          nfok++;
        }
        else
        {
          fmapi.Set (nfok, 0);
          fmapr.Set (nfok, 3);
          nfok--;

          fused.Set (fmapi.Get(nfok), 0);
          for (j = 1; j <= rule->GetNP (nfok); j++)
          {
            refpi = rule->GetPointNr (nfok, j);
            pused.Elem(pmap.Get(refpi))--;

            if (pused.Get(pmap.Get(refpi)) == 0)
            {
              pmap.Set(refpi, 0);
            }
          }
        }
      }

      else

      {
        // all faces are mapped
        // now map all points:

        if (loktestmode)
        {
          (*testout) << "Faces Ok" << endl;
          sprintf (problems[ri], "Faces Ok");
        }

        npok = 1;
        incnpok = 1;

        pfixed.SetSize (pmap.Size());
        for (i = 1; i <= pmap.Size(); i++)
          pfixed.Set(i, (pmap.Get(i) != 0) );

        while (npok >= 1)
        {

          if (npok <= rule->GetNOldP())
          {

            if (pfixed.Get(npok))

            {
              if (incnpok)
                npok++;
              else
                npok--;
            }

            else

            {
              locpi = pmap.Elem(npok);
              ok = 0;

              if (locpi)
                pused.Elem(locpi)--;

              while (!ok && locpi < lpoints.Size())
              {
                ok = 1;
                locpi++;

                if (pused.Get(locpi))
                {
                  ok = 0;
                }
                else
                {
                  if (rule->CalcPointDist (npok, lpoints.Get(locpi)) > minerr)
                    ok = 0;
                }
              }


              if (ok)
              {
                pmap.Set (npok, locpi);

                if (npok <= 3)
                  (*testout) << "set face1 point, mark3" << endl;

                pused.Elem(locpi)++;
                npok++;
                incnpok = 1;
              }

              else

              {
                pmap.Set (npok, 0);

                if (npok <= 3)
                  (*testout) << "set face1 point, mark4" << endl;

                npok--;
                incnpok = 0;
              }
            }
          }

          else

          {

            // all points are mapped

            if (loktestmode)
            {
              (*testout) << "Mapping found!!: Rule " << rule->Name() << endl;
              sprintf (problems[ri], "mapping found");
            }

            ok = 1;


            // check mapedges:


            for (i = 1; i <= rule->GetNEd(); i++)
            {
              int ok2, i1, i2;
              ok2 = 0;
              i1 = pmap[rule->GetEdge(i).i1];
              i2 = pmap[rule->GetEdge(i).i2];

              for (j = 1; j <= lfaces.Size(); j++)
                for (k = 1; k <= 3; k++)
                  if (lfaces[j].PNumMod(k) == i1 &&
                      lfaces[j].PNumMod(k+1) == i2)
                    ok2 = 1;

              if (!ok2) ok = 0;
            }



            for (i = rule->GetNOldF() + 1; i <= rule->GetNF(); i++)
              fmapi.Set(i, 0);


            /*
                        for (i = rule->GetNOldF() + 1; i <= rule->GetNF(); i++)
                          {
                          fmapii[i] = 0;

                          if (rule->GetPointNr(i, 1) <= rule->GetNOldP() &&
                              rule->GetPointNr(i, 2) <= rule->GetNOldP() &&
                              rule->GetPointNr(i, 3) <= rule->GetNOldP())
                            {
                            hc = 0;
                            for (k = 1; k <= lfaces.Size() && !hc; k++)
                              if (!fused.Get(k))
                                for (j = 1; j <= 3; j++)
                                  if (lfaces.Get(k).PNumMod(j  ) == pmap.Get(rule->GetPointNr(i, 1)) &&
                                      lfaces.Get(k).PNumMod(j+1) == pmap.Get(rule->GetPointNr(i, 3)) &&
                                      lfaces.Get(k).PNumMod(j+2) == pmap.Get(rule->GetPointNr(i, 2)))
                                    {
                                    hc = 1;
                                    notnewnewfaces.Append (k);
                                    fused.Set(k, 1);
                                    fmapii[i] = k;
                                    }
                            }
                          }
             */


            oldu.SetLength (3 * rule->GetNOldP());

            for (i = 1; i <= rule->GetNOldP(); i++)
            {
              ui = lpoints.Get(pmap.Get(i)) - rule->GetPoint(i);
              oldu.Set (3*i-2, ui.X());
              oldu.Set (3*i-1, ui.Y());
              oldu.Set (3*i  , ui.Z());
            }

            rule->GetOldUToFreeZone().Mult (oldu, newu);
            rule->SetFreeZoneTransformation (newu);

            if (!rule->ConvexFreeZone())
            {
              if (loktestmode)
              {
                (*testout) << "Freezone not convex" << endl;
                /*
                                for (i = 1; i <= pmap.Size(); i++)
                                (*testout) << "pmap(" << i << ") = " << pmap[i] << endl;
                                for (i = 1; i <= lpoints.Size(); i++)
                                (*testout) << "p" << i << "= " << lpoints[i] << endl;
                 */
                sprintf (problems[ri], "Freezone not convex");
              }
              ok = 0;
            }


            // check freezone:

            for (i = 1; i <= lpoints.Size() && ok; i++)
            {
              if ( !pused.Get(i) && rule->IsInFreeZone(lpoints.Get(i)))
              {
                if (loktestmode)
                {
                  (*testout) << "Point " << i << " in Freezone" << endl;
                  sprintf (problems[ri], "locpoint %d in Freezone", i);
                }
                ok = 0;
              }
            }



            for (i = 1; i <= lfaces.Size() && ok; i++)
            {
              if (!fused[i])
              {
                int triin = rule->IsTriangleInFreeZone (
                  lpoints.Get(lfaces.Get(i).PNum(1)),
                  lpoints.Get(lfaces.Get(i).PNum(2)),
                  lpoints.Get(lfaces.Get(i).PNum(3)) );

                if (triin == -1)
                {
                  //                  (*testout) << "Rule: " << rule->Name() << endl;
                  //                  (*testout) << "Triangle: can't decide" << endl;
                  ok = 0;
                }

                if (triin == 1)
                {
                  hc = 0;
                  for (k = rule->GetNOldF() + 1; k <= rule->GetNF(); k++)
                  {
                    if (rule->GetPointNr(k, 1) <= rule->GetNOldP() &&
                        rule->GetPointNr(k, 2) <= rule->GetNOldP() &&
                        rule->GetPointNr(k, 3) <= rule->GetNOldP())
                    {
                      for (j = 1; j <= 3; j++)
                        if (lfaces.Get(i).PNumMod(j  ) == pmap.Get(rule->GetPointNr(k, 1)) &&
                            lfaces.Get(i).PNumMod(j+1) == pmap.Get(rule->GetPointNr(k, 3)) &&
                            lfaces.Get(i).PNumMod(j+2) == pmap.Get(rule->GetPointNr(k, 2)))
                        {
                          fmapi[k] = i;
                          hc = 1;
                        }
                    }
                  }

                  if (!hc)
                  {
                    if (loktestmode)
                    {
                      (*testout) << "Triangle in freezone" << endl;
                      sprintf (problems[ri], "triangle (%d, %d, %d) in Freezone",
                               lfaces.Get(i).PNum(1), lfaces.Get(i).PNum(2),
                               lfaces.Get(i).PNum(3));
                    }
                    ok = 0;
                  }
                }
              }
            }


            if (ok)
            {

              err = 0;
              for (i = 1; i <= rule->GetNOldP(); i++)
              {
                hf = rule->CalcPointDist (i, lpoints[pmap[i]]);
                if (hf > err) err = hf;
              }


              if (loktestmode)
              {
                (*testout) << "Rule ok" << endl;
                sprintf (problems[ri], "Rule ok, err = %f", err);
              }

              newu = rule->GetOldUToNewU() * oldu;

              // set new points:

              oldnp = rule->GetNOldP();
              noldlp = lpoints.Size();
              noldlf = lfaces.Size();


              for (i = oldnp + 1; i <= rule->GetNP(); i++)
              {
                np = rule->GetPoint(i);
                np.X() += newu (3 * (i-oldnp) - 2);
                np.Y() += newu (3 * (i-oldnp) - 1);
                np.Z() += newu (3 * (i-oldnp));

                pmap[i] = lpoints.Append (np);
              }

              // Set new Faces:

              for (i = rule->GetNOldF() + 1; i <= rule->GetNF(); i++)
                if (!fmapi[i])
                {
                  ELEMENT nface;
                  nface.SetNP(3);
                  for (j = 1; j <= 3; j++)
                    nface.PNum(j) = pmap[rule->GetPointNr (i, j)];

                  lfaces.Append (nface);
                }


              // Delete old Faces:

              for (i = 1; i <= rule->GetNDelF(); i++)
                delfaces.Append (fmapi.Get(rule->GetDelFace(i)));
              for (i = rule->GetNOldF()+1; i <= rule->GetNF(); i++)
                if (fmapi.Get(i))
                {
                  delfaces.Append (fmapi[i]);
                  fmapi[i] = 0;
                }


              for (i = 1; i <= rule->GetNO() && ok; i++)
              {
                const /* vnetrule::*/ fourint * fouri;
                double v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;

                fouri = &rule->GetOrientation(i);

                v1x = lpoints.Get(pmap.Get(fouri->i2)).X() -
                      lpoints.Get(pmap.Get(fouri->i1)).X();
                v1y = lpoints.Get(pmap.Get(fouri->i2)).Y() -
                      lpoints.Get(pmap.Get(fouri->i1)).Y();
                v1z = lpoints.Get(pmap.Get(fouri->i2)).Z() -
                      lpoints.Get(pmap.Get(fouri->i1)).Z();

                v2x = lpoints.Get(pmap.Get(fouri->i3)).X() -
                      lpoints.Get(pmap.Get(fouri->i1)).X();
                v2y = lpoints.Get(pmap.Get(fouri->i3)).Y() -
                      lpoints.Get(pmap.Get(fouri->i1)).Y();
                v2z = lpoints.Get(pmap.Get(fouri->i3)).Z() -
                      lpoints.Get(pmap.Get(fouri->i1)).Z();

                v3x = lpoints.Get(pmap.Get(fouri->i4)).X() -
                      lpoints.Get(pmap.Get(fouri->i1)).X();
                v3y = lpoints.Get(pmap.Get(fouri->i4)).Y() -
                      lpoints.Get(pmap.Get(fouri->i1)).Y();
                v3z = lpoints.Get(pmap.Get(fouri->i4)).Z() -
                      lpoints.Get(pmap.Get(fouri->i1)).Z();


                if (v1x * v2y * v3z +
                    v1y * v2z * v3x +
                    v1z * v2x * v3y -
                    v1x * v2z * v3y -
                    v1y * v2x * v3z -
                    v1z * v2y * v3x > -1E-7)
                {
                  if (loktestmode)
                  {
                    sprintf (problems[ri], "Orientation wrong");
                    (*testout) << "Orientation wrong" << endl;
                  }
                  ok = 0;
                }
              }

              for (i = rule->GetNOldP() + 1; i <= rule->GetNP() && ok; i++)
                if (!rule->IsInFreeZone (lpoints[pmap[i]]))
                {
                  if (loktestmode)
                    (*testout) << "Newpoint " << lpoints[pmap[i]] << " outside convex hull" << endl;
                  ok = 0;
                }

              // insert new elements

              for (i = 1; i <= rule->GetNE(); i++)
              {
                elements.Append (rule->GetElement(i));
                for (j = 1; j <= elements[i].NP(); j++)
                  elements[i].PNum(j) = pmap[elements[i].PNum(j)];
              }


              // Calculate Element badness

              teterr = 0;
              for (i = 1; i <= elements.Size(); i++)
              {
                hf = CalcElementBadness (lpoints, elements[i]);
                if (hf > teterr) teterr = hf;
              }


              if (ok && teterr < minteterr)
              {
                found = ri;
                minteterr = teterr;

                if (testmode)
                {
                  for (i = 1; i <= rule->GetNOldP(); i++)
                  {
                    (*testout) << "P" << i << ": Ref: "
                               << rule->GetPoint (i) << "  is: "
                               << lpoints[pmap[i]] << endl;
                  }
                }

                tempnewpoints.SetSize (0);
                for (i = noldlp+1; i <= lpoints.Size(); i++)
                  tempnewpoints.Append (lpoints[i]);

                tempnewfaces.SetSize (0);
                for (i = noldlf+1; i <= lfaces.Size(); i++)
                  tempnewfaces.Append (lfaces[i]);

                tempdelfaces.SetSize (0);
                for (i = 1; i <= delfaces.Size(); i++)
                  tempdelfaces.Append (delfaces[i]);

                tempelements.SetSize (0);
                for (i = 1; i <= elements.Size(); i++)
                  tempelements.Append (elements[i]);
              }

              lpoints.SetSize (noldlp);
              lfaces.SetSize (noldlf);
              delfaces.SetSize (0);
              elements.SetSize (0);
            }

            npok = rule->GetNOldP();
            incnpok = 0;
          }
        }

        nfok = rule->GetNOldF();

        for (j = 1; j <= rule->GetNP (nfok); j++)
        {
          refpi = rule->GetPointNr (nfok, j);
          pused.Elem(pmap.Get(refpi))--;

          if (pused.Get(pmap.Get(refpi)) == 0)
          {
            pmap.Set(refpi, 0);
          }
        }

      }
    }
  }

  if (found)
  {
    for (i = 1; i <= tempnewfaces.Size(); i++)
    {
      hc = 1;
      for (k = 1; k <= lfaces.Size() && hc; k++)
        for (j = 1; j <= 3 && hc; j++)
          if (tempnewfaces[i].PNumMod(j  ) == lfaces[k].PNum(1) &&
              tempnewfaces[i].PNumMod(j+1) == lfaces[k].PNum(3) &&
              tempnewfaces[i].PNumMod(j+2) == lfaces[k].PNum(2))
          {
            tempdelfaces.Append(k);
            tempnewfaces[i].PNum(1) = 0;
            hc = 0;
          }
    }


    for (i = 1; i <= tempnewpoints.Size(); i++)
      lpoints.Append (tempnewpoints[i]);
    for (i = 1; i <= tempnewfaces.Size(); i++)
      if (tempnewfaces[i].PNum(1))
        lfaces.Append (tempnewfaces[i]);
    for (i = 1; i <= tempdelfaces.Size(); i++)
      delfaces.Append (tempdelfaces[i]);
    for (i = 1; i <= tempelements.Size(); i++)
      elements.Append (tempelements[i]);
  }

  retminerr = minerr;
  return found;
}
