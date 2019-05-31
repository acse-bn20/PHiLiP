#ifndef __PHYSICS__
#define __PHYSICS__

#include <deal.II/base/tensor.h>

#include "parameters/all_parameters.h"

namespace PHiLiP {
namespace Physics {

/// Base class from which Advection, Diffusion, ConvectionDiffusion, and Euler is derived.
/**
 *  Main interface for all the convective and diffusive terms.
 *
 *  LinearAdvection, Diffusion, ConvectionDiffusion, Euler are derived from this class.
 *
 *  Partial differential equation is given by the divergence of the convective and
 *  diffusive flux equal to the source term
 *
 *  \f[ \boldsymbol{\nabla} \cdot
 *         (  \mathbf{F}_{conv}( u ) 
 *          + \mathbf{F}_{diss}( u, \boldsymbol{\nabla}(u) )
 *      = s(\mathbf{x})
 *  \f]
 */
template <int dim, int nstate, typename real>
class PhysicsBase
{
public:
    /// Virtual destructor required for abstract classes.
    virtual ~PhysicsBase() = 0;

    /// Default manufactured solution.
    /** ~~~~~{.cpp}
     *  if (dim==1) uexact = sin(a*pos[0]+d);
     *  if (dim==2) uexact = sin(a*pos[0]+d)*sin(b*pos[1]+e);
     *  if (dim==3) uexact = sin(a*pos[0]+d)*sin(b*pos[1]+e)*sin(c*pos[2]+f);
     *  ~~~~~
     */
    virtual void manufactured_solution (
        const dealii::Point<dim,double> &pos,
        //std::array<real,nstate> &solution) const;
        real *const solution) const;

    /// Default manufactured solution gradient.
    virtual void manufactured_gradient (
        const dealii::Point<dim,double> &pos,
        std::array<dealii::Tensor<1,dim,real>,nstate> &solution_gradient) const;

    /// Returns the integral of the manufactured solution over the hypercube [0,1].
    ///
    /// Either returns the linear output \f$\int u dV\f$
    /// or the nonlinear output \f$\int u^2 dV\f$.
    virtual double integral_output (const bool linear) const;

    /// Convective fluxes that will be differentiated once in space.
    virtual void convective_flux (
        const std::array<real,nstate> &solution,
        std::array<dealii::Tensor<1,dim,real>,nstate> &conv_flux) const = 0;

    /// Spectral radius of convective term Jacobian.
    /// Used for scalar dissipation
    virtual std::array<real,nstate> convective_eigenvalues (
        const std::array<real,nstate> &/*solution*/,
        const dealii::Tensor<1,dim,real> &/*normal*/) const = 0;

    // /// Evaluate the diffusion matrix \f$ A \f$ such that \f$F_v = A \nabla u\f$.
    // virtual std::array<dealii::Tensor<1,dim,real>,nstate> apply_diffusion_matrix (
    //     const std::array<real,nstate> &solution,
    //     const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_grad) const = 0;

    /// Dissipative fluxes that will be differentiated once in space.
    /// Evaluates the dissipative flux through the linearization F = A(u)*grad(u).
    void dissipative_flux_A_gradu (
        const real scaling,
        const std::array<real,nstate> &solution,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_gradient,
        std::array<dealii::Tensor<1,dim,real>,nstate> &diss_flux) const;

    /// Dissipative fluxes that will be differentiated once in space.
    virtual void dissipative_flux (
        const std::array<real,nstate> &solution,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_gradient,
        std::array<dealii::Tensor<1,dim,real>,nstate> &diss_flux) const = 0;

    /// Source term that does not require differentiation.
    virtual void source_term (
        const dealii::Point<dim,double> &pos,
        const std::array<real,nstate> &solution,
        std::array<real,nstate> &source) const = 0;

