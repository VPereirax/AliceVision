// This file is part of the AliceVision project.
// Copyright (c) 2016 AliceVision contributors.
// Copyright (c) 2012 openMVG contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/numeric/numeric.hpp>
#include <aliceVision/camera/cameraCommon.hpp>
#include <aliceVision/camera/IntrinsicInitMode.hpp>
#include <aliceVision/geometry/Pose3.hpp>
#include <aliceVision/stl/hash.hpp>

#include <vector>

namespace aliceVision {
namespace camera {

/**
 * @brief Basis class for all intrinsic parameters of a camera
 *  Store the image size & define all basis optical modelization of a camera
 */
class IntrinsicBase
{
public:

  IntrinsicBase(unsigned int width = 0, unsigned int height = 0, const std::string& serialNumber = "")
    : _w(width)
    , _h(height)
    , _serialNumber(serialNumber)
  {}

  virtual ~IntrinsicBase()
  {}
  
  /**
   * @brief Get the lock state of the intrinsic
   * @return true if the intrinsic is locked
   */
  inline bool isLocked() const
  {
    return _locked;
  }
  
  /**
   * @brief Get the intrinsic image width
   * @return The intrinsic image width
   */
  inline unsigned int w() const
  {
    return _w;
  }

  /**
   * @brief Get the intrinsic image height
   * @return The intrinsic image height
   */
  inline unsigned int h() const
  {
    return _h;
  }

  /**
   * @brief Get the intrinsic serial number
   * @return The intrinsic serial number
   */
  inline const std::string& serialNumber() const
  {
    return _serialNumber;
  }

  /**
   * @brief Get the intrinsic initialization mode
   * @return The intrinsic initialization mode
   */
  inline EIntrinsicInitMode getInitializationMode() const
  {
    return _initializationMode;
  }

  /**
   * @brief operator ==
   * @param[in] other
   * @return True if equals
   */
  inline bool operator==(const IntrinsicBase& other) const
  {
    return _w == other._w &&
           _h == other._h &&
           _serialNumber == other._serialNumber &&
           _initializationMode == other._initializationMode &&
           getType() == other.getType() &&
           getParams() == other.getParams();
  }

  /**
   * @brief Projection of a 3D point into the camera plane (Apply pose, disto (if any) and Intrinsics)
   * @param[in] pose The pose
   * @param[in] pt3D The 3d ppont
   * @param[in] applyDistortion If true apply distrortion if any
   * @return The 2d projection in the camera plane
   */
  virtual Vec2 project(const geometry::Pose3& pose, const Vec3& pt3D, bool applyDistortion = true) const = 0;

  /**
   * @brief Compute the residual between the 3D projected point X and an image observation x
   * @param[in] pose The pose
   * @param[in] X The 3D projected point
   * @param[in] x The image observation
   * @return residual
   */
  inline Vec2 residual(const geometry::Pose3& pose, const Vec3& X, const Vec2& x) const
  {
    const Vec2 proj = this->project(pose, X);
    return x - proj;
  }

  /**
   * @brief Compute the residual between the 3D projected point X and an image observation x
   * @param[in] pose The pose
   * @param[in] X The 3D projection
   * @param[in] x The image observation
   * @return residual
   */
  inline Mat2X residuals(const geometry::Pose3& pose, const Mat3X& X, const Mat2X& x) const
  {
    assert(X.cols() == x.cols());
    const std::size_t numPts = x.cols();
    Mat2X residuals = Mat2X::Zero(2, numPts);
    for(std::size_t i = 0; i < numPts; ++i)
    {
      residuals.col(i) = residual(pose, X.col(i), x.col(i));
    }
    return residuals;
  }

  /**
   * @brief lock the intrinsic
   */
  inline void lock()
  {
    _locked  = true;
  }

  /**
   * @brief unlock the intrinsic
   */
  inline void unlock()
  {
    _locked  = false;
  }

  /**
   * @brief Set intrinsic image width
   * @param[in] width The image width
   */
  inline void setWidth(unsigned int width)
  {
    _w = width;
  }

  /**
   * @brief Set intrinsic image height
   * @param[in] height The image height
   */
  inline void setHeight(unsigned int height)
  {
    _h = height;
  }
  
  /**
   * @brief Set the serial number
   * @param[in] serialNumber The serial number
   */
  inline void setSerialNumber(const std::string& serialNumber)
  {
    _serialNumber = serialNumber;
  }

  /**
   * @brief Set The intrinsic initialization mode
   * @param[in] initializationMode THe intrintrinsic initialization mode enum
   */
  inline void setInitializationMode(EIntrinsicInitMode initializationMode)
  {
    _initializationMode = initializationMode;
  }

  // Virtual members

  /**
   * @brief Polymorphic clone
   */
  virtual IntrinsicBase* clone() const = 0;

  /**
   * @brief Assign object
   * @param[in] other
   */
  virtual void assign(const IntrinsicBase& other) = 0;

  /**
   * @brief Get embed camera type
   * @return EINTRINSIC enum
   */
  virtual EINTRINSIC getType() const = 0;

