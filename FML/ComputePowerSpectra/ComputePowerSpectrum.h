#ifndef COMPUTEPOWERSPECTRUM_HEADER
#define COMPUTEPOWERSPECTRUM_HEADER

#include <vector>
#include <iostream>
#include <cassert>

#ifdef USE_MPI
#include <mpi.h>
#endif

#ifdef USE_OMP
#include <omp.h>
#endif

#include <FML/Global/Global.h>
#include <FML/FFTWGrid/FFTWGrid.h>
#include <FML/Interpolation/ParticleGridInterpolation.h>
#include <FML/MPIParticles/MPIParticles.h> // Only for compute_multipoles from particles

// The classes that define how to bin and return the data
#include <FML/ComputePowerSpectra/PowerSpectrumBinning.h>
#include <FML/ComputePowerSpectra/BispectrumBinning.h>
#include <FML/ComputePowerSpectra/PolyspectrumBinning.h>

namespace FML {
  namespace CORRELATIONFUNCTIONS {

    using namespace FML::INTERPOLATION;

    template<int N>
      using FFTWGrid = FML::GRID::FFTWGrid<N>;

    // Keep track of everything we need for a binned power-spectrum
    // The k-spacing (linear or log or ...), the count in each bin,
    // the power in each bin etc. Its through this interface the results
    // of the methods below are given. See PowerSpectrumBinning.h for how it works
    template<int N> 
      class PowerSpectrumBinning;

    // The bispectrum result, see BispectrumBinning.h
    template<int N> 
      class BispectrumBinning;

    // General polyspectrum result, see PolyspectrumBinning.h
    template<int N, int ORDER>
      class PolyspectrumBinning;

    //=====================================================================
    //=====================================================================

    // Assign particles to grid using density_assignment_method = NGP,CIC,TSC,PCS,...
    // Fourier transform, decolvolve the window function for the assignement above,
    // bin up power-spectrum and subtract shot-noise 1/NumPartTotal
    template<int N, class T>
      void compute_power_spectrum(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          PowerSpectrumBinning<N> &pofk,
          std::string density_assignment_method);

    // Assign particles to grid using density_assignment_method = NGP,CIC,TSC,PCS,...
    // Assign particles to an interlaced grid (displaced by dx/2 in all directions)
    // Fourier transform both and add the together to cancel the leading aliasing contributions
    // Decolvolve the window function for the assignements above,
    // bin up power-spectrum and subtract shot-noise 1/NumPartTotal
    template<int N, class T>
      void compute_power_spectrum_interlacing(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          PowerSpectrumBinning<N> &pofk,
          std::string density_assignment_method);

    // Brute force. Add particles to the grid using direct summation
    // This gives alias free P(k), but scales as O(Npart)*O(Nmesh^N)
    template<int N, class T>
      void compute_power_spectrum_direct_summation(
          int Ngrid, 
          T *part, 
          size_t NumPart, 
          PowerSpectrumBinning<N> &pofk);

    // Bin up the power-spectrum of a given fourier grid
    template <int N>
      void bin_up_power_spectrum(
          FFTWGrid<N> &fourier_grid,
          PowerSpectrumBinning<N> &pofk);

    // Compute power-spectrum multipoles P0,P1,...,Pn-1 for the case
    // where we have a fixed line_of_sight_direction (typical coordinate axes like (0,0,1))
    // Pell contains P0,P1,P2,...Pell_max where ell_max = n-1 is the size of Pell
    template <int N>
      void compute_power_spectrum_multipoles(
          FFTWGrid<N> &fourier_grid, 
          std::vector< PowerSpectrumBinning<N> > &Pell,
          std::vector<double> line_of_sight_direction);

    // Simple method to estimate multipoles from simulation data
    // Take particles in realspace and use their velocity to put them into
    // redshift space. Fourier transform and compute multipoles from this like in the method above.
    // We do this for all coordinate axes and return the mean P0,P1,... we get from this
    // velocity_to_displacement is factor to convert your velocity to a coordinate shift in [0,1)
    // e.g. c/(a H Box) with H ~ 100 h km/s/Mpc and Box boxsize in Mpc/h if velocities are peculiar
    template<int N, class T>
      void compute_power_spectrum_multipoles(
          int Ngrid,
          FML::PARTICLE::MPIParticles<T> & part,
          double velocity_to_displacement,
          std::vector< PowerSpectrumBinning<N> > &Pell,
          std::string density_assignment_method);

    // Computes the bispectrum B(k1,k2,k3) from particles
    template<int N, class T>
      void compute_bispectrum(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          BispectrumBinning<N> &bofk,
          std::string density_assignment_method);

    // Computes the bispectrum B(k1,k2,k3) from a fourier grid
    template<int N>
      void compute_bispectrum(
          FFTWGrid<N> & density_k,
          BispectrumBinning<N> &bofk);

    // Computes the polyspectrum P(k1,k2,k3,...) from particles
    template<int N, class T, int ORDER>
      void compute_polyspectrum(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          PolyspectrumBinning<N,ORDER> &polyofk,
          std::string density_assignment_method);

    // Compute the polyspectrum <F1 F2 ... Forder> from a fourier grid
    template<int N, int ORDER>
      void compute_polyspectrum(
          FFTWGrid<N> & density_k,
          PolyspectrumBinning<N,ORDER> &polyofk);

    //=====================================================================
    //=====================================================================

