/* ----------------------------------------------------------------------
   ChemCell - Cell simulator for particle diffusion and reactions
   Steve Plimpton (sjplimp@sandia.gov), Alex Slepoy (aslepoy@sandia.gov)
   Sandia National Laboratories, www.cs.sandia.gov/~sjplimp/chemcell.html

   Copyright (2004) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the GNU General Public License.

   See the README file in the top-level ChemCell directory.
------------------------------------------------------------------------- */

#include "string.h"
#include "stdlib.h"
#include "fix_dna_toggle.h"
#include "simulator.h"
#include "particle.h"
#include "react.h"
#include "chem.h"
#include "chem_gillespie.h"
#include "random.h"
#include "memory.h"
#include "error.h"

#define AVOGADRO 6.023e23

/* ---------------------------------------------------------------------- */

FixDNAToggle::FixDNAToggle(int narg, char **arg) : Fix(narg, arg)
{
  if (narg != 11) error->all("Illegal fix dna/toggle command");
  if (simulator->spatial_flag == 1)
    error->all("Cannot use fix dna/toggle with spatial simulations");

  nevery = atoi(arg[2]);

  int n = strlen(arg[3]) + 1;
  dnaspecies = new char[n];
  strcpy(dnaspecies,arg[3]);

  kon = atof(arg[4]);
  koff = atof(arg[5]);

  n = strlen(arg[6]) + 1;
  eqrna = new char[n];
  strcpy(eqrna,arg[6]);

  ktranscription = atof(arg[7]);
  kconstitutive = atof(arg[8]);

  n = strlen(arg[9]) + 1;
  eqdna = new char[n];
  strcpy(eqdna,arg[9]);

  n = strlen(arg[10]) + 1;
  bindspecies = new char[n];
  strcpy(bindspecies,arg[10]);
}

/* ---------------------------------------------------------------------- */

FixDNAToggle::~FixDNAToggle()
{
  delete [] dnaspecies;
  delete [] eqrna;
  delete [] eqdna;
  delete [] bindspecies;
}

/* ---------------------------------------------------------------------- */

int FixDNAToggle::setmask()
{
  int mask = 0;
  mask |= INITIAL;
  mask |= CLEANUP;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixDNAToggle::init()
{
  // extract two initial reaction rates

  rate = react->rate;
  pcount = particle->pcount;
  ccount = particle->ccount;

  idna = particle->find(dnaspecies);
  if (idna < 0)
    error->all("Invalid species ID in fix dna/toggle command");
  ibind = particle->find(bindspecies);
  if (ibind < 0)
    error->all("Invalid species ID in fix dna/toggle command");

  ieqrna = react->find(eqrna);
  if (ieqrna < 0)
    error->all("Invalid reaction ID in fix dna/toggle command");
  rate_rna = rate[ieqrna];

  ieqdna = react->find(eqdna);
  if (ieqdna < 0)
    error->all("Invalid reaction ID in fix dna/toggle command");
  rate_dna = rate[ieqdna];
}

/* ---------------------------------------------------------------------- */

void FixDNAToggle::initial()
{
  if (simulator->ntimestep % nevery) return;

  // Gillespie stochastic simulation
  // adjust rate, reset propensity, trigger tree recalibration

  if (simulator->stochastic_flag) {

    rate[ieqrna] = ktranscription * koff/kon * pcount[idna] + 
      kconstitutive * (1.0-pcount[idna]);
    rate[ieqdna] = -koff * pcount[idna] + 
      kon * pcount[ibind] * (1.0-pcount[idna]);

    ChemGillespie *gillespie = (ChemGillespie *) chem;

    double propensity = gillespie->compute_propensity(ieqrna);
    gillespie->set(ieqrna,propensity);
    propensity = gillespie->compute_propensity(ieqdna);
    gillespie->set(ieqdna,propensity);

  // continuum ODE simulation, adjust 2 rates

  } else {
    rate[ieqrna] = ktranscription * koff/kon * ccount[idna] + 
      kconstitutive * (1.0-ccount[idna]);
    rate[ieqdna] = -koff * ccount[idna] + 
      kon * ccount[ibind] * (1.0-ccount[idna]);
  }
}

/* ---------------------------------------------------------------------- */

void FixDNAToggle::cleanup()
{
  rate[ieqrna] = rate_rna;
  rate[ieqdna] = rate_dna;
}