    /// Evaluates boundary values and gradients on the other side of the face.
    virtual void boundary_face_values (
        const int /*boundary_type*/,
        const dealii::Point<dim, double> &/*pos*/,
        const dealii::Tensor<1,dim,real> &/*normal*/,
        const std::array<real,nstate> &/*soln_int*/,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &/*soln_grad_int*/,
        std::array<real,nstate> &/*soln_bc*/,
        std::array<dealii::Tensor<1,dim,real>,nstate> &/*soln_grad_bc*/) const;
protected:
    /// Not yet implemented
    virtual void set_manufactured_dirichlet_boundary_condition (
        const std::array<real,nstate> &/*soln_int*/,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &/*soln_grad_int*/,
        std::array<real,nstate> &/*soln_bc*/,
        std::array<dealii::Tensor<1,dim,real>,nstate> &/*soln_grad_bc*/) const;
    /// Not yet implemented
    virtual void set_manufactured_neumann_boundary_condition (
        const std::array<real,nstate> &/*soln_int*/,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &/*soln_grad_int*/,
        std::array<real,nstate> &/*soln_bc*/,
        std::array<dealii::Tensor<1,dim,real>,nstate> &/*soln_grad_bc*/) const;


    /// Constant \f$\pi\f$
    const double pi = atan(1)*4.0;

    /// Some constants used to define manufactured solution
    const double freq_x = 0.59/dim, freq_y = 2*0.81/dim,    freq_z = 3*0.76/dim;
    const double offs_x = 1,        offs_y = 1.2,           offs_z = 1.5;
    const double velo_x = exp(1)/2, velo_y =-pi/4.0,        velo_z = sqrt(2);
    //const double velo_x = 1.0, velo_y =-pi/4.0,        velo_z = sqrt(2);
    const double diff_coeff = 5.0;

    /// Heterogeneous diffusion matrix
    /** As long as the diagonal components are positive and diagonally dominant
     *  we should have a stable diffusive system
     */
    const double A11 =   9, A12 =  -2, A13 =  -6;
    const double A21 =   3, A22 =  20, A23 =   4;
    const double A31 =  -2, A32 = 0.5, A33 =   8;
};

/// Create specified physics as PhysicsBase object 
/** Factory design pattern whose job is to create the correct physics
 */
template <int dim, int nstate, typename real>
class PhysicsFactory
{
public:
    static PhysicsBase<dim,nstate,real>*
        create_Physics(Parameters::AllParameters::PartialDifferentialEquation pde_type);
};


/// Linear advection equation. Derived from PhysicsBase
/**
 *  Also allows the use of vector-valued linear advection for 2 state variables,
 *  independent of the number of dimensions.
 *
 *  State variable: \f[ u \f]
 *  
 *  Equation: \f[ \boldsymbol\nabla \cdot (c*u) = s \f]
 */
template <int dim, int nstate, typename real>
class LinearAdvection : public PhysicsBase <dim, nstate, real>
{

public:
    ~LinearAdvection () {};
    /// Convective flux:  c*u
    void convective_flux (
        const std::array<real,nstate> &solution,
        std::array<dealii::Tensor<1,dim,real>,nstate> &conv_flux) const;

    /// Spectral radius of convective term Jacobian is simply the maximum 'c'
    std::array<real,nstate> convective_eigenvalues (
        const std::array<real,nstate> &/*solution*/,
        const dealii::Tensor<1,dim,real> &normal) const;

    //  /// Diffusion matrix: 0
    //  std::array<dealii::Tensor<1,dim,real>,nstate> apply_diffusion_matrix (
    //      const std::array<real,nstate> &solution,
    //      const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_grad) const;

    /// Dissipative flux: 0
    void dissipative_flux (
        const std::array<real,nstate> &solution,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_gradient,
        std::array<dealii::Tensor<1,dim,real>,nstate> &diss_flux) const;

