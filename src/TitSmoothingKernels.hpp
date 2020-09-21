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
public:
    static_assert(std::is_floating_point_v<Real>);
    static_assert(1 <= Dim && Dim <= 3);

public:
    /** Value of the smoothing kernel at point. */ 
    Real GetValue(Vector<Real,Dim> r, Real h) const noexcept;
    /** Spatial gradient value of the smoothing kernel at point. */ 
    Vector<Real,Dim> GetGradientValue(Vector<Real,Dim> r, Real h) const noexcept;

    /** Support radius. */
    Real GetRadius(Real h) const noexcept;
    /** Width derivative value of the smoothing kernel at point. */ 
    Real GetRadiusDerivative(Vector<Real,Dim> r, Real h) const noexcept;

private:
    /** Base support radius. */
    virtual Real GetBaseRadius_() const noexcept = 0;
    /** Value of the base smoothing kernel at a point. */ 
    virtual Real GetBaseValue_(Real q) const noexcept = 0;
    /** Derivative value of the base smoothing kernel at a point. */ 
    virtual Real GetBaseDerivativeValue_(Real q) const noexcept = 0;
};  // class TSmoothingKernel

template<typename Real, int Dim>
Real TSmoothingKernel<Real,Dim>::GetValue(Vector<Real,Dim> r, Real h) const noexcept 
{
    TIT_ASSERT(h > 0.0);
    Real hInverse = Inverse(h);
    Real q = hInverse*Norm(r);
    return Pow(hInverse, Dim)*GetBaseValue_(q);
}

template<typename Real, int Dim>
Vector<Real,Dim> TSmoothingKernel<Real,Dim>::GetGradientValue(Vector<Real,Dim> r, Real h) const noexcept 
{
    TIT_ASSERT(h > 0.0);
    Real hInverse = Inverse(h);
    Real q = hInverse*Norm(r);
    return Pow(hInverse, Dim+1)*Normalize(r)*GetBaseDerivativeValue_(q);
}

template<typename Real, int Dim>
Real TSmoothingKernel<Real,Dim>::GetRadius(Real h) const noexcept 
{
    TIT_ASSERT(h > 0.0);
    return GetBaseRadius_()*h;
}

template<typename Real, int Dim>
Real TSmoothingKernel<Real,Dim>::GetRadiusDerivative(Vector<Real,Dim> r, Real h) const noexcept 
{
    TIT_ASSERT(h > 0.0);
    Real hInverse = Inverse(h);
    Real q = hInverse*Norm(r);
    return Pow(hInverse, Dim+1)*(-Dim*GetBaseValue_(q) - q*GetBaseDerivativeValue_(q));
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
    static Real s_Weight;
private:
    Real GetBaseRadius_() const noexcept override;
    Real GetBaseValue_(Real q) const noexcept override;
    Real GetBaseDerivativeValue_(Real q) const noexcept override;
};  // class TSmoothingKernel_Gaussian

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::s_Weight = 
Select(
    Dim == 1, Inverse(SqrtPi<Real>),
    Dim == 2, Inverse(Pi<Real>),
    Dim == 3, Inverse(SqrtPi<Real>*Pi<Real>)
);

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::GetBaseRadius_() const noexcept 
{
    return Infinity<Real>;
}

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::GetBaseValue_(Real q) const noexcept 
{
    return s_Weight*Exp(-Pow2(q));
}

template<typename Real, int Dim>
Real TGaussianSmoothingKernel<Real,Dim>::GetBaseDerivativeValue_(Real q) const noexcept 
{
    return s_Weight*(-Real(2.0)*q*Exp(-Pow2(q)));
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
    static Real s_Weight;
private:
    Real GetBaseRadius_() const noexcept final;
    Real GetBaseValue_(Real q) const noexcept final;
    Real GetBaseDerivativeValue_(Real q) const noexcept override;
};  // class TCubicSmoothingKernel

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::s_Weight = 
Select(
    Dim == 1, Real( 2.0/3.0),
    Dim == 2, Real(10.0/7.0)/Pi<Real>,
    Dim == 3, Real(     1.0)/Pi<Real>
);

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::GetBaseRadius_() const noexcept 
{
    return Real(2.0);
}

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::GetBaseValue_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return s_Weight*(Real(0.25)*Pow3(Real(2.0)-q) - Pow3(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0)) 
    {
        return s_Weight*(Real(0.25)*Pow3(Real(2.0)-q));
    } 
    return Real(0.0);
}

template<typename Real, int Dim>
Real TCubicSmoothingKernel<Real,Dim>::GetBaseDerivativeValue_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return s_Weight*(-Real(0.75)*Pow2(Real(2.0)-q) + Real(3.0)*Pow2(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0)) 
    {
        return s_Weight*(-Real(0.75)*Pow2(Real(2.0)-q));
    }
    return Real(0.0);
}

/********************************************************************
 ** The cubic B-spline (M4) smoothing kernel
 ** with Thomas-Couchman (1992) modified derivative.
 ********************************************************************/
