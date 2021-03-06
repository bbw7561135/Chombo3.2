#ifdef CH_LANG_CC
/*
 *      _______              __
 *     / ___/ /  ___  __ _  / /  ___
 *    / /__/ _ \/ _ \/  V \/ _ \/ _ \
 *    \___/_//_/\___/_/_/_/_.__/\___/
 *    Please refer to Copyright.txt, in Chombo's root directory.
 */
#endif

#include "LoHiSide.H"
#include "SolidF_F.H"

#include "AdvectIBC.H"

#include "OldLoHiCenter.H"

#include "UsingNamespace.H"

// Null constructor
AdvectIBC::AdvectIBC()
{
  setDefaultValues();
}

void
AdvectIBC::setDefaultValues()
{
  // default values
  for (int dir=0; dir<SpaceDim; dir++)
    {
      m_bcVal[dir][0] = 0.0;
      m_bcVal[dir][1] = 0.0;
      m_slopeVal[dir][0] = 0.0;
      m_slopeVal[dir][1] = 0.0;
    }
  m_isBCvalSet = false;
  m_isSlopeValSet = false;
}


// Factory method - this object is its own factory:
//   Return a pointer to a new OldPhysIBC object with m_isDefined = false (i.e.,
//   its define() must be called before it is used) and m_isFortranCommonSet
//   set to value of m_isFortranCommonset in the current (factory) object.
OldPhysIBC* AdvectIBC::new_physIBC()
{
  AdvectIBC* retval = new AdvectIBC();
  retval->m_velocity = m_velocity;
  retval->m_probtype = m_probtype;
  retval->m_time = m_time;
  retval->m_isBCvalSet = m_isBCvalSet;
  retval->m_isSlopeValSet = m_isSlopeValSet;
  for (int dir=0; dir<SpaceDim; dir++)
    {
      if (m_isBCvalSet)
        {
          retval->m_bcVal[dir][0] = m_bcVal[dir][0];
          retval->m_bcVal[dir][1] = m_bcVal[dir][1];
        }

      if (m_isSlopeValSet)
        {
          retval->m_slopeVal[dir][0] = m_slopeVal[dir][0];
          retval->m_slopeVal[dir][1] = m_slopeVal[dir][1];
        }
    }
  return static_cast<OldPhysIBC*>(retval);
}

// Set boundary fluxes
void AdvectIBC::fluxBC(FArrayBox&          a_F,
                       const FArrayBox&      a_W,
                       const FArrayBox&      a_Wextrap,
                       const int&            a_dir,
                       const Side::LoHiSide& a_side,
                       const Real&           a_time)
{
  CH_assert(m_isDefined == true);

  // In periodic case, this doesn't do anything
  if (!m_domain.isPeriodic(a_dir))
    {
      // the default for slopes (0) is a pretty reasonable
      // choice, so bag the CH_assertion here.
      //CH_assert(m_isSlopeValSet);

      int sign;
      Box FBox = a_F.box();
      Box tmp = FBox;
      Real bcVal;

      // Determine which side and thus shifting directions
      if (a_side == Side::Lo)
        {
          sign = -1;
          bcVal = m_bcVal[a_dir][0];
        }
      else
        {
          sign = 1;
          bcVal = m_bcVal[a_dir][1];
        }

      tmp.shiftHalf(a_dir,sign);

      // Is there a domain boundary next to this grid
      if (!m_domain.contains(tmp))
        {
          tmp &= m_domain;

          Box boundaryBox;

          // Find the strip of cells next to the domain boundary
          if (a_side == Side::Lo)
            {
              boundaryBox = bdryLo(tmp,a_dir);
            }
          else
            {
              boundaryBox = bdryHi(tmp,a_dir);
            }

          // Shift things to all line up correctly
          boundaryBox.shiftHalf(a_dir,-sign);
          a_F.shiftHalf(a_dir,-sign);
          // do this in a slightly clunky way for the
          // case where the domain is split and there are ghost cells
          // in a_F.
          if (a_F.box().contains(boundaryBox))
            {
              // Set the boundary fluxes
              FORT_SOLIDBCF(CHF_FRA(a_F),
                            CHF_CONST_FRA(a_Wextrap),
                            CHF_CONST_REAL(bcVal),
                            CHF_CONST_INT(sign),
                            CHF_CONST_REAL(m_dx),
                            CHF_CONST_INT(a_dir),
                            CHF_BOX(boundaryBox));
            }

          // Shift returned fluxes to be face centered
          a_F.shiftHalf(a_dir,sign);
        }
    }
}

