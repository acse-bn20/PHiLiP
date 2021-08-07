#include <iomanip>
#include <cmath>
#include <limits>
#include <type_traits>


#include <assert.h>
#include <deal.II/grid/grid_generator.h>

#include "assert_compare_array.h"
#include "parameters/parameters.h"
#include "physics/navier_stokes.h"

const double TOLERANCE = 1E-5;

int main (int /*argc*/, char * /*argv*/[])
{
    std::cout << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::scientific;
    const int dim = PHILIP_DIM;
    const int nstate = dim+2;

    //const double ref_length = 1.0, mach_inf=1.0, angle_of_attack = 0.0, side_slip_angle = 0.0, gamma_gas = 1.4;
    //const double prandtl_number = 0.72, reynolds_number_inf=50000.0;
    const double a = 1.0 , b = 0.0, c = 1.4, d=0.72, e=1.0e-4;
    PHiLiP::Physics::NavierStokes<dim, nstate, double> navier_stokes_physics = PHiLiP::Physics::NavierStokes<dim, nstate, double>(a,c,a,b,b,d,e);

    const double min = 0.0;
    const double max = 1.0;
    const int nx = 11;

    const double perturbation = 1e-5;

    std::vector<unsigned int> repetitions(dim, nx);
    dealii::Point<dim,double> corner1, corner2;
    for (int d=0; d<dim; d++) { 
        corner1[d] = min;
        corner2[d] = max;
    }
    dealii::Triangulation<dim> grid;
    dealii::GridGenerator::subdivided_hyper_rectangle(grid, repetitions, corner1, corner2);

    // For finite-difference (fourth-order accurate, central difference)
    std::array<double, nstate> soln_plus; // i+1
    std::array<double, nstate> soln_mins; // i-1
    std::array<double, nstate> soln_plusplus; // i+2
    std::array<double, nstate> soln_minsmins; // i-2
    std::array<dealii::Tensor<1,dim,double>,nstate> conv_flux_plus;
    std::array<dealii::Tensor<1,dim,double>,nstate> conv_flux_mins;
    std::array<dealii::Tensor<1,dim,double>,nstate> visc_flux_plus;
    std::array<dealii::Tensor<1,dim,double>,nstate> visc_flux_mins;
    std::array<dealii::Tensor<1,dim,double>,nstate> conv_flux_plusplus;
    std::array<dealii::Tensor<1,dim,double>,nstate> conv_flux_minsmins;
    std::array<dealii::Tensor<1,dim,double>,nstate> visc_flux_plusplus;
    std::array<dealii::Tensor<1,dim,double>,nstate> visc_flux_minsmins;
    std::array<dealii::Tensor<1,dim,double>,nstate> soln_gradient_plus;
    std::array<dealii::Tensor<1,dim,double>,nstate> soln_gradient_mins;
    std::array<dealii::Tensor<1,dim,double>,nstate> soln_gradient_plusplus;
    std::array<dealii::Tensor<1,dim,double>,nstate> soln_gradient_minsmins;

    std::array<dealii::Tensor<1,dim,double>,nstate> soln_gradient;

    for (auto cell : grid.active_cell_iterators()) {
        for (unsigned int v=0; v < dealii::GeometryInfo<dim>::vertices_per_cell; ++v) {

            const dealii::Point<dim,double> vertex = cell->vertex(v);
            std::array<double, nstate> source_term = navier_stokes_physics.source_term(vertex, soln_plus);

            std::array<double, nstate> divergence_finite_differences;
            divergence_finite_differences.fill(0.0);

            std::vector<dealii::Tensor<1,dim,double>> manufactured_solution_gradient_dealii(nstate);
            // - compute gradient @ vertex
            navier_stokes_physics.manufactured_solution_function->vector_gradient(vertex, manufactured_solution_gradient_dealii);
            for (int d=0;d<dim;d++) {
                for (int s=0; s<nstate; s++) {
                    soln_gradient[s][d] = manufactured_solution_gradient_dealii[s][d];
                }
            }

            for (int d=0; d<dim; d++) {
                dealii::Point<dim,double> vertex_plus = vertex;
                dealii::Point<dim,double> vertex_mins = vertex;
                vertex_plus[d] = vertex[d] + perturbation;
                vertex_mins[d] = vertex[d] - perturbation;
                dealii::Point<dim,double> vertex_plusplus = vertex;
                dealii::Point<dim,double> vertex_minsmins = vertex;
                vertex_plusplus[d] = vertex[d] + 2.0*perturbation;
                vertex_minsmins[d] = vertex[d] - 2.0*perturbation;
                
                // Get manufactured solution value
                for (int s=0; s<nstate; s++) {
                    soln_plus[s] = navier_stokes_physics.manufactured_solution_function->value(vertex_plus, s);
                    soln_mins[s] = navier_stokes_physics.manufactured_solution_function->value(vertex_mins, s);
                    soln_plusplus[s] = navier_stokes_physics.manufactured_solution_function->value(vertex_plusplus, s);
                    soln_minsmins[s] = navier_stokes_physics.manufactured_solution_function->value(vertex_minsmins, s);
                }

                // Get manufactured solution gradients
                // - compute gradient @ vertex_plus
                navier_stokes_physics.manufactured_solution_function->vector_gradient(vertex_plus, manufactured_solution_gradient_dealii);
                for (int d=0;d<dim;d++) {
                    for (int s=0; s<nstate; s++) {
                        soln_gradient_plus[s][d] = manufactured_solution_gradient_dealii[s][d];
                    }
                }
                // - compute gradient @ vertex_mins
                navier_stokes_physics.manufactured_solution_function->vector_gradient(vertex_mins, manufactured_solution_gradient_dealii);
                for (int d=0;d<dim;d++) {
                    for (int s=0; s<nstate; s++) {
                        soln_gradient_mins[s][d] = manufactured_solution_gradient_dealii[s][d];
                    }
                }
                // - compute gradient @ vertex_plusplus
                navier_stokes_physics.manufactured_solution_function->vector_gradient(vertex_plusplus, manufactured_solution_gradient_dealii);
                for (int d=0;d<dim;d++) {
                    for (int s=0; s<nstate; s++) {
                        soln_gradient_plusplus[s][d] = manufactured_solution_gradient_dealii[s][d];
                    }
                }
                // - compute gradient @ vertex_minsmins
                navier_stokes_physics.manufactured_solution_function->vector_gradient(vertex_minsmins, manufactured_solution_gradient_dealii);
                for (int d=0;d<dim;d++) {
                    for (int s=0; s<nstate; s++) {
                        soln_gradient_minsmins[s][d] = manufactured_solution_gradient_dealii[s][d];
                    }
                }

                conv_flux_plus     = navier_stokes_physics.convective_flux(soln_plus);
                conv_flux_mins     = navier_stokes_physics.convective_flux(soln_mins);
                visc_flux_plus     = navier_stokes_physics.dissipative_flux(soln_plus,soln_gradient_plus);
                visc_flux_mins     = navier_stokes_physics.dissipative_flux(soln_mins,soln_gradient_mins);
                // visc_flux_plus     = navier_stokes_physics.dissipative_flux(soln_plus,soln_gradient);
                // visc_flux_mins     = navier_stokes_physics.dissipative_flux(soln_mins,soln_gradient);
                conv_flux_plusplus = navier_stokes_physics.convective_flux(soln_plusplus);
                conv_flux_minsmins = navier_stokes_physics.convective_flux(soln_minsmins);
                visc_flux_plusplus = navier_stokes_physics.dissipative_flux(soln_plusplus,soln_gradient_plusplus);
                visc_flux_minsmins = navier_stokes_physics.dissipative_flux(soln_minsmins,soln_gradient_minsmins);
                // visc_flux_plusplus = navier_stokes_physics.dissipative_flux(soln_plusplus,soln_gradient);
                // visc_flux_minsmins = navier_stokes_physics.dissipative_flux(soln_minsmins,soln_gradient);

                for (int s=0; s<nstate; s++) {
                    // divergence_finite_differences[s] += (conv_flux_plus[s][d] - conv_flux_mins[s][d]) / (2.0 * perturbation);
                    // divergence_finite_differences[s] += (visc_flux_plus[s][d] - visc_flux_mins[s][d]) / (2.0 * perturbation);
                    divergence_finite_differences[s] += (-0.25*conv_flux_plusplus[s][d] + 2.0*conv_flux_plus[s][d] - 2.0*conv_flux_mins[s][d] + 0.25*conv_flux_minsmins[s][d]) / (3.0 * perturbation);
                    divergence_finite_differences[s] += (-0.25*visc_flux_plusplus[s][d] + 2.0*visc_flux_plus[s][d] - 2.0*visc_flux_mins[s][d] + 0.25*visc_flux_minsmins[s][d]) / (3.0 * perturbation);
                }
            }

            assert_compare_array<nstate> ( divergence_finite_differences, source_term, 1.0, TOLERANCE);
        }
    }
    return 0;
}

