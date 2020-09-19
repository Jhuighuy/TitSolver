/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_SMOOTHING_KERNELS_HPP_
#define TIT_SMOOTHING_KERNELS_HPP_

#include <cmath>
#include <cassert>
#include <type_traits>

#include "TitHelpers.hpp"
#include "TitVector.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Abstract smoothing kernel.
 ********************************************************************/
template<typename Real, int Dim>
class TSmoothingKernel 
{
private:
    static_assert(std::is_floating_point_v<Real>);
    static_assert(1 <= Dim && Dim <= 3);

private:
    /** Unit support radius. */
    virtual Real Radius_() const noexcept = 0;
    /** Value of the base smoothing kernel at a point. */ 
    virtual Real Value_(Real q) const noexcept = 0;
    /** Derivative value of the base smoothing kernel at a point. */ 
    virtual Real ValueDerivative_(Real q) const noexcept = 0;

public:
    /** Value of the smoothing kernel at point. */ 
    Real Value(Vector<Real,Dim> r, Real h) const noexcept;
    /** Spatial gradient value of the smoothing kernel at point. */ 
    Vector<Real,Dim> GradientValue(Vector<Real,Dim> r, Real h) const noexcept;

    /** Support radius. */
    Real Radius(Real h) const noexcept;
    /** Width derivative value of the smoothing kernel at point. */ 
    Real RadiusDerivative(Vector<Real,Dim> r, Real h) const noexcept;
};  // class TSmoothingKernel

template<typename Real, int Dim>
Real TSmoothingKernel<Real,Dim>::Value(Vector<Real,Dim> r, Real h) const noexcept 
{
    assert(h > 0.0);
    Real q = Norm(r)/h;
    return Value_(q)*std::pow(h, -Dim);
}

template<typename Real, int Dim>
Vector<Real,Dim> TSmoothingKernel<Real,Dim>::GradientValue(Vector<Real,Dim> r, Real h) const noexcept 
{
    assert(h > 0.0);
    Real q = Norm(r)/h;
    return Normalize(r)*ValueDerivative_(q)/std::pow(h, Dim+1);
}

template<typename Real, int Dim>
Real TSmoothingKernel<Real,Dim>::Radius(Real h) const noexcept 
{
    assert(h > 0.0);
    return Radius_()*h;
}

template<typename Real, int Dim>
Real TSmoothingKernel<Real,Dim>::RadiusDerivative(Vector<Real,Dim> r, Real h) const noexcept 
{
    assert(h > 0.0);
    Real q = Norm(r)/h;
    return -(Dim*Value_(q) + q*ValueDerivative_(q))/std::pow(h, Dim+1);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** The gaussian smoothing kernel.
 ********************************************************************/
template<typename Real, int Dim>
class TGaussianSmoothingKernel final : public TSmoothingKernel<Real,Dim> 
{
private:
    static Real Weight_() noexcept;
    Real Radius_() const noexcept override;
    Real Value_(Real q) const noexcept override;
    Real ValueDerivative_(Real q) const noexcept override;
};  // class TSmoothingKernel_Gaussian

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::Radius_() const noexcept 
{
    return Real(1e+10);
}

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::Weight_() noexcept 
{
    static const Real weight = std::pow(Pi<Real>, -Real(0.5*Dim));
    return weight;
}

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::Value_(Real q) const noexcept 
{
    return Weight_()*std::exp(-Pow2(q));
}

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::ValueDerivative_(Real q) const noexcept 
{
    return Weight_()*(-Real(2.0)*q*Exp(-Pow2(q)));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** The cubic B-spline (M4) smoothing kernel.
 ********************************************************************/
template<typename Real, int Dim>
class TCubicSmoothingKernel : public TSmoothingKernel<Real,Dim> 
{
protected:
    static Real Weight_() noexcept;
private:
    Real Radius_() const noexcept final;
    Real Value_(Real q) const noexcept final;
    Real ValueDerivative_(Real q) const noexcept override;
};  // class TCubicSmoothingKernel

/********************************************************************
 ** The cubic B-spline (M4) smoothing kernel
 ** with Thomas-Couchman (1992) modified derivative.
 ********************************************************************/
template<typename Real, int Dim>
class TModifiedCubicSmoothingKernel final : public TCubicSmoothingKernel<Real,Dim> 
{
private:
    using TCubicSmoothingKernel<Real,Dim>::Weight_;
    Real ValueDerivative_(Real q) const noexcept override;
};  // class TModifiedCubicSmoothingKernel

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::Weight_() noexcept 
{
    if constexpr (Dim == 1) 
    {
        return Real(2.0/3.0);
    } 
    else if constexpr (Dim == 2) 
    {
        return Real(10.0/7.0)*Pi<Real>;
    } 
    else if constexpr (Dim == 3) 
    {
        return Real(1.0)/Pi<Real>;
    }
}

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::Radius_() const noexcept 
{
    return Real(2.0);
}

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::Value_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return Weight_()*(Real(0.25)*Pow3(Real(2.0)-q) - Pow3(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0)) 
    {
        return Weight_()*(Real(0.25)*Pow3(Real(2.0)-q));
    } 
    return Real(0.0);
}

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::ValueDerivative_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return Weight_()*(-Real(0.75)*Pow2(Real(2.0)-q) + Real(3.0)*Pow2(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0)) 
    {
        return Weight_()*(-Real(0.75)*Pow2(Real(2.0)-q));
    }
    return Real(0.0);
}

