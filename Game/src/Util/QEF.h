#pragma once

class QEFSolver
{
public:
  QEFSolver();

  const Vec3& getMassPoint() const { return massPoint; }

  void add(const Vec3& p, const Vec3& n);

  float solve(Vec3& outx, const float svd_tol, const int svd_sweeps, const float pinv_tol);

private:
  struct QEFData
  {
    float ata_00, ata_01, ata_02, ata_11, ata_12, ata_22;
    Vec3 atb;
    float btb;
    Vec3 massPoint;
    int numPoints;
  };

  QEFData data;
  Mat3 ata;
  Vec3 atb, massPoint, x;
  bool hasSolution;

  void setAta();
  void setAtb();
};