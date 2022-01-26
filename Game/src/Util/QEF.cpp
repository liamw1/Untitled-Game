#include "GMpch.h"
#include "QEF.h"

static void setSymmetric(Mat3& A, float a00, float a01, float a02, float a11, float a12, float a22)
{
  A[0][0] = a00; A[0][1] = a01; A[0][2] = a02;
  A[1][0] = a01; A[1][1] = a11; A[1][2] = a12;
  A[2][0] = a02; A[2][1] = a12; A[2][2] = a22;
}

static void set(Mat3& A, float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22)
{
  A[0][0] = a00; A[0][1] = a01; A[0][2] = a02;
  A[1][0] = a10; A[1][1] = a11; A[1][2] = a12;
  A[2][0] = a20; A[2][1] = a21; A[2][2] = a22;
}

static float off(const Mat3& A)
{
  return sqrt((A[0][1] * A[0][1]) + (A[0][2] * A[0][2])
            + (A[1][0] * A[1][0]) + (A[1][2] * A[1][2])
            + (A[2][0] * A[2][0]) + (A[2][1] * A[2][1]));
}

static float fnorm(const Mat3& A)
{
  return sqrt((A[0][0] * A[0][0]) + (A[0][1] * A[0][1]) + (A[0][2] * A[0][2])
            + (A[1][0] * A[1][0]) + (A[1][1] * A[1][1]) + (A[1][2] * A[1][2])
            + (A[2][0] * A[2][0]) + (A[2][1] * A[2][1]) + (A[2][2] * A[2][2]));
}

static Vec3 vmul_symmetric(const Mat3& a, const Vec3& v)
{
  return { a[0][0] * v.x + a[0][1] * v.y + a[0][2] * v.z,
           a[1][0] * v.x + a[1][1] * v.y + a[1][2] * v.z,
           a[2][0] * v.x + a[2][1] * v.y + a[2][2] * v.z };
}

static void calcSymmetricGivensCoefficients(float a_pp, float a_pq, float a_qq, float& c, float& s)
{
  if (a_pq == 0)
  {
    c = 1;
    s = 0;
    return;
  }

  const float tau = (a_qq - a_pp) / (2 * a_pq);
  const float stt = sqrt(1 + tau * tau);
  const float tan = 1.0f / ((tau >= 0) ? (tau + stt) : (tau - stt));
  c = 1.0f / sqrt(1.0f + tan * tan);
  s = tan * c;
}

static void rot01(Mat3& M, float& c, float& s)
{
  calcSymmetricGivensCoefficients(M[0][0], M[0][1], M[1][1], c, s);
  const float cc = c * c;
  const float ss = s * s;
  const float mix = 2 * c * s * M[0][1];
  setSymmetric(M, cc * M[0][0] - mix + ss * M[1][1], 0, c * M[0][2] - s * M[1][2],
    ss * M[0][0] + mix + cc * M[1][1], s * M[0][2] + c * M[1][2], M[2][2]);
}

static void rot02(Mat3& M, float& c, float& s)
{
  calcSymmetricGivensCoefficients(M[0][0], M[0][2], M[2][2], c, s);
  const float cc = c * c;
  const float ss = s * s;
  const float mix = 2 * c * s * M[0][2];
  setSymmetric(M, cc * M[0][0] - mix + ss * M[2][2], c * M[0][1] - s * M[1][2], 0,
    M[1][1], s * M[0][1] + c * M[1][2], ss * M[0][0] + mix + cc * M[2][2]);
}

static void rot12(Mat3& M, float& c, float& s)
{
  calcSymmetricGivensCoefficients(M[1][1], M[1][2], M[2][2], c, s);
  const float cc = c * c;
  const float ss = s * s;
  const float mix = 2 * c * s * M[1][2];
  setSymmetric(M, M[0][0], c * M[0][1] - s * M[0][2], s * M[0][1] + c * M[0][2],
    cc * M[1][1] - mix + ss * M[2][2], 0, ss * M[1][1] + mix + cc * M[2][2]);
}

static void rot01_post(Mat3& M, float c, float s)
{
  const float m00 = M[0][0], m01 = M[0][1], m10 = M[1][0], m11 = M[1][1], m20 = M[2][0], m21 = M[2][1];
  set(M, c * m00 - s * m01, s * m00 + c * m01, M[0][2], c * m10 - s * m11,
    s * m10 + c * m11, M[1][2], c * m20 - s * m21, s * m20 + c * m21, M[2][2]);
}

static void rot02_post(Mat3& M, float c, float s)
{
  const float m00 = M[0][0], m02 = M[0][2], m10 = M[1][0], m12 = M[1][2], m20 = M[2][0], m22 = M[2][2];
  set(M, c * m00 - s * m02, M[0][1], s * m00 + c * m02, c * m10 - s * m12, M[1][1],
    s * m10 + c * m12, c * m20 - s * m22, M[2][1], s * m20 + c * m22);
}

static void rot12_post(Mat3& M, float c, float s)
{
  const float m01 = M[0][1], m02 = M[0][2], m11 = M[1][1], m12 = M[1][2], m21 = M[2][1], m22 = M[2][2];
  set(M, M[0][0], c * m01 - s * m02, s * m01 + c * m02, M[1][0], c * m11 - s * m12,
    s * m11 + c * m12, M[2][0], c * m21 - s * m22, s * m21 + c * m22);
}