    /// Source term is zero or depends on manufactured solution
    void source_term (
        const dealii::Point<dim,double> &pos,
        const std::array<real,nstate> &solution,
        std::array<real,nstate> &source) const;

protected:
    /// Linear advection speed:  c
    dealii::Tensor<1,dim,real> advection_speed () const;

};

/// Poisson equation. Derived from PhysicsBase
/** State variable: \f$ u \f$
 *  
 *  Convective flux \f$ \mathbf{F}_{conv} =  0 \f$
 *
 *  Dissipative flux \f$ \mathbf{F}_{diss} = -\boldsymbol\nabla u \f$
 *
 *  Source term \f$ s(\mathbf{x}) \f$
 *
 *  Equation:
 *  \f[ \boldsymbol{\nabla} \cdot
 *         (\mathbf{F}_{diss}( u, \boldsymbol{\nabla}(u) )
 *      = s(\mathbf{x})
 *  \f]
 */
template <int dim, int nstate, typename real>
class Diffusion : public PhysicsBase <dim, nstate, real>
{
public:
    ~Diffusion () {};
    /// Convective flux:  0
    void convective_flux (
        const std::array<real,nstate> &solution,
        std::array<dealii::Tensor<1,dim,real>,nstate> &conv_flux) const;

    /// Convective eigenvalues dotted with normal
    std::array<real,nstate> convective_eigenvalues (
        const std::array<real,nstate> &/*solution*/,
        const dealii::Tensor<1,dim,real> &/*normal*/) const;

    //  /// Diffusion matrix is identity
    //  std::array<dealii::Tensor<1,dim,real>,nstate> apply_diffusion_matrix (
    //      const std::array<real,nstate> &solution,
    //      const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_grad) const;

    /// Dissipative flux: u
    void dissipative_flux (
        const std::array<real,nstate> &solution,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_gradient,
        std::array<dealii::Tensor<1,dim,real>,nstate> &diss_flux) const;

    /// Source term is zero or depends on manufactured solution
    void source_term (
        const dealii::Point<dim,double> &pos,
        const std::array<real,nstate> &solution,
        std::array<real,nstate> &source) const;
};

/// Convection-diffusion with linear advective and diffusive term.  Derived from PhysicsBase.
/** State variable: \f$ u \f$
 *  
 *  Convective flux \f$ \mathbf{F}_{conv} =  u \f$
 *
 *  Dissipative flux \f$ \mathbf{F}_{diss} = -\boldsymbol\nabla u \f$
 *
 *  Source term \f$ s(\mathbf{x}) \f$
 *
 *  Equation:
 *  \f[ \boldsymbol{\nabla} \cdot
 *         (  \mathbf{F}_{conv}( u ) 
 *          + \mathbf{F}_{diss}( u, \boldsymbol{\nabla}(u) )
 *      = s(\mathbf{x})
 *  \f]
 */
template <int dim, int nstate, typename real>
class ConvectionDiffusion : public PhysicsBase <dim, nstate, real>
{
public:
    ~ConvectionDiffusion () {};
    /// Convective flux: \f$ \mathbf{F}_{conv} =  u \f$
    void convective_flux (
        const std::array<real,nstate> &solution,
        std::array<dealii::Tensor<1,dim,real>,nstate> &conv_flux) const;

    /// Spectral radius of convective term Jacobian is 'c'
    std::array<real,nstate> convective_eigenvalues (
        const std::array<real,nstate> &/*solution*/,
        const dealii::Tensor<1,dim,real> &/*normal*/) const;

    //  /// Diffusion matrix is identity
    //  std::array<dealii::Tensor<1,dim,real>,nstate> apply_diffusion_matrix (
    //      const std::array<real,nstate> &solution,
    //      const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_grad) const;

    /// Dissipative flux: u
    void dissipative_flux (
        const std::array<real,nstate> &solution,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_gradient,
        std::array<dealii::Tensor<1,dim,real>,nstate> &diss_flux) const;