    //==========================================================================================
    // Compute the power-spectrum multipoles of a fourier grid assuming a fixed line of sight
    // direction (typically coordinate axes). Provide Pell with [ell+1] initialized binnings to compute
    // the first 0,1,...,ell multipoles The result has no scales. Get scales by scaling
    // PowerSpectrumBinning using scale(kscale, pofkscale) with kscale = 1/Boxsize
    // and pofkscale = Boxsize^N once spectrum has been computed
    //==========================================================================================
    template <int N>
      void compute_power_spectrum_multipoles(
          FFTWGrid<N> &fourier_grid, 
          std::vector<PowerSpectrumBinning<N>> &Pell,
          std::vector<double> line_of_sight_direction) { 

        assert_mpi(line_of_sight_direction.size() == N,
            "[compute_power_spectrum_multipoles] Line of sight direction has wrong number of dimensions\n");
        assert_mpi(Pell.size() > 0, 
            "[compute_power_spectrum_multipoles] Pell must have size > 0\n");
        assert_mpi(fourier_grid.get_nmesh() > 0, 
            "[compute_power_spectrum_multipoles] grid must have Nmesh > 0\n");

        int Nmesh           = fourier_grid.get_nmesh();
        auto Local_nx       = fourier_grid.get_local_nx();
        auto NmeshTotTotal  = size_t(Local_nx) * FML::power(size_t(Nmesh),N-2) * size_t(Nmesh/2+1);//fourier_grid.get_ntot_fourier();
        auto *cdelta = fourier_grid.get_fourier_grid();

        // Norm of LOS vector
        double rnorm = 0.0;
        for(int idim = 0; idim < N; idim++)
          rnorm += line_of_sight_direction[idim]*line_of_sight_direction[idim];
        rnorm = std::sqrt(rnorm);
        assert_mpi(rnorm > 0.0, 
            "[compute_power_spectrum_multipoles] Line of sight vector has zero length\n");

        // Initialize binning just in case
        for (size_t ell = 0; ell < Pell.size(); ell++)
          Pell[ell].reset();

        // Bin up mu^k |delta|^2
#ifdef USE_OMP
#pragma omp parallel for
#endif
        for (size_t ind = 0; ind < NmeshTotTotal; ind++) {
          // Special treatment of k = 0 plane
          int last_coord = ind % (Nmesh / 2 + 1);
          double weight = last_coord > 0 && last_coord < Nmesh / 2 ? 2.0 : 1.0;

          // Compute kvec, |kvec| and |delta|^2
          double kmag;
          std::vector<double> kvec(N);
          fourier_grid.get_fourier_wavevector_and_norm_by_index(ind, kvec, kmag);
          double power = std::norm(cdelta[ind]);

          // Compute mu = k_vec*r_vec
          double mu = 0.0;
          for(int idim = 0; idim < N; idim++)
            mu += kvec[idim] * line_of_sight_direction[idim];
          mu /= (kmag * rnorm);

          // Add to bin |delta|^2, |delta|^2mu^2, |delta^2|mu^4, ...
          double mutoell = 1.0;
          for (size_t ell = 0; ell < Pell.size(); ell++) {
            Pell[ell].add_to_bin(kmag, power * mutoell, weight);
            mutoell *= mu;
            //Pell[ell].add_to_bin(kmag, power * std::pow(mu, ell), weight);
          }
        }

#ifdef USE_MPI
        for (size_t ell = 0; ell < Pell.size(); ell++) {
          MPI_Allreduce(MPI_IN_PLACE, Pell[ell].pofk.data(),  Pell[ell].n, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
          MPI_Allreduce(MPI_IN_PLACE, Pell[ell].count.data(), Pell[ell].n, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
          MPI_Allreduce(MPI_IN_PLACE, Pell[ell].kbin.data(),  Pell[ell].n, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        }
#endif

        // Normalize
        for (size_t ell = 0; ell < Pell.size(); ell++)
          Pell[ell].normalize();

        // Binomial coefficient
        auto binomial = [](const double n, const double k){
          double res = 1.0;
          for (int i = 0; i < k; i++) {
            res *= double(n - i) / double(k - i);
          }
          return res;
        };

        // P_ell(x) = Sum_{k=0}^{ell/2} summand_legendre_polynomial * x^(ell - 2k)
        auto summand_legendre_polynomial = [&](const int k, const int ell) {
          double sign = (k % 2) == 0 ? 1.0 : -1.0;
          return sign * binomial(ell, k) * binomial(2 * ell - 2 * k, ell) /
            std::pow(2.0, ell);
        };

        // Go from <mu^k |delta|^2> to <L_ell(mu) |delta|^2>
        std::vector<std::vector<double>> temp;
        for (size_t ell = 0; ell < Pell.size(); ell++) {
          std::vector<double> sum(Pell[0].pofk.size(), 0.0);
          for (size_t k = 0; k <= ell / 2; k++) {
            std::vector<double> mu_power = Pell[ell - 2 * k].pofk;
            for(size_t i = 0; i < sum.size(); i++)
              sum[i] += mu_power[i] * summand_legendre_polynomial(k, ell);
          }
          temp.push_back(sum);
        }

        // Copy over data. We now have P0,P1,... in Pell
        for (size_t ell = 0; ell < Pell.size(); ell++) {
          Pell[ell].pofk = temp[ell];
        }
      }

    //==========================================================================================
    // Compute the power-spectrum of a fourier grid. The result has no scales. Get
    // scales by calling pofk.scale(kscale, pofkscale) with kscale = 1/Boxsize and
    // pofkscale = Boxsize^N once spectrum has been computed
    //==========================================================================================
    template <int N>
      void bin_up_power_spectrum(
          FFTWGrid<N> &fourier_grid,
          PowerSpectrumBinning<N> &pofk) {

        assert_mpi(fourier_grid.get_nmesh() > 0, 
            "[bin_up_power_spectrum] grid must have Nmesh > 0\n");
        assert_mpi(pofk.n > 0 && pofk.kmax > pofk.kmin && pofk.kmin >= 0.0, 
            "[bin_up_power_spectrum] Binning has inconsistent parameters\n");

        auto *cdelta = fourier_grid.get_fourier_grid();
        int Nmesh = fourier_grid.get_nmesh();
        auto Local_nx = fourier_grid.get_local_nx();
        auto NmeshTotLocal = size_t(Local_nx) * FML::power(size_t(Nmesh),N-2) * size_t(Nmesh/2+1);//fourier_grid.get_ntot_fourier();

        // Initialize binning just in case
        pofk.reset();

        // Bin up P(k)
#ifdef USE_OMP
#pragma omp parallel for
#endif
        for (size_t ind = 0; ind < NmeshTotLocal; ind++) {
          // Special treatment of k = 0 plane
          int last_coord = ind % (Nmesh / 2 + 1);
          double weight = last_coord > 0 && last_coord < Nmesh / 2 ? 2.0 : 1.0;

          auto delta_norm = std::norm(cdelta[ind]);

          // Add norm to bin
          double kmag;
          std::vector<double> kvec(N);
          fourier_grid.get_fourier_wavevector_and_norm_by_index(ind, kvec, kmag);
          pofk.add_to_bin(kmag, delta_norm, weight);
        }

        // Normalize to get P(k) (this communicates over tasks)
        pofk.normalize();
      }

    //==============================================================================================
    // Brute force (but aliasing free) computation of the power spectrum
    // Loop over all grid-cells and all particles and add up contribution and subtracts shot-noise term
    // Since we need to combine all particles with all cells this is not easiy parallelizable with MPI
    // so we assume all CPUs have exactly the same particles when this is run on more than 1 MPI tasks
    //==============================================================================================

    template<int N, class T>
      void compute_power_spectrum_direct_summation(
          int Ngrid, 
          T *part, 
          size_t NumPart, 
          PowerSpectrumBinning<N> &pofk)
      {

        assert_mpi(Ngrid > 0, 
            "[direct_summation_power_spectrum] Ngrid > 0 required\n");
        if(NTasks > 1 and ThisTask == 0) 
          std::cout << "[direct_summation_power_spectrum] Warning: this method assumes all tasks have the same particles\n";

        const std::complex<double> I(0,1);
        const double norm = 1.0/double(NumPart);

        FFTWGrid<N> density_k(Ngrid, 1, 1);
        density_k.add_memory_label("FFTWGrid::compute_power_spectrum_direct_summation::density_k");

        auto *f = density_k.get_fourier_grid();
        for(auto && complex_index: density_k.get_fourier_range()) {
          auto kvec = density_k.get_fourier_wavevector_from_index(complex_index);
          double real = 0.0;
          double imag = 0.0;
#ifdef USE_OMP
#pragma omp parallel for reduction(+:real) reduction(+:imag) 
#endif
          for(size_t i = 0; i < NumPart; i++){
            auto *x = part[i].get_pos();
            double kx = 0.0;
            for(int idim = 0; idim < N; idim++){
              kx += kvec[idim] * x[idim];
            }
            auto val = std::exp(-kx * I);
            real += val.real();
            imag += val.imag();
          }
          std::complex<double> sum = {real, imag};
          if(ThisTask == 0 and complex_index == 0)
            sum -= 1.0;
          f[complex_index] = sum * norm;
        }

        // Bin up the power-spectrum
        bin_up_power_spectrum<N>(density_k, pofk);

        // Subtract shot-noise
        for(int i = 0; i < pofk.n; i++)
          pofk.pofk[i] -= 1.0 / double(NumPart);
      }

    //================================================================================
    // A simple power-spectrum estimator for multipoles in simulations - nothing fancy
    // Displacing particles from realspace to redshift-space using their velocities
    // along each of the coordinate axes. Result is the mean of this
    // Deconvolving the window-function and subtracting shot-noise (1/NumPartTotal) (for monopole)
    //
    // velocity_to_distance is the factor to convert a velocity to a displacement in units
    // of the boxsize. This is c / ( aH(a) Boxsize ) for peculiar and c / (H(a)Boxsize) for comoving velocities 
    // At z = 0 velocity_to_displacement = 1.0/(100 * Boxsize) when Boxsize is in Mpc/h
    //================================================================================
    template<int N, class T>
      void compute_power_spectrum_multipoles(
          int Ngrid,
          FML::PARTICLE::MPIParticles<T> & part,
          double velocity_to_displacement,
          std::vector< PowerSpectrumBinning<N> > &Pell,
          std::string density_assignment_method) {

        // Set how many extra slices we need for the density assignment to go smoothly
        auto nleftright = get_extra_slices_needed_for_density_assignment(density_assignment_method);
        const int nleft  = nleftright.first;
        const int nright = nleftright.second;

        // Initialize binning just in case
        for (size_t ell = 0; ell < Pell.size(); ell++)
          Pell[ell].reset();

        // Set a binning for each axes
        std::vector<std::vector< PowerSpectrumBinning<N> >> Pell_all(N);
        for(int dir = 0; dir < N; dir++) {
          Pell_all[dir] = Pell;
        }

        // Loop over all the axes we are going to put the particles
        // into redshift space
        for(int dir = 0; dir < N; dir++) {

          // Set up binning for current axis
          std::vector< PowerSpectrumBinning<N> > Pell_current = Pell_all[dir];
          for (size_t ell = 0; ell < Pell_current.size(); ell++)
            Pell_current[ell].reset();

          // Make line of sight direction unit vector
          std::vector<double> line_of_sight_direction(N,0.0);
          line_of_sight_direction[dir] = 1.0;

          // Displace particles
#ifdef USE_OMP
#pragma omp parallel for
#endif
          for(size_t i = 0; i < part.get_npart(); i++){
            auto *pos = part[i].get_pos();
            auto *vel = part[i].get_vel();
            double vdotr = 0.0;
            for(int idim = 0; idim < N; idim++) {
              vdotr += vel[idim] * line_of_sight_direction[idim];
            }
            for(int idim = 0; idim < N; idim++) {
              pos[idim] +=  vdotr * line_of_sight_direction[idim] * velocity_to_displacement;
              // Periodic boundary conditions
              if(pos[idim] <  0.0) pos[idim] += 1.0;
              if(pos[idim] >= 1.0) pos[idim] -= 1.0;
            }
          }
          // Only displacements along the x-axis can trigger communication needs so we can avoid one call
          // if(dir == 0) part.communicate_particles();
          part.communicate_particles();

          // Bin particles to grid
          FFTWGrid<N> density_k(Ngrid, nleft, nright);
          density_k.add_memory_label("FFTWGrid::compute_power_spectrum_multipoles::density_k");
          density_k.set_grid_status_real(true);
          particles_to_grid<N,T>(
              part.get_particles_ptr(),
              part.get_npart(),
              part.get_npart_total(),
              density_k,
              density_assignment_method);

          // Displace particles XXX add OMP
          for(auto &p : part){
            auto *pos = p.get_pos();
            auto *vel = p.get_vel();
            double vdotr = 0.0;
            for(int idim = 0; idim < N; idim++) {
              vdotr += vel[idim] * line_of_sight_direction[idim];
            }
            for(int idim = 0; idim < N; idim++) {
              pos[idim] -= vdotr * line_of_sight_direction[idim] * velocity_to_displacement;
              // Periodic boundary conditions
              if(pos[idim] <  0.0) pos[idim] += 1.0;
              if(pos[idim] >= 1.0) pos[idim] -= 1.0;
            }
          }
          // Only displacements along the x-axis can trigger communication needs so we can avoid one call
          // if(dir == 0) part.communicate_particles();
          part.communicate_particles();

          // Fourier transform
          density_k.fftw_r2c();

          // Deconvolve window function
          deconvolve_window_function_fourier<N>(density_k,  density_assignment_method);

          // Compute power-spectrum multipoles
          compute_power_spectrum_multipoles(
              density_k,
              Pell_current,
              line_of_sight_direction);

          // Assign back
          Pell_all[dir] = Pell_current;
        }

        // Normalize
        for (size_t ell = 0; ell < Pell.size(); ell++){
          for(int dir = 0; dir < N; dir++) {
            Pell[ell] += Pell_all[dir][ell];
          }
        }
        for (size_t ell = 0; ell < Pell.size(); ell++){
          for(int i = 0; i < Pell[ell].n; i++){
            Pell[ell].pofk[i] /= double(N);
            Pell[ell].count[i] /= double(N);
            Pell[ell].kbin[i] /= double(N);
          }
        }

        // XXX Compute variance of pofk

        // Subtract shotnoise for monopole
        for(int i = 0; i < Pell[0].n; i++){
          Pell[0].pofk[i] -= 1.0/double(part.get_npart_total());
        }
      }

    //================================================================================
    // A simple power-spectrum estimator - nothing fancy
    // Deconvolving the window-function and subtracting shot-noise (1/NumPartTotal)
    //================================================================================
    template<int N, class T>
      void compute_power_spectrum(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          PowerSpectrumBinning<N> &pofk,
          std::string density_assignment_method) {

        // Set how many extra slices we need for the density assignment to go smoothly
        auto nleftright = get_extra_slices_needed_for_density_assignment(density_assignment_method);
        int nleft  = nleftright.first;
        int nright = nleftright.second;

        // Bin particles to grid
        FFTWGrid<N> density_k(Ngrid, nleft, nright);
        density_k.add_memory_label("FFTWGrid::compute_power_spectrum::density_k");
        particles_to_grid<N,T>(
            part,
            NumPart,
            NumPartTotal,
            density_k,
            density_assignment_method);

        // Fourier transform
        density_k.fftw_r2c();

        // Deconvolve window function
        deconvolve_window_function_fourier<N>(density_k,  density_assignment_method);

        // Bin up power-spectrum
        bin_up_power_spectrum<N>(density_k, pofk);

        // Subtract shotnoise
        for(int i = 0; i < pofk.n; i++){
          pofk.pofk[i] -= 1.0/double(NumPartTotal);
        }
      }

    //======================================================================
    // Computes the power-spectum by using two interlaced grids
    // to reduce the effect of aliasing (allowing us to use a smaller Ngrid)
    // Deconvolves the window function and subtracts shot-noise
    //======================================================================
    template<int N, class T>
      void compute_power_spectrum_interlacing(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          PowerSpectrumBinning<N> &pofk,
          std::string density_assignment_method) {

        // Set how many extra slices we need for the density assignment to go smoothly
        auto nleftright = get_extra_slices_needed_for_density_assignment(density_assignment_method);
        int nleft  = nleftright.first;
        int nright = nleftright.second;

        // One extra slice in general as we need to shift the particle half a grid-cell
        nright += 1; 

        // Bin particles to grid
        FFTWGrid<N> density_k(Ngrid, nleft, nright);
        density_k.add_memory_label("FFTWGrid::compute_power_spectrum_interlacing::density_k");
        particles_to_grid<N,T>(
            part,
            NumPart,
            NumPartTotal,
            density_k,
            density_assignment_method);

        // Shift particles
        const double shift = 1.0/double(2*Ngrid);
#ifdef USE_OMP
#pragma omp parallel for 
#endif
        for(size_t i = 0; i < NumPart; i++){
          auto *pos = part[i].get_pos();
          pos[0] += shift;
          for(int idim = 1; idim < N; idim++){
            pos[idim] += shift;
            if(pos[idim] >= 1.0) pos[idim] -= 1.0;
          }
        }

        // Bin shifted particles to grid
        FFTWGrid<N> density_k2(Ngrid, nleft, nright);
        density_k2.add_memory_label("FFTWGrid::compute_power_spectrum_interlacing::density_k2");
        particles_to_grid<N,T>(
            part,
            NumPart,
            NumPartTotal,
            density_k2,
            density_assignment_method);

        // Shift particles back as not to ruin anything
#ifdef USE_OMP
#pragma omp parallel for 
#endif
        for(size_t i = 0; i < NumPart; i++){
          auto *pos = part[i].get_pos();
          pos[0] -= shift;
          for(int idim = 1; idim < N; idim++){
            pos[idim] -= shift;
            if(pos[idim] < 0.0) pos[idim] += 1.0;
          }
        }

        // Fourier transform
        density_k.fftw_r2c();
        density_k2.fftw_r2c();

        // The mean of the two grids
        auto ff = density_k.get_fourier_grid();
        auto gg = density_k2.get_fourier_grid();
        const std::complex<double> I(0,1);
        for(auto && complex_index: density_k.get_fourier_range()) {
          auto kvec = density_k.get_fourier_wavevector_from_index(complex_index);
          double ksum = 0.0;
          for(int idim = 0; idim < N; idim++)
            ksum += kvec[idim];
          auto norm = std::exp( I * ksum * shift);
          ff[complex_index] = (ff[complex_index] + norm * gg[complex_index])/2.0;
        }

        // Deconvolve window function
        deconvolve_window_function_fourier<N>(density_k,  density_assignment_method);

        // Bin up power-spectrum
        bin_up_power_spectrum<N>(density_k, pofk);

        // Subtract shotnoise
        for(int i = 0; i < pofk.n; i++){
          pofk.pofk[i] -= 1.0/double(NumPartTotal);
        }
      }

    // https://arxiv.org/pdf/1506.02729.pdf
    // The general quadrupole estimator Eq. 20
    // P(k) = <delta0(k)delta2^*(k>>
    // We compute delta0 and delta2
    //template<int N>
    //  void quadrupole_estimator_3D(FFTWGrid<N> & density_real)
    //  {

    //    FFTWGrid<N> Q_xx, Q_yy, Q_zz, Q_xy, Q_yz, Q_zx;

    //    compute_multipole_Q_term(density_real, Q_xx, {0,0}, origin);
    //    compute_multipole_Q_term(density_real, Q_xy, {0,1}, origin);
    //    compute_multipole_Q_term(density_real, Q_yy, {1,1}, origin);
    //    Q_xx.fftw_r2c();
    //    Q_xy.fftw_r2c();
    //    Q_yy.fftw_r2c();
    //    if constexpr (N > 2){
    //      compute_multipole_Q_term(density_real, Q_yz, {1,2}, origin);
    //      compute_multipole_Q_term(density_real, Q_zx, {2,0}, origin);
    //      compute_multipole_Q_term(density_real, Q_zz, {2,2}, origin);
    //      Q_yz.fftw_r2c();
    //      Q_zx.fftw_r2c();
    //      Q_zz.fftw_r2c();
    //    }

    //    // Density to fourier space
    //    density_real.fftw_r2c();

    //    FFTWGrid<N> delta2 =  density_real;

    //    double kmag;
    //    std::vector<double> kvec(N);
    //    for(auto & fourier_index : density_real.get_fourier_range()){
    //      density_real.get_fourier_wavevector_and_norm_by_index(fourier_index, kvec, kmag2);

    //      // Unit kvector
    //      if(kmag > 0.0)
    //        for(auto & k : kvec)
    //          k /= kmag;

    //      auto Q_xx_of_k = Q_xx.get_fourier_from_index(fourier_index);
    //      auto Q_xy_of_k = Q_xy.get_fourier_from_index(fourier_index);
    //      auto Q_yy_of_k = Q_yy.get_fourier_from_index(fourier_index);

    //      auto Q_yz_of_k = Q_yz.get_fourier_from_index(fourier_index);
    //      auto Q_zx_of_k = Q_zx.get_fourier_from_index(fourier_index);
    //      auto Q_zz_of_k = Q_zz.get_fourier_from_index(fourier_index);
    //      auto delta0    = density_real.get_fourier_from_index(fourier_index);

    //      auto res = Q_xx_of_k * kvec[0] * kvec[0] + Q_yy_of_k * kvec[1] * kvec[1] +  2.0 * (Q_xy_of_k * kvec[0] * kvec[1])
    //        if constexpr (N > 2) res += Q_zz_of_k * kvec[2] * kvec[2] + 2.0 * (Q_yz_of_k * kvec[1] * kvec[2] + Q_zx_of_k * kvec[2] * kvec[0]);
    //      res = 1.5 * res - 0.5 * delta0;

    //      delta2.set_real_from_fourier(fourier_index, res);
    //    }
    //  }

    // Compute delta(x) -> delta(x) * xi * xj * xk * .... (i,j,k,... in Qindex)
    // where xi's being the i'th component of the unit norm line of sight direction
    // For the quadrupole we put Qindex = {ii,jj} and we need the 6 combinations xx,yy,zz,xy,yz,zx
    // For the hexadecapole we need {ii,jj,kk,ll} for 15 different combinations
    // xxxx + cyc (3), xxxy + cyc, xxyy + cyc and xxyz + cyc
    template<int N>
      void compute_multipole_Q_term(
          FFTWGrid<N> & density_real,
          FFTWGrid<N> & Q_real,
          std::vector<int> Qindex,
          std::vector<double> origin)
      {
        assert(Qindex.size() > 0);
        assert(origin.size() == N);

        for(auto & real_index : density_real.get_real_range()){
          auto coord = density_real.get_coord_from_index(real_index);
          auto pos = density_real.get_real_position(coord);

          double norm = 0.0;
          for(int idim = 0; idim < N; idim++){
            pos[idim] -= origin[idim];
            norm += pos[idim]*pos[idim];
          }
          norm = std::sqrt(norm);
          for(int idim = 0; idim < N; idim++){
            pos[idim] /= norm;
          }
          auto value = density_real.get_real_from_index(real_index);
          for(auto ii: Qindex)
            value *= pos[ii];

          Q_real.set_real_from_index(real_index, value);
        }
      }


    // Computes the bispectrum B(k1,k2,k3) for *all* k1,k2,k3
    // One can make this much faster if one *just* wants say k1=k2=k3
    template<int N, class T>
      void compute_bispectrum(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          BispectrumBinning<N> &bofk,
          std::string density_assignment_method) {

        // Set how many extra slices we need for the density assignment to go smoothly
        auto nleftright = get_extra_slices_needed_for_density_assignment(density_assignment_method);
        int nleft  = nleftright.first;
        int nright = nleftright.second;

        // Bin particles to grid
        FFTWGrid<N> density_k(Ngrid, nleft, nright);
        density_k.add_memory_label("FFTWGrid::compute_bispectrum::density_k");
        particles_to_grid<N,T>(
            part,
            NumPart,
            NumPartTotal,
            density_k,
            density_assignment_method);

        // To fourier space
        density_k.fftw_r2c();

        // Deconvolve window function
        deconvolve_window_function_fourier<N>(density_k,  density_assignment_method);

        // Compute bispectrum
        compute_bispectrum<N>(density_k, bofk);

      }

    // Computes the polyspectrum P(k1,k2,k3,...) 
    template<int N, class T, int ORDER>
      void compute_polyspectrum(
          int Ngrid,
          T *part,
          size_t NumPart,
          size_t NumPartTotal,
          PolyspectrumBinning<N,ORDER> & polyofk,
          std::string density_assignment_method) {

        // Set how many extra slices we need for the density assignment to go smoothly
        auto nleftright = get_extra_slices_needed_for_density_assignment(density_assignment_method);
        int nleft  = nleftright.first;
        int nright = nleftright.second;

        // Bin particles to grid
        FFTWGrid<N> density_k(Ngrid, nleft, nright);
        density_k.add_memory_label("FFTWGrid::compute_bispectrum::density_k");
        particles_to_grid<N,T>(
            part,
            NumPart,
            NumPartTotal,
            density_k,
            density_assignment_method);

        // To fourier space
        density_k.fftw_r2c();

        // Deconvolve window function
        deconvolve_window_function_fourier<N>(density_k,  density_assignment_method);

        // Compute bispectrum
        compute_polyspectrum<N,ORDER>(density_k, polyofk);
      }

    template<int N>
      void compute_bispectrum(
          FFTWGrid<N> & density_k,
          BispectrumBinning<N> &bofk) {

        const int Nmesh = density_k.get_nmesh();
        const int nbins = bofk.n;
        assert(nbins > 0);
        assert(Nmesh > 0);

        // Now loop over bins and do FFTs
        std::vector< FFTWGrid<N> > F_k(nbins);
        std::vector< FFTWGrid<N> > N_k(nbins);

        // Set ranges for which we will compute F_k
        std::vector<double> khigh(nbins);
        std::vector<double> k_bin(nbins);
        std::vector<double> klow(nbins);
        double deltak = (bofk.k[1] - bofk.k[0]);
        for(int i = 0; i < nbins; i++){
          F_k[i] = density_k;
          N_k[i] = density_k;
          N_k[i].fill_fourier_grid(0.0);

          if(i == 0){
            klow[i]  = bofk.k[0];
            khigh[i] = bofk.k[0] + (bofk.k[1] - bofk.k[0])/2.0;
          } else if(i < nbins-1){
            klow[i]  = khigh[i-1];
            khigh[i] = bofk.k[i] + (bofk.k[i+1] - bofk.k[i])/2.0;
          } else {
            klow[i] = khigh[i-1];
            khigh[i] = bofk.k[nbins-1];
          }
          k_bin[i] = (khigh[i]+klow[i])/2.0;
        }

        // Compute how many configurations we have to store
        // This is (n+ORDER choose ORDER)
        //nbins_tot = 1;
        //int faculty = 1;
        //for(int i = 0; i < order-1; i++){
        //  nbins_tot *= (nbins+i);
        //  faculty *= (1+i);
        //}
        //nbins_tot /= faculty;
        //We just store all the symmmetry configurations as written

        // Set up results vector
        size_t nbins_tot = FML::power(size_t(nbins), 3);
        std::vector<double> N123(nbins_tot,0.0);
        std::vector<double> B123(nbins_tot,0.0);
        std::vector<double> kmean_bin(nbins,0.0);
        std::vector<double> pofk_bin(nbins,0.0);

        for(int i = 0; i < nbins; i++){
#ifdef DEBUG_BISPECTRUM
          if(FML::ThisTask == 0)
            std::cout << "Computing bispectrum " << i+1 << " / " << nbins << " kbin: " << klow[i] / (2.0*M_PI) << " -> " << khigh[i] / (2.0*M_PI) << "\n";;
#endif

          FFTWGrid<N> & grid = F_k[i];
          FFTWGrid<N> & count_grid = N_k[i];

          // For each bin get klow, khigh and deltak
          const double kmag_max  = khigh[i];
          const double kmag_min  = klow[i];
          const double kmag2_max = kmag_max*kmag_max;
          const double kmag2_min = kmag_min*kmag_min;

          //=====================================================
          // Bin weights, currently not in use. Basically some averaging about 
          // k for each bin. Not tested so well, but gives very good results for local
          // nn-gaussianity (much less noisy estimates), but not so much in general
          //=====================================================
          //double sigma2 = 8.0 * std::pow( deltak ,2);
          //auto weight = [&](double kmag, double kbin){
          //  //return std::fabs(kmag - kbin) < deltak/2 ? 1.0 : 0.0;
          //  return 1.0/std::sqrt(2.0 * M_PI * sigma2) * std::exp( -0.5*(kmag - kbin)*(kmag - kbin)/sigma2 );
          //};

          // Loop over all cells
          double kmean = 0.0;
          double nk = 0;
          double kmag2;
          std::vector<double> kvec(N);
          for(auto & fourier_index : grid.get_fourier_range()){
            grid.get_fourier_wavevector_and_norm2_by_index(fourier_index, kvec, kmag2);

            //=====================================================
            // Set to zero outside the bin (N_k already init to 0)
            //=====================================================
            if(kmag2 >= kmag2_max or kmag2 < kmag2_min){
              grid.set_fourier_from_index(fourier_index, 0.0);
              count_grid.set_fourier_from_index(fourier_index, 0.0);
            } else {
              // Compute mean k in the bin
              kmean += std::sqrt(kmag2);
              pofk_bin[i] += std::norm(grid.get_fourier_from_index(fourier_index));
              nk += 1.0;
              count_grid.set_fourier_from_index(fourier_index, 1.0);
            }

            //=====================================================
            // Alternative to the above: add with weights
            //=====================================================
            //double kmag = std::sqrt(kmag2);
            //double fac = weight(kmag, k_bin[i]);
            //kmean += kmag * fac;
            //pofk_bin[i] += std::norm(grid.get_fourier_from_index(fourier_index)) * fac;
            //nk += fac;
            //grid.set_fourier_from_index(fourier_index, grid.get_fourier_from_index(fourier_index) * fac);
            //count_grid.set_fourier_from_index(fourier_index, fac);
          }
#ifdef USE_MPI
          MPI_Allreduce(MPI_IN_PLACE,&kmean,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
          MPI_Allreduce(MPI_IN_PLACE,&pofk_bin[i],1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
          MPI_Allreduce(MPI_IN_PLACE,&nk,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
#endif
          // The mean k in the bin
          kmean_bin[i] = (nk == 0) ? k_bin[i] : kmean / double(nk);
          // Power spectrum in the bin
          pofk_bin[i] = (nk == 0) ? 0.0 : pofk_bin[i] / double(nk);

#ifdef DEBUG_BISPECTRUM
          if(FML::ThisTask == 0)
            std::cout << "kmean: " << kmean_bin[i] / (2.0*M_PI) << "\n";
#endif

          // Transform to real space
          grid.fftw_c2r();
          count_grid.fftw_c2r();
        }

        // We now have F_k and N_k for all bins
        for(size_t i = 0; i < nbins_tot; i++){
#ifdef DEBUG_BISPECTRUM
          if(FML::ThisTask == 0)
            if( (i*10)/nbins_tot != ((i+1)*10)/nbins_tot )
              std::cout << "Integrating up " << 100*(i+1)/nbins_tot << " %\n";;
#endif

          // Current values of k1,k2,k3
          int ik[3];
          for(int ii = 0, n = 1; ii < 3; ii++, n *= nbins){
            ik[ii] = i / n % nbins;
          }
          // Only compute stuff for k1 <= k2 <= k3
          if(ik[0] > ik[1] or ik[1] > ik[2]) {
            continue;
          }

          // No valid triangles if k1+k2 < k3 so just set too zero right away
          if(k_bin[ik[0]] + k_bin[ik[1]] < k_bin[ik[2]] - 3*deltak/2.){
            N123[i] = 0.0;
            B123[i] = 0.0;
            continue;
          }

          // Compute number of triangles in current bin (norm insignificant below)
          double N123_current = 0.0;
          for(auto & real_index : N_k[0].get_real_range()){
            double N1 = N_k[ik[0]].get_real_from_index(real_index);
            double N2 = N_k[ik[1]].get_real_from_index(real_index);
            double N3 = N_k[ik[2]].get_real_from_index(real_index);
            N123_current += N1 * N2 * N3;
          }
#ifdef USE_MPI
          MPI_Allreduce(MPI_IN_PLACE,&N123_current,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
#endif

          // Compute sum over triangles
          double F123_current = 0.0;
          for(auto & real_index : F_k[0].get_real_range()){
            auto F1 = F_k[ik[0]].get_real_from_index(real_index);
            auto F2 = F_k[ik[1]].get_real_from_index(real_index);
            auto F3 = F_k[ik[2]].get_real_from_index(real_index);
            F123_current += F1 * F2 * F3;
          }
#ifdef USE_MPI
          MPI_Allreduce(MPI_IN_PLACE,&F123_current,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
#endif
          // Normalize dx^N / (2pi)^N
          F123_current *= std::pow(1.0 / double(Nmesh) / (2.0 * M_PI), N);
          N123_current *= std::pow(1.0 / double(Nmesh) / (2.0 * M_PI), N);

          // Set the result
          B123[i] = (N123_current > 0.0) ? F123_current / N123_current : 0.0;
          N123[i] = (N123_current > 0.0) ? N123_current : 0.0;

          // Return the reduced bispectrum
          double norm = pofk_bin[ik[0]] * pofk_bin[ik[1]] + pofk_bin[ik[1]] * pofk_bin[ik[2]] + pofk_bin[ik[2]] * pofk_bin[ik[0]];
          if(norm > 0.0) B123[i] /= norm;
        }

        // Set the values we didn't compute by using symmetry
        for(int i = 0; i < nbins; i++){
          for(int j = 0; j < nbins; j++){
            for(int k = 0; k < nbins; k++){
              size_t index = (i*nbins + j)*nbins + k;

              std::vector<int> inds{i,j,k};
              std::sort(inds.begin(), inds.end(), std::less<int>());

              size_t index0 = (inds[0]*nbins+inds[1])*nbins+inds[2];
              size_t index1 = (inds[0]*nbins+inds[2])*nbins+inds[1];
              size_t index2 = (inds[1]*nbins+inds[2])*nbins+inds[0];
              size_t index3 = (inds[1]*nbins+inds[0])*nbins+inds[2];
              size_t index4 = (inds[2]*nbins+inds[0])*nbins+inds[1];
              size_t index5 = (inds[2]*nbins+inds[1])*nbins+inds[0];

              if(B123[index0] == 0.0) B123[index0] = B123[index];
              if(B123[index1] == 0.0) B123[index1] = B123[index];
              if(B123[index2] == 0.0) B123[index2] = B123[index];
              if(B123[index3] == 0.0) B123[index3] = B123[index];
              if(B123[index4] == 0.0) B123[index4] = B123[index];
              if(B123[index5] == 0.0) B123[index5] = B123[index];
            }
          }
        }

        // Copy results over
        bofk.B123 = B123;
        bofk.N123 = N123;
        bofk.kbin = k_bin;
        bofk.pofk = pofk_bin;

      }

    template<int N, int order>
      void compute_polyspectrum(
          FFTWGrid<N> & density_k,
          PolyspectrumBinning<N,order> &polyofk) {

        const int Nmesh = density_k.get_nmesh();
        const int nbins = polyofk.n;
        assert(nbins > 0);
        assert(Nmesh > 0);
        static_assert(order > 1);

        // Now loop over bins and do FFTs
        std::vector< FFTWGrid<N> > F_k(nbins);
        std::vector< FFTWGrid<N> > N_k(nbins);

        // Set ranges for which we will compute F_k
        std::vector<double> khigh(nbins);
        std::vector<double> k_bin(nbins);
        std::vector<double> klow(nbins);
        double deltak = (polyofk.k[1] - polyofk.k[0]);
        for(int i = 0; i < nbins; i++){
          F_k[i] = density_k;
          N_k[i] = density_k;
          N_k[i].fill_fourier_grid(0.0);

          if(i == 0){
            klow[i]  = polyofk.k[0];
            khigh[i] = polyofk.k[0] + (polyofk.k[1] - polyofk.k[0])/2.0;
          } else if(i < nbins-1){
            klow[i]  = khigh[i-1];
            khigh[i] = polyofk.k[i] + (polyofk.k[i+1] - polyofk.k[i])/2.0;
          } else {
            klow[i] = khigh[i-1];
            khigh[i] = polyofk.k[nbins-1];
          }
          k_bin[i] = (khigh[i]+klow[i])/2.0;
        }

        // Set up results vector
        size_t nbins_tot = FML::power(size_t(nbins),order);
        std::vector<double> P123(nbins_tot,0.0);
        std::vector<double> N123(nbins_tot,0.0);
        std::vector<double> kmean_bin(nbins,0.0);
        std::vector<double> pofk_bin(nbins,0.0);

        for(int i = 0; i < nbins; i++){
#ifdef DEBUG_BISPECTRUM
          if(FML::ThisTask == 0)
            std::cout << "Computing polyspectrum " << i+1 << " / " << nbins << " kbin: " << klow[i] / (2.0*M_PI) << " -> " << khigh[i] / (2.0*M_PI) << "\n";;
#endif

          FFTWGrid<N> & grid = F_k[i];
          FFTWGrid<N> & count_grid = N_k[i];

          // For each bin get klow, khigh and deltak
          const double kmag_max  = khigh[i];
          const double kmag_min  = klow[i];
          const double kmag2_max = kmag_max*kmag_max;
          const double kmag2_min = kmag_min*kmag_min;

          //=====================================================
          // Bin weights, currently not in use. Basically some averaging about 
          // k for each bin. Not tested so well, but gives very good results for local
          // nn-gaussianity (much less noisy estimates), but not so much in general
          //=====================================================
          //double sigma2 = 8.0 * std::pow( deltak ,2);
          //auto weight = [&](double kmag, double kbin){
          //  //return std::fabs(kmag - kbin) < deltak/2 ? 1.0 : 0.0;
          //  return 1.0/std::sqrt(2.0 * M_PI * sigma2) * std::exp( -0.5*(kmag - kbin)*(kmag - kbin)/sigma2 );
          //};

          // Loop over all cells
          double kmean = 0.0;
          double nk = 0;
          double kmag2;
          std::vector<double> kvec(N);
          for(auto & fourier_index : grid.get_fourier_range()){
            grid.get_fourier_wavevector_and_norm2_by_index(fourier_index, kvec, kmag2);

            //=====================================================
            // Set to zero outside the bin (N_k already init to 0)
            //=====================================================
            if(kmag2 >= kmag2_max or kmag2 < kmag2_min){
              grid.set_fourier_from_index(fourier_index, 0.0);
              count_grid.set_fourier_from_index(fourier_index, 0.0);
            } else {
              // Compute mean k in the bin
              kmean += std::sqrt(kmag2);
              pofk_bin[i] += std::norm(grid.get_fourier_from_index(fourier_index));
              nk += 1.0;
              count_grid.set_fourier_from_index(fourier_index, 1.0);
            }

            //=====================================================
            // Alternative to the above: add with weights
            //=====================================================
            //double kmag = std::sqrt(kmag2);
            //double fac = weight(kmag, k_bin[i]);
            //kmean += kmag * fac;
            //pofk_bin[i] += std::norm(grid.get_fourier_from_index(fourier_index)) * fac;
            //nk += fac;
            //grid.set_fourier_from_index(fourier_index, grid.get_fourier_from_index(fourier_index) * fac);
            //count_grid.set_fourier_from_index(fourier_index, fac);
          }
#ifdef USE_MPI
          MPI_Allreduce(MPI_IN_PLACE,&kmean,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
          MPI_Allreduce(MPI_IN_PLACE,&pofk_bin[i],1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
          MPI_Allreduce(MPI_IN_PLACE,&nk,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
#endif
          // The mean k in the bin
          kmean_bin[i] = (nk == 0) ? k_bin[i] : kmean / double(nk);
          // Power spectrum in the bin
          pofk_bin[i] = (nk == 0) ? 0.0 : pofk_bin[i] / double(nk);

#ifdef DEBUG_BISPECTRUM
          if(FML::ThisTask == 0)
            std::cout << "kmean: " << kmean_bin[i] / (2.0*M_PI) << "\n";
#endif

          // Transform to real space
          grid.fftw_c2r();
          count_grid.fftw_c2r();
        }

        // We now have F_k and N_k for all bins
        for(int i = 0; i < nbins_tot; i++){
#ifdef DEBUG_BISPECTRUM
          if(FML::ThisTask == 0)
            if( (i*10)/nbins_tot != ((i+1)*10)/nbins_tot )
              std::cout << "Integrating up " << 100*(i+1)/nbins_tot << " %\n";;
#endif

          // Current values of ik1,ik2,ik3,...
          int ik[order];
          for(int ii = order-1, n = 1; ii >= 0; ii--, n *= nbins){
            ik[ii] = i / n % nbins;
          }

          // Symmetry: only do ik1 <= ik2 <= ...
          bool valid = true;
          double ksum = 0.0;
          for(int ii = 1; ii < order; ii++){
            if(ik[ii] > ik[ii-1]) valid = false;
            ksum += k_bin[ik[ii-1]];
          }
          if(!valid) continue;

          // No valid 'triangles' if k1+k2+... < kN so just set too zero right away
          if(ksum < k_bin[ik[order-1]] - order*deltak/2.){
            N123[i] = 0.0;
            P123[i] = 0.0;
            continue;
          }

          // Compute number of triangles in current bin (norm insignificant below)
          double N123_current = 0.0;
          for(auto & real_index : N_k[0].get_real_range()){
            double Nproduct = 1.0;
            for(int ii = 0; ii < order; ii++)
              Nproduct *= N_k[ik[ii]].get_real_from_index(real_index);
            N123_current += Nproduct;
          }
#ifdef USE_MPI
          MPI_Allreduce(MPI_IN_PLACE,&N123_current,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
#endif

          // Compute sum over triangles
          double F123_current = 0.0;
          for(auto & real_index : F_k[0].get_real_range()){
            double Fproduct = 1.0;
            for(int ii = 0; ii < order; ii++)
              Fproduct *= F_k[ik[ii]].get_real_from_index(real_index);
            F123_current += Fproduct;
          }
#ifdef USE_MPI
          MPI_Allreduce(MPI_IN_PLACE,&F123_current,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
#endif
          // Normalize dx^N / (2pi)^N
          F123_current *= std::pow(1.0 / double(Nmesh) / (2.0 * M_PI), N);
          N123_current *= std::pow(1.0 / double(Nmesh) / (2.0 * M_PI), N);

          // Set the result
          P123[i] = (N123_current > 0.0) ? F123_current / N123_current : 0.0;
          N123[i] = (N123_current > 0.0) ? N123_current : 0.0;
        }

        // Set stuff not computed
        for(int i = 0; i < nbins_tot; i++){

          // Current values of k1,k2,k3
          std::vector<int> ik(order);
          for(int ii = 0, n = 1; ii < order; ii++, n *= nbins){
            ik[ii] = i / n % nbins;
          }

          // Symmetry: only do ik1 <= ik2 <= ...
          bool valid = true;
          for(int ii = 1; ii < order; ii++){
            if(ik[ii] > ik[ii-1]) valid = false;
          }
          if(!valid) continue;

          // Find a cell given by symmetry that we have computed
          // by sorting ik in increasing order
          std::sort(ik.begin(), ik.end(), std::less<int>());

          // Compute cell index
          size_t index = 0;
          for(int ii = 0; ii < order; ii++)
            index = index * nbins + ik[ii];

          // Set value (if it has not been set before just in case we fucked up)
          if(P123[index] == 0.0) P123[index] = P123[i];
          if(N123[index] == 0.0) N123[index] = N123[i];
        }

        // Copy results over
        polyofk.P123 = P123;
        polyofk.N123 = N123;
        polyofk.kbin = k_bin;
        polyofk.pofk = pofk_bin;

      }
  }
}

#endif