static void rotate01(Mat3& VTAV, Mat3& V)
{
  if (VTAV[0][1] == 0)
    return;

  float c, s;
  rot01(VTAV, c, s);
  rot01_post(V, c, s);
}

static void rotate02(Mat3& VTAV, Mat3& V)
{
  if (VTAV[0][2] == 0)
    return;

  float c, s;
  rot02(VTAV, c, s);
  rot02_post(V, c, s);
}

static void rotate12(Mat3& VTAV, Mat3& V)
{
  if (VTAV[1][2] == 0)
    return;

  float c, s;
  rot12(VTAV, c, s);
  rot12_post(V, c, s);
}

static void getSymmetricSvd(const Mat3& A, Mat3& VTAV, Mat3& V, float tol, int max_sweeps)
{
  setSymmetric(VTAV, A[0][0], A[0][1], A[0][2], A[1][1], A[1][2], A[2][2]);
  V = glm::identity<Mat3>();

  const float delta = tol * fnorm(VTAV);
  for (int i = 0; i < max_sweeps && off(VTAV) > delta; ++i)
  {
    rotate01(VTAV, V);
    rotate02(VTAV, V);
    rotate12(VTAV, V);
  }
}

static float pinv(float x, float tol)
{
  return (fabs(x) < tol || fabs(1 / x) < tol) ? 0 : (1 / x);
}

static void pseudoInverse(Mat3& out, const Mat3& D, const Mat3& V, float tol)
{
  const float d0 = pinv(D[0][0], tol), d1 = pinv(D[1][1], tol), d2 = pinv(D[2][2], tol);

  set(out, V[0][0] * d0 * V[0][0] + V[0][1] * d1 * V[0][1] + V[0][2] * d2 * V[0][2],
           V[0][0] * d0 * V[1][0] + V[0][1] * d1 * V[1][1] + V[0][2] * d2 * V[1][2],
           V[0][0] * d0 * V[2][0] + V[0][1] * d1 * V[2][1] + V[0][2] * d2 * V[2][2],
           V[1][0] * d0 * V[0][0] + V[1][1] * d1 * V[0][1] + V[1][2] * d2 * V[0][2],
           V[1][0] * d0 * V[1][0] + V[1][1] * d1 * V[1][1] + V[1][2] * d2 * V[1][2],
           V[1][0] * d0 * V[2][0] + V[1][1] * d1 * V[2][1] + V[1][2] * d2 * V[2][2],
           V[2][0] * d0 * V[0][0] + V[2][1] * d1 * V[0][1] + V[2][2] * d2 * V[0][2],
           V[2][0] * d0 * V[1][0] + V[2][1] * d1 * V[1][1] + V[2][2] * d2 * V[1][2],
           V[2][0] * d0 * V[2][0] + V[2][1] * d1 * V[2][1] + V[2][2] * d2 * V[2][2]);
}

static float calcError(const Mat3& origA, const Vec3& x, const Vec3& b)
{
  Mat3 A{};
  setSymmetric(A, origA[0][0], origA[0][1], origA[0][2], origA[1][1], origA[1][2], origA[2][2]);
  Vec3 vtmp = A * x;
  vtmp = b - vtmp;
  return glm::dot(vtmp, vtmp);
}

static float solveSymmetric(const Mat3& A, const Vec3& b, Vec3& x, float svd_tol, int svd_sweeps, float pinv_tol)
{
  Mat3 mtmp, pinv, V{};
  Mat3 VTAV{};
  getSymmetricSvd(A, VTAV, V, svd_tol, svd_sweeps);
  pseudoInverse(pinv, VTAV, V, pinv_tol);
  x = pinv * b;
  return calcError(A, x, b);
}

QEFSolver::QEFSolver()
  : data(), ata(), atb(), massPoint(), x(), hasSolution(false) {}

void QEFSolver::add(const Vec3& p, const Vec3& n)
{
  hasSolution = false;
  data.ata_00 += n.x * n.x;
  data.ata_01 += n.x * n.y;
  data.ata_02 += n.x * n.z;
  data.ata_11 += n.y * n.y;
  data.ata_12 += n.y * n.z;
  data.ata_22 += n.z * n.z;

  const float dot = glm::dot(n, p);
  data.atb += dot * n;
  data.btb += dot * dot;
  data.massPoint += p;

  data.numPoints++;
}

float QEFSolver::solve(Vec3& outx, const float svd_tol, const int svd_sweeps, const float pinv_tol)
{
  EN_ASSERT(data.numPoints > 0, "Must have at least 1 data point!");

  massPoint = data.massPoint;
  massPoint /= data.numPoints;
  setAta();
  setAtb();
  atb -= vmul_symmetric(ata, massPoint);
  x = Vec3(0.0);
  const float result = solveSymmetric(ata, atb, x, svd_tol, svd_sweeps, pinv_tol);
  x += massPoint;
  setAtb();
  outx = x;
  hasSolution = true;
  return result;
}

void QEFSolver::setAta()
{
  setSymmetric(ata, data.ata_00, data.ata_01, data.ata_02, data.ata_11, data.ata_12, data.ata_22);
}

void QEFSolver::setAtb()
{
  atb = data.atb;
}