// Set boundary slopes:
//   The boundary slopes in a_dW are already set to one sided difference
//   approximations.  If this function doesn't change them they will be
//   used for the slopes at the boundaries.
void AdvectIBC::setBdrySlopes(FArrayBox&       a_dW,
                              const FArrayBox& a_W,
                              const int&       a_dir,
                              const Real&      a_time)
{
  //  CH_assert(m_isFortranCommonSet == true);
  CH_assert(m_isDefined == true);


  // In periodic case, this doesn't do anything
  if (!m_domain.isPeriodic(a_dir))
    {
      // don't check for this, since setting slopes to 0
      // (default) is a reasonable choice.
      //CH_assert (m_isSlopeValSet);
      Box loBox,hiBox,centerBox,domain;
      int hasLo,hasHi;
      Box slopeBox = a_dW.box()&m_domain;

      Real loVal = m_slopeVal[a_dir][0];
      Real hiVal = m_slopeVal[a_dir][1];

      // Generate the domain boundary boxes, loBox and hiBox, if there are
      // domain boundarys there
      oldLoHiCenter(loBox,hasLo,hiBox,hasHi,centerBox,domain,
                 slopeBox,m_domain,a_dir);

      // Set the boundary slopes if necessary
      if ((hasLo != 0) || (hasHi != 0))
        {
          FORT_SLOPEBCSF(CHF_FRA(a_dW),
                         CHF_CONST_FRA(a_W),
                         CHF_CONST_REAL(m_dx),
                         CHF_CONST_INT(a_dir),
                         CHF_CONST_REAL(loVal),
                         CHF_BOX(loBox),
                         CHF_CONST_INT(hasLo),
                         CHF_CONST_REAL(hiVal),
                         CHF_BOX(hiBox),
                         CHF_CONST_INT(hasHi));
        }
    }
}

// Set up initial conditions
void AdvectIBC::initialize(LevelData<FArrayBox>& a_U)
{
  /// shouldn't be in this function
  MayDay::Error("Shouldn't be in AdvectIBC::initialize");
}

  /// set velocity
void
AdvectIBC::advectionVel(const RealVect& a_advVel)
{
  m_velocity = a_advVel;
}

///
const RealVect&
AdvectIBC::advectionVel() const
{
  return m_velocity;
}


// set probtype
void
AdvectIBC::probType(const int a_probtype)
{
  m_probtype = a_probtype;
}

//
int
AdvectIBC::probType() const
{
  return m_probtype;
}


void
AdvectIBC::setBoundaryValue(Real a_bcVal, int a_dir,
                            Side::LoHiSide a_hiLo)
{
  if (a_hiLo == Side::Lo)
    {
      m_bcVal[a_dir][0] = a_bcVal;
    }
  else
    {
      m_bcVal[a_dir][1] = a_bcVal;
    }
}

Real
AdvectIBC::getBoundaryValue(int a_dir, Side::LoHiSide a_hiLo) const
{
  Real bcval;
  if (a_hiLo == Side::Lo)
    {
      bcval = m_bcVal[a_dir][0];
    }
  else
    {
      bcval = m_bcVal[a_dir][1];
    }

  return bcval;
}


void
AdvectIBC::setSlopeValue(Real a_slopeVal, int a_dir,
                         Side::LoHiSide a_hiLo)
{
  if (a_hiLo == Side::Lo)
    {
      m_slopeVal[a_dir][0] = a_slopeVal;
    }
  else
    {
      m_slopeVal[a_dir][1] = a_slopeVal;
    }
}

Real
AdvectIBC::getSlopeValue(int a_dir, Side::LoHiSide a_hiLo) const
{
  Real bcval;
  if (a_hiLo == Side::Lo)
    {
      bcval = m_slopeVal[a_dir][0];
    }
  else
    {
      bcval = m_slopeVal[a_dir][1];
    }

  return bcval;
}