template<typename Real, int Dim>
class TModifiedCubicSmoothingKernel final : public TCubicSmoothingKernel<Real,Dim> 
{
private:
    using TCubicSmoothingKernel<Real,Dim>::s_Weight;
    Real GetBaseDerivativeValue_(Real q) const noexcept override;
};  // class TModifiedCubicSmoothingKernel

template<typename Real, int Dim>
Real TModifiedCubicSmoothingKernel<Real,Dim>::GetBaseDerivativeValue_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(2.0/3.0)) 
    {
        return s_Weight*(-Real(1.0));
    } 
    else if (Real(2.0/3.0) <= q && q < Real(1.0)) 
    {
        return s_Weight*(Real(2.25)*q - Real(3.0))*q;
    }
    else if (Real(1.0) <= q && q < Real(2.0))
    {
        return s_Weight*(Real(0.75)*Pow2(Real(2.0)-q));
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
    static Real s_Weight;
private:
    Real GetBaseRadius_() const noexcept override;
    Real GetBaseValue_(Real q) const noexcept override;
    Real GetBaseDerivativeValue_(Real q) const noexcept override;
};  // class TQuarticSmoothingKernel

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::s_Weight = 
Select(
    Dim == 1, Real(1.0/24.0),
    Dim == 2, Real(96.0/1199.0)/Pi<Real>,
    Dim == 3, Real(    1.0/2.0)/Pi<Real>
);

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::GetBaseRadius_() const noexcept 
{
    return Real(2.5);
}

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::GetBaseValue_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(0.5)) 
    {
        return s_Weight*(Pow4(Real(2.5)-q) - Real(5.0)*Pow4(Real(1.5)-q) + Real(10.0)*Pow4(Real(0.5)-q));
    } 
    else if (Real(0.5) <= q && q < Real(1.5)) 
    {
        return s_Weight*(Pow4(Real(2.5)-q) - Real(5.0)*Pow4(Real(1.5)-q));
    } 
    else if (Real(1.5) <= q && q < Real(2.5)) 
    {
        return s_Weight*(Pow4(Real(2.5)-q));
    } 
    return Real(0.0);
}

template<typename Real, int Dim>
Real TQuarticSmoothingKernel<Real,Dim>::GetBaseDerivativeValue_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(0.5)) 
    {
        return s_Weight*(-Real(4.0)*Pow3(Real(2.5)-q) + Real(20.0)*Pow3(Real(1.5)-q) - Real(40.0)*Pow3(Real(0.5)-q));
    } 
    else if (Real(0.5) <= q && q < Real(1.5)) 
    {
        return s_Weight*(-Real(4.0)*Pow3(Real(2.5)-q) + Real(20.0)*Pow3(Real(1.5)-q));
    } 
    else if (Real(1.5) <= q && q < Real(2.5)) 
    {
        return s_Weight*(-Real(4.0)*Pow3(Real(2.5)-q));
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
    static Real s_Weight;
private:
    Real GetBaseRadius_() const noexcept override;
    Real GetBaseValue_(Real q) const noexcept override;
    Real GetBaseDerivativeValue_(Real q) const noexcept override;
};  // class TQuinticSmoothingKernel

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::s_Weight = 
Select(
    Dim == 1, Real(1.0/120.0),
    Dim == 2, Real(7.0/478.0)/Pi<Real>,
    Dim == 3, Real(1.0/120.0)/Pi<Real>
);

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::GetBaseRadius_() const noexcept 
{
    return Real(3.0);
}

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::GetBaseValue_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return s_Weight*(Pow5(Real(3.0)-q) - Real(6.0)*Pow5(Real(2.0)-q) + Real(15.0)*Pow5(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0))
    {
        return s_Weight*(Pow5(Real(3.0)-q) - Real(6.0)*Pow5(Real(2.0)-q));
    } 
    else if (Real(2.0) <= q && q < Real(3.0)) 
    {
        return s_Weight*(Pow5(Real(3.0)-q));
    } 
    return Real(0.0);
}

template<typename Real, int Dim>
Real TQuinticSmoothingKernel<Real,Dim>::GetBaseDerivativeValue_(Real q) const noexcept 
{
    if (Real(0.0) <= q && q < Real(1.0)) 
    {
        return s_Weight*(-Real(5.0)*Pow4(Real(3.0)-q) + Real(30.0)*Pow4(Real(2.0)-q) - Real(75.0)*Pow4(Real(1.0)-q));
    } 
    else if (Real(1.0) <= q && q < Real(2.0)) 
    {
        return s_Weight*(-Real(5.0)*Pow4(Real(3.0)-q) + Real(30.0)*Pow4(Real(2.0)-q));
    } 
    else if (Real(2.0) <= q && q < Real(3.0)) 
    {
        return s_Weight*(-Real(5.0)*Pow4(Real(3.0)-q));
    } 
    return Real(0.0);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_SMOOTHING_KERNELS_HPP_