  /**
   * get a string
   * @return the string describing the intrinsic type
   */
  std::string getTypeStr() const { 
    return EINTRINSIC_enumToString(getType()); 
  }

  /**
   * @brief Get intrinsic parameters
   * @return intrinsic parameters
   */
  virtual std::vector<double> getParams() const = 0;

  /**
   * @brief Update intrinsic parameters
   * @param[in] intrinsic parameters
   * @return true if done
   */
  virtual bool updateFromParams(const std::vector<double>& params) = 0;

  /**
   * @brief Get bearing vector of p point (image coordinate)
   * @param[in] p Vec2
   * @return Vec3 bearing vector
   */
  virtual Vec3 operator () (const Vec2& p) const
  {
    return toUnitSphere(p);
  }

  /**
   * @brief Transform a point from the camera plane to the image plane
   * @param[in] p A point from the camera plane
   * @return Image plane point
   */
  virtual Vec2 cam2ima(const Vec2& p) const = 0;

  /**
   * @brief Transform a point from the image plane to the camera plane
   * @param[in] p A point from the image plane
   * @return Camera plane point
   */
  virtual Vec2 ima2cam(const Vec2& p) const = 0;

  /**
   * @brief Camera model handle a distortion field
   * @return True if the camera model handle a distortion field
   */
  virtual bool have_disto() const
  {
    return false;
  }

  /**
   * @brief Add the distortion field to a point (that is in normalized camera frame)
   * @param[in] p The point
   * @return The point with added distortion field
   */
  virtual Vec2 add_disto(const Vec2& p) const = 0;

  /**
   * @brief Remove the distortion to a camera point (that is in normalized camera frame)
   * @param[in] p The point
   * @return The point with removed distortion field
   */
  virtual Vec2 remove_disto(const Vec2& p) const = 0;

  /**
   * @brief Return the undistorted pixel (with removed distortion)
   * @param[in] p The point
   * @return The undistorted pixel
   */
  virtual Vec2 get_ud_pixel(const Vec2& p) const = 0;

  /**
   * @brief Return the distorted pixel (with added distortion)
   * @param[in] p The undistorted point
   * @return The distorted pixel
   */
  virtual Vec2 get_d_pixel(const Vec2& p) const = 0;

  /**
   * @brief Normalize a given unit pixel error to the camera plane
   * @param[in] value Given unit pixel error
   * @return Normalized unit pixel error to the camera plane
   */
  virtual double imagePlane_toCameraPlaneError(double value) const = 0;


  /**
   * @brief Return true if the intrinsic is valid
   * @return True if the intrinsic is valid
   */
  virtual bool isValid() const
  {
    return _w != 0 && _h != 0;
  }

  virtual bool hasDistortion() const
  {
    return false;
  }

  /**
   * @brief Return true if this ray should be visible in the image
   * @param ray input ray to check for visibility
   * @return true if this ray is visible theorically
   */
  virtual bool isVisibleRay(const Vec3 & ray) const = 0;

  /**
   * @brief Return true if these pixel coordinates should be visible in the image
   * @param pix input pixel coordinates to check for visibility
   * @return true if visible
   */
  virtual bool isVisible(const Vec2 & pix) const {

    if (pix(0) < 0 || pix(0) >= _w || pix(1) < 0 || pix(1) >= _h) {
      return false;
    }

    return true;
  }

  /**
   * @brief Assuming the distortion is a function of radius, estimate the 
   * maximal undistorted radius for a range of distorted radius.
   * @param min_radius the minimal radius to consider
   * @param max_radius the maximal radius to consider
   * @return the maximal undistorted radius
   */
  virtual float getMaximalDistortion(double min_radius, double max_radius) const {

    /*Without distortion, obvious*/
    return max_radius;
  }

  /**
   * @brief Generate an unique Hash from the camera parameters (used for grouping)
   * @return Unique Hash from the camera parameters
   */
  virtual std::size_t hashValue() const
  {
    size_t seed = 0;
    stl::hash_combine(seed, static_cast<int>(this->getType()));
    stl::hash_combine(seed, _w);
    stl::hash_combine(seed, _h);
    stl::hash_combine(seed, _serialNumber);
    const std::vector<double> params = this->getParams();
    for (size_t i=0; i < params.size(); ++i)
      stl::hash_combine(seed, params[i]);
    return seed;
  }

  /**
   * @brief Rescale intrinsics to reflect a rescale of the camera image
   * @param factor a scale factor
   */
  virtual void rescale(float factor) {

    _w = (unsigned int)(floor(float(_w) * factor));
    _h = (unsigned int)(floor(float(_h) * factor));
  }

  /**
   * @brief transform a given point (in pixels) to unit sphere in meters
   * @param pt the input point
   * @return a point on the unit sphere
   */
  virtual Vec3 toUnitSphere(const Vec2 & pt) const = 0;

protected:

