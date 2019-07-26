#include "interp_inspection.hh"
#include "interp_internal.hh"


double &currentX(setup_pointer s)
{
  return s->current[X];
}

double &currentY(setup_pointer s)
{
  return s->current[Y];
}

double &currentZ(setup_pointer s)
{
  return s->current[Z];
}

double & currentA(setup_pointer s)
{
  return s->current[A];
}

double & currentB(setup_pointer s)
{
  return s->current[B];
}

double & currentC(setup_pointer s)
{
  return s->current[C];
}

double & currentWorkOffsetX(setup_pointer s)
{
  return s->transform.g5xOffset.shift.tf[X];
}

double & currentWorkOffsetY(setup_pointer s)
{
  return s->transform.g5xOffset.shift.tf[Y];
}

double &currentWorkOffsetZ(setup_pointer s)
{
  return s->transform.g5xOffset.shift.tf[Z];
}

double & currentWorkOffsetA(setup_pointer s)
{
  return s->transform.g5xOffset.shift.tf[A];
}

double & currentWorkOffsetB(setup_pointer s)
{
  return s->transform.g5xOffset.shift.tf[B];
}

double & currentWorkOffsetC(setup_pointer s)
{
  return s->transform.g5xOffset.shift.tf[C];
}

double & currentAxisOffsetX(setup_pointer s)
{
  return s->transform.g92Offset.tf[X];
}

double & currentAxisOffsetY(setup_pointer s)
{
  return s->transform.g92Offset.tf[Y];
}

double &currentAxisOffsetZ(setup_pointer s)
{
  return s->transform.g92Offset.tf[Z];
}

double & currentAxisOffsetA(setup_pointer s)
{
  return s->transform.g92Offset.tf[A];
}

double & currentAxisOffsetB(setup_pointer s)
{
  return s->transform.g92Offset.tf[B];
}

double & currentAxisOffsetC(setup_pointer s)
{
  return s->transform.g92Offset.tf[C];
}