    /// Source term is zero or depends on manufactured solution
    void source_term (
        const dealii::Point<dim,double> &pos,
        const std::array<real,nstate> &solution,
        std::array<real,nstate> &source) const;

protected:
    /// Linear advection speed:  c
    dealii::Tensor<1,dim,real> advection_speed () const;
};

/// Euler equations. Derived from PhysicsBase
/** Only 2D and 3D
 *  State variable and convective fluxes given by
 *
 *  \f[ 
 *  \mathbf{u} = 
 *  \begin{bmatrix} \rho \\ \rho v_1 \\ \rho v_2 \\ \rho v_3 \\ e \end{bmatrix}
 *  , \qquad
 *  \mathbf{F}_{conv} = 
 *  \begin{bmatrix} 
 *      \mathbf{f}^x_{conv}, \mathbf{f}^y_{conv}, \mathbf{f}^z_{conv}
 *  \end{bmatrix}
 *  =
 *  \begin{bmatrix} 
 *  \begin{bmatrix} 
 *  \rho v_1 \\
 *  \rho v_1 v_1 + p \\
 *  \rho v_1 v_2     \\ 
 *  \rho v_1 v_3     \\
 *  v_1 (e+p)
 *  \end{bmatrix}
 *  ,
 *  \begin{bmatrix} 
 *  \rho v_2 \\
 *  \rho v_1 v_2     \\
 *  \rho v_2 v_2 + p \\ 
 *  \rho v_2 v_3     \\
 *  v_2 (e+p)
 *  \end{bmatrix}
 *  ,
 *  \begin{bmatrix} 
 *  \rho v_3 \\
 *  \rho v_1 v_3     \\
 *  \rho v_2 v_3     \\ 
 *  \rho v_3 v_3 + p \\
 *  v_3 (e+p)
 *  \end{bmatrix}
 *  \end{bmatrix} \f]
 *  
 *  For a calorically perfect gas
 *
 *  \f[
 *  p=(\gamma -1)(e-\frac{1}{2}\rho \|\mathbf{v}\|)
 *  \f]
 *
 *  Dissipative flux \f$ \mathbf{F}_{diss} = \mathbf{0} \f$
 *
 *  Source term \f$ s(\mathbf{x}) \f$
 *
 *  Equation:
 *  \f[ \boldsymbol{\nabla} \cdot
 *         (  \mathbf{F}_{conv}( u ) 
 *          + \mathbf{F}_{diss}( u, \boldsymbol{\nabla}(u) )
 *      = s(\mathbf{x})
 *  \f]
 */
template <int dim, int nstate, typename real>
class Euler : public PhysicsBase <dim, nstate, real>
{
public:
    ~Euler () {};

    /// Convective flux: \f$ \mathbf{F}_{conv} =  u \f$
    void convective_flux (
        const std::array<real,nstate> &solution,
        std::array<dealii::Tensor<1,dim,real>,nstate> &conv_flux) const;

    /// Spectral radius of convective term Jacobian is 'c'
    std::array<real,nstate> convective_eigenvalues (
        const std::array<real,nstate> &/*solution*/,
        const dealii::Tensor<1,dim,real> &/*normal*/) const;

    //  /// Diffusion matrix is identity
    //  std::array<dealii::Tensor<1,dim,real>,nstate> apply_diffusion_matrix (
    //      const std::array<real,nstate> &solution,
    //      const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_grad) const;

    /// Dissipative flux: u
    void dissipative_flux (
        const std::array<real,nstate> &solution,
        const std::array<dealii::Tensor<1,dim,real>,nstate> &solution_gradient,
        std::array<dealii::Tensor<1,dim,real>,nstate> &diss_flux) const;

    /// Source term is zero or depends on manufactured solution
    void source_term (
        const dealii::Point<dim,double> &pos,
        const std::array<real,nstate> &solution,
        std::array<real,nstate> &source) const;

protected:
    const real gam = 1.4;
    real compute_pressure ( const std::array<real,nstate> &solution ) const;
    real compute_sound ( const std::array<real,nstate> &solution ) const;
    std::array<real,dim> compute_velocities ( const std::array<real,nstate> &solution ) const;

};

} // Physics namespace
} // PHiLiP namespace

#endif