  /// initialization mode
  EIntrinsicInitMode _initializationMode = EIntrinsicInitMode::NONE;
  /// intrinsic lock
  bool _locked = false;
  unsigned int _w = 0;
  unsigned int _h = 0;
  std::string _serialNumber;
};

/**
 * @brief Return the angle (degree) between two bearing vector rays
 * @param[in] ray1 First bearing vector ray
 * @param[in] ray2 Second bearing vector ray
 * @return The angle (degree) between two bearing vector rays
 */
inline double AngleBetweenRays(const Vec3& ray1, const Vec3& ray2)
{
  const double mag = ray1.norm() * ray2.norm();
  const double dotAngle = ray1.dot(ray2);
  return radianToDegree(acos(clamp(dotAngle/mag, -1.0 + 1.e-8, 1.0 - 1.e-8)));
}

/**
 * @brief Return the angle (degree) between two bearing vector rays
 * @param[in] pose1 First pose
 * @param[in] intrinsic1 First intrinsic
 * @param[in] pose2 Second pose
 * @param[in] intrinsic2 Second intrinsic
 * @param[in] x1 First image point
 * @param[in] x2 Second image point
 * @return The angle (degree) between two bearing vector rays
 */
inline double AngleBetweenRays(const geometry::Pose3& pose1,
                               const IntrinsicBase* intrinsic1,
                               const geometry::Pose3& pose2,
                               const IntrinsicBase* intrinsic2,
                               const Vec2& x1,
                               const Vec2& x2)
{
  // x = (u, v, 1.0)  // image coordinates
  // X = R.t() * K.inv() * x + C // Camera world point
  // getting the ray:
  // ray = X - C = R.t() * K.inv() * x
  const Vec3 ray1 = (pose1.rotation().transpose() * intrinsic1->operator()(x1)).normalized();
  const Vec3 ray2 = (pose2.rotation().transpose() * intrinsic2->operator()(x2)).normalized();
  return AngleBetweenRays(ray1, ray2);
}

/**
 * @brief Return the angle (degree) between two poses and a 3D point.
 * @param[in] pose1 First pose
 * @param[in] pose2 Second Pose
 * @param[in] pt3D The 3d point
 * @return The angle (degree) between two poses and a 3D point.
 */
inline double AngleBetweenRays(const geometry::Pose3& pose1,
                               const geometry::Pose3& pose2,
                               const Vec3& pt3D)
{
  const Vec3 ray1 = pt3D - pose1.center();
  const Vec3 ray2 = pt3D - pose2.center();
  return AngleBetweenRays(ray1, ray2);
}

} // namespace camera

template <size_t M, size_t N>
Eigen::Matrix<double, M*N, M*N> getJacobian_At_wrt_A() {
	Eigen::Matrix<double, M*N, M*N> ret;

	/** vec(M1*M2*M3) = kron(M3.t, M1) * vec(M2) */
	/** vec(IAtB) = kron(B.t, I) * vec(A) */
	/** dvec(IAtB)/dA = kron(B.t, I) * dvec(At)/dA */

	ret.fill(0);

	size_t pos_at = 0;
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < N; j++) {
			size_t pos_a = N * j + i;
			ret(pos_at, pos_a) = 1;

			pos_at++;
		}
	}

	return ret;
}

template <size_t M, size_t N, size_t K>
Eigen::Matrix<double, M*K, M*N> getJacobian_AB_wrt_A(const Eigen::Matrix<double, M , N> & A, const Eigen::Matrix<double, N, K> & B)  {
	Eigen::Matrix<double, M*K, M*N> ret;

	/** vec(M1*M2*M3) = kron(M3.t, M1) * vec(M2) */
	/** vec(IAB) = kron(B.t, I) * vec(A) */
	/** dvec(IAB)/dA = kron(B.t, I) * dvec(A)/dA */
	/** dvec(IAB)/dA = kron(B.t, I) */

	ret.fill(0);

	Eigen::Matrix<double, K, N> Bt = B.transpose();

	for (size_t row = 0; row < K; row++) {
		for (size_t col = 0; col < N; col++) {

			ret.template block<M, M>(row * M, col * M) = Bt(row, col) * Eigen::Matrix<double, M, M>::Identity();
		}
	}


	return ret;
}

template <size_t M, size_t N, size_t K>
Eigen::Matrix<double, M*K, M*N> getJacobian_AtB_wrt_A(const Eigen::Matrix<double, M, N> & A, const Eigen::Matrix<double, M, K> & B) {
	return getJacobian_AB_wrt_A<M, N, K>(A.transpose(), B) * getJacobian_At_wrt_A<M, N>();
}

template <size_t M, size_t N, size_t K>
Eigen::Matrix<double, M*K, N*K> getJacobian_AB_wrt_B(const Eigen::Matrix<double, M, N> & A, const Eigen::Matrix<double, N, K> & B) {
	Eigen::Matrix<double, M*K, N*K> ret;

	/** vec(M1*M2*M3) = kron(M3.t, M1) * vec(M2) */
	/** vec(ABI) = kron(I, A) * vec(B) */
	/** dvec(ABI)/dB = kron(I, A) * dvec(B)/dB */
	/** dvec(ABI)/dB = kron(I, A) */

	ret.fill(0);

	for (size_t index = 0; index < K; index++) {

		ret.template block<M, N>(M * index, N * index) = A;
	}

	return ret;
}

} // namespace aliceVision