template<typename Real, int Dim>
Real TModifiedCubicSmoothingKernel<Real,Dim>::ValueDerivative_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(2.0/3.0)) 
    {
        return Weight_()*(-Real(1.0));
    } 
    else if (Real(2.0/3.0) <= q && q < Real(1.0)) 
    {
        return Weight_()*(Real(2.25)*q - Real(3.0))*q;
    }
    else if (Real(1.0) <= q && q < Real(2.0))
    {
        return Weight_()*(Real(0.75)*Pow2(Real(2.0)-q));
    } 
    return Real(0.0);
}

/********************************************************************
 ** The quartic B-spline (M5) smoothing kernel.
 ********************************************************************/
template<typename Real, int Dim>
class TQuarticSmoothingKernel final : public TSmoothingKernel<Real,Dim> 
{
private:
    static Real Weight_() noexcept;
    Real Radius_() const noexcept override;
    Real Value_(Real q) const noexcept override;
    Real ValueDerivative_(Real q) const noexcept override;
};  // class TQuarticSmoothingKernel

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::Weight_() noexcept 
{
    if constexpr (Dim == 1) 
    {
        return Real(1.0/24.0);
    } 
    else if constexpr (Dim == 2) 
    {
        return Real(96.0/1199.0)/Pi<Real>;
    } 
    else if constexpr (Dim == 3) 
    {
        return Real(1.0/2.0)/Pi<Real>;
    }
}

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::Radius_() const noexcept 
{
    return Real(2.5);
}

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::Value_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(0.5)) 
    {
        return Weight_()*(Pow4(Real(2.5)-q) - Real(5.0)*Pow4(Real(1.5)-q) + Real(10.0)*Pow4(Real(0.5)-q));
    } 
    else if (Real(0.5) <= q && q < Real(1.5)) 
    {
        return Weight_()*(Pow4(Real(2.5)-q) - Real(5.0)*Pow4(Real(1.5)-q));
    } 
    else if (Real(1.5) <= q && q < Real(2.5)) 
    {
        return Weight_()*(Pow4(Real(2.5)-q));
    } 
    return Real(0.0);
}

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::ValueDerivative_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(0.5)) 
    {
        return Weight_()*(-Real(4.0)*Pow3(Real(2.5)-q) + Real(20.0)*Pow3(Real(1.5)-q) - Real(40.0)*Pow3(Real(0.5)-q));
    } 
    else if (Real(0.5) <= q && q < Real(1.5)) 
    {
        return Weight_()*(-Real(4.0)*Pow3(Real(2.5)-q) + Real(20.0)*Pow3(Real(1.5)-q));
    } 
    else if (Real(1.5) <= q && q < Real(2.5)) 
    {
        return Weight_()*(-Real(4.0)*Pow3(Real(2.5)-q));
    } 
    return Real(0.0);
}

/********************************************************************
 ** The quintic B-spline (M6) smoothing kernel.
 ********************************************************************/
template<typename Real, int Dim>
class TQuinticSmoothingKernel final : public TSmoothingKernel<Real,Dim> 
{
private:
    static Real Weight_() noexcept;
    Real Radius_() const noexcept override;
    Real Value_(Real q) const noexcept override;
    Real ValueDerivative_(Real q) const noexcept override;
};  // class TQuinticSmoothingKernel

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::Weight_() noexcept 
{
    if constexpr (Dim == 1) 
    {
        return Real(1.0/120.0);
    } 
    else if constexpr (Dim == 2) 
    {
        return Real(7.0/478.0)/Pi<Real>;
    } 
    else if constexpr (Dim == 3) 
    {
        return Real(1.0/120.0)/Pi<Real>;
    }
}

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::Radius_() const noexcept 
{
    return Real(3.0);
}

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::Value_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return Weight_()*(Pow5(Real(3.0)-q) - Real(6.0)*Pow5(Real(2.0)-q) + Real(15.0)*Pow5(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0))
    {
        return Weight_()*(Pow5(Real(3.0)-q) - Real(6.0)*Pow5(Real(2.0)-q));
    } 
    else if (Real(2.0) <= q && q < Real(3.0)) 
    {
        return Weight_()*(Pow5(Real(3.0)-q));
    } 
    return Real(0.0);
}

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::ValueDerivative_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return Weight_()*(-Real(5.0)*Pow4(Real(3.0)-q) + Real(30.0)*Pow4(Real(2.0)-q) - Real(75.0)*Pow4(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0)) 
    {
        return Weight_()*(-Real(5.0)*Pow4(Real(3.0)-q) + Real(30.0)*Pow4(Real(2.0)-q));
    } 
    else if (Real(2.0) <= q && q < Real(3.0)) 
    {
        return Weight_()*(-Real(5.0)*Pow4(Real(3.0)-q));
    } 
    return Real(0.0);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_SMOOTHING_KERNELS_HPP_
