/**
 * GauXC Copyright (c) 2020-2023, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of
 * any required approvals from the U.S. Dept. of Energy). All rights reserved.
 *
 * See LICENSE.txt for details
 */
#pragma once

#include "reference_replicated_xc_host_integrator.hpp"
#include "integrator_util/integrator_common.hpp"
#include "host/local_host_work_driver.hpp"
#include <stdexcept>

namespace GauXC  {
namespace detail {

template <typename ValueType>
void ReferenceReplicatedXCHostIntegrator<ValueType>::
  eval_exc_vxc_( int64_t m, int64_t n, const value_type* P,
                 int64_t ldp, value_type* VXC, int64_t ldvxc,
                 value_type* EXC ) {

  const auto& basis = this->load_balancer_->basis();

  // Check that P / VXC are sane
  const int64_t nbf = basis.nbf();
  if( m != n ) 
    GAUXC_GENERIC_EXCEPTION("P/VXC Must Be Square");
  if( m != nbf ) 
    GAUXC_GENERIC_EXCEPTION("P/VXC Must Have Same Dimension as Basis");
  if( ldp < nbf )
    GAUXC_GENERIC_EXCEPTION("Invalid LDP");
  if( ldvxc < nbf )
    GAUXC_GENERIC_EXCEPTION("Invalid LDVXC");


  // Get Tasks
  this->load_balancer_->get_tasks();

  // Temporary electron count to judge integrator accuracy
  value_type N_EL;

  // Compute Local contributions to EXC / VXC
  this->timer_.time_op("XCIntegrator.LocalWork", [&](){
    //exc_vxc_local_work_( P, ldp, VXC, ldvxc, EXC, &N_EL );
    exc_vxc_local_work_( P, ldp, nullptr, 0, VXC, ldvxc, nullptr, 0, EXC, &N_EL );
  });


  // Reduce Results
  this->timer_.time_op("XCIntegrator.Allreduce", [&](){

    if( not this->reduction_driver_->takes_host_memory() )
      GAUXC_GENERIC_EXCEPTION("This Module Only Works With Host Reductions");

    this->reduction_driver_->allreduce_inplace( VXC, nbf*nbf, ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( EXC,   1    , ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( &N_EL, 1    , ReductionOp::Sum );

  });

}

template <typename ValueType>
void ReferenceReplicatedXCHostIntegrator<ValueType>::
  eval_exc_vxc_( int64_t m, int64_t n, const value_type* Ps,
                      int64_t ldps,
                      const value_type* Pz,
                      int64_t ldpz,
                      value_type* VXCs, int64_t ldvxcs,
                      value_type* VXCz, int64_t ldvxcz,
                      value_type* EXC ) {

  const auto& basis = this->load_balancer_->basis();

  // Check that P / VXC are sane
  const int64_t nbf = basis.nbf();
  if( m != n )
    GAUXC_GENERIC_EXCEPTION("P/VXC Must Be Square");
  if( m != nbf )
    GAUXC_GENERIC_EXCEPTION("P/VXC Must Have Same Dimension as Basis");
  if( ldps < nbf )
    GAUXC_GENERIC_EXCEPTION("Invalid LDPSCALAR");
  if( ldpz < nbf )
    GAUXC_GENERIC_EXCEPTION("Invalid LDPZ");
  if( ldvxcs < nbf )
    GAUXC_GENERIC_EXCEPTION("Invalid LDVXCSCALAR");
  if( ldvxcz < nbf )
    GAUXC_GENERIC_EXCEPTION("Invalid LDVXCZ");

  // Get Tasks
  this->load_balancer_->get_tasks();

  // Temporary electron count to judge integrator accuracy
  value_type N_EL;

  // Compute Local contributions to EXC / VXC
  this->timer_.time_op("XCIntegrator.LocalWork", [&](){
    exc_vxc_local_work_( Ps, ldps, Pz, ldpz, VXCs, ldvxcs, VXCz, ldvxcz, EXC, &N_EL );
  });


  // Reduce Results
  this->timer_.time_op("XCIntegrator.Allreduce", [&](){

    if( not this->reduction_driver_->takes_host_memory() )
      GAUXC_GENERIC_EXCEPTION("This Module Only Works With Host Reductions");

    this->reduction_driver_->allreduce_inplace( VXCs, nbf*nbf, ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( VXCz, nbf*nbf, ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( EXC,   1    , ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( &N_EL, 1    , ReductionOp::Sum );

  });


}

template <typename ValueType>
void ReferenceReplicatedXCHostIntegrator<ValueType>::
  neo_eval_exc_vxc_( int64_t m1, int64_t n1, int64_t m2, int64_t n2, 
                     const value_type* P1s, int64_t ldp1s,
                     const value_type* P2s, int64_t ldp2s,
                     const value_type* P2z, int64_t ldp2z,
                     value_type* VXC1s, int64_t ldvxc1s,
                     value_type* VXC2s, int64_t ldvxc2s,
                     value_type* VXC2z, int64_t ldvxc2z,
                     value_type* EXC1,  value_type* EXC2 ){
  
  const auto& basis  = this->load_balancer_->basis();
  const auto& basis2 = this->load_balancer_->basis2();

  // Check that P / VXC are sane
  const int64_t nbf1 = basis.nbf();
  const int64_t nbf2 = basis2.nbf();

  if( m1 != n1 | m2 != n2)
    GAUXC_GENERIC_EXCEPTION("P/VXC Must Be Square");
  if( m1 != nbf1 | m2 != nbf2)
    GAUXC_GENERIC_EXCEPTION("P/VXC Must Have Same Dimension as Basis");
  if( ldp1s < nbf1 | ldp2s < nbf2 | ldp2z < nbf2 )
    GAUXC_GENERIC_EXCEPTION("Invalid LDP");
  if( ldvxc1s < nbf1 | ldvxc2s < nbf2 | ldvxc2z < nbf2 )
    GAUXC_GENERIC_EXCEPTION("Invalid LDVXC");

  // Get Tasks
  this->load_balancer_->get_tasks();

  // Temporary electron count to judge integrator accuracy
  value_type N_EL;

  // Compute Local contributions to EXC / VXC
  this->timer_.time_op("XCIntegrator.LocalWork", [&](){
    neo_exc_vxc_local_work_( P1s, ldp1s,
                             P2s, ldp2s,
                             P2z, ldp2z,
                             VXC1s, ldvxc1s,
                             VXC2s, ldvxc2s,
                             VXC2z, ldvxc2z,
                             EXC1, EXC2, &N_EL );
  });


  // Reduce Results
  this->timer_.time_op("XCIntegrator.Allreduce", [&](){

    if( not this->reduction_driver_->takes_host_memory() )
      GAUXC_GENERIC_EXCEPTION("This Module Only Works With Host Reductions");

    this->reduction_driver_->allreduce_inplace( VXC1s, nbf1*nbf1,  ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( VXC2s, nbf2*nbf1,  ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( VXC2z, nbf2*nbf2,  ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( EXC1,  1    ,  ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( EXC2,  1    ,  ReductionOp::Sum );
    this->reduction_driver_->allreduce_inplace( &N_EL, 1    ,  ReductionOp::Sum );

  });

}


template <typename ValueType>
void ReferenceReplicatedXCHostIntegrator<ValueType>::
  exc_vxc_local_work_( const value_type* Ps, int64_t ldps,
                       const value_type* Pz, int64_t ldpz,
                       value_type* VXCs, int64_t ldvxcs,
                       value_type* VXCz, int64_t ldvxcz, 
                       value_type* EXC, value_type *N_EL ) {


  const bool is_uks = (Pz != nullptr) and (VXCz != nullptr);
  const bool is_rks = not is_uks; // TODO: GKS

  // Cast LWD to LocalHostWorkDriver
  auto* lwd = dynamic_cast<LocalHostWorkDriver*>(this->local_work_driver_.get());

  // Setup Aliases
  const auto& func  = *this->func_;
  const auto& basis = this->load_balancer_->basis();
  const auto& mol   = this->load_balancer_->molecule();

  // Get basis map
  BasisSetMap basis_map(basis,mol);

  const int32_t nbf = basis.nbf();

  // Sort tasks on size (XXX: maybe doesnt matter?)
  auto task_comparator = []( const XCTask& a, const XCTask& b ) {
    return (a.points.size() * a.bfn_screening.nbe) > (b.points.size() * b.bfn_screening.nbe);
  };

  auto& tasks = this->load_balancer_->get_tasks();
  std::sort( tasks.begin(), tasks.end(), task_comparator );


  // Check that Partition Weights have been calculated
  auto& lb_state = this->load_balancer_->state();
  if( not lb_state.modified_weights_are_stored ) {
    GAUXC_GENERIC_EXCEPTION("Weights Have Not Beed Modified");
  }

  // Zero out integrands
  
  for( auto j = 0; j < nbf; ++j ) {
    for( auto i = 0; i < nbf; ++i ) {
      VXCs[i + j*ldvxcs] = 0.;
    }
  }
  if(is_uks) {
    for( auto j = 0; j < nbf; ++j ) {
      for( auto i = 0; i < nbf; ++i ) {
        VXCz[i + j*ldvxcz] = 0.;
      }
    }
  }
  *EXC = 0.;
 
    
  // Loop over tasks
  const size_t ntasks = tasks.size();

  #pragma omp parallel
  {

  XCHostData<value_type> host_data; // Thread local host data

  #pragma omp for schedule(dynamic)
  for( size_t iT = 0; iT < ntasks; ++iT ) {

    //std::cout << iT << "/" << ntasks << std::endl;
    // Alias current task
    const auto& task = tasks[iT];

    // Get tasks constants
    const int32_t  npts    = task.points.size();
    const int32_t  nbe     = task.bfn_screening.nbe;
    const int32_t  nshells = task.bfn_screening.shell_list.size();

    const auto* points      = task.points.data()->data();
    const auto* weights     = task.weights.data();
    const int32_t* shell_list = task.bfn_screening.shell_list.data();

    // Allocate enough memory for batch

    const size_t spin_dim_scal = is_rks ? 1 : 2; 
    // Things that every calc needs
    host_data.nbe_scr .resize(nbe  * nbe);
    host_data.zmat    .resize(npts * nbe * spin_dim_scal); 
    host_data.eps     .resize(npts);
    host_data.vrho    .resize(npts * spin_dim_scal);

    // LDA data requirements
    if( func.is_lda() ){
      host_data.basis_eval .resize( npts * nbe );
      host_data.den_scr    .resize( npts * spin_dim_scal);
    }

    // GGA data requirements
    const size_t gga_dim_scal = is_rks ? 1 : 3;
    if( func.is_gga() ){
      host_data.basis_eval .resize( 4 * npts * nbe );
      host_data.den_scr    .resize( spin_dim_scal * 4 * npts );
      host_data.gamma      .resize( gga_dim_scal * npts );
      host_data.vgamma     .resize( gga_dim_scal * npts );
    }

    // Alias/Partition out scratch memory
    auto* basis_eval = host_data.basis_eval.data();
    auto* den_eval   = host_data.den_scr.data();
    auto* nbe_scr    = host_data.nbe_scr.data();
    auto* zmat       = host_data.zmat.data();

    decltype(zmat) zmat_z = nullptr;
    if(!is_rks) {
      zmat_z = zmat + nbe * npts;
    }

    auto* eps        = host_data.eps.data();
    auto* gamma      = host_data.gamma.data();
    auto* vrho       = host_data.vrho.data();
    auto* vgamma     = host_data.vgamma.data();

    value_type* dbasis_x_eval = nullptr;
    value_type* dbasis_y_eval = nullptr;
    value_type* dbasis_z_eval = nullptr;
    value_type* dden_x_eval = nullptr;
    value_type* dden_y_eval = nullptr;
    value_type* dden_z_eval = nullptr;

    if( func.is_gga() ) {
      dbasis_x_eval = basis_eval    + npts * nbe;
      dbasis_y_eval = dbasis_x_eval + npts * nbe;
      dbasis_z_eval = dbasis_y_eval + npts * nbe;
      dden_x_eval   = den_eval    + spin_dim_scal * npts;
      dden_y_eval   = dden_x_eval + spin_dim_scal * npts;
      dden_z_eval   = dden_y_eval + spin_dim_scal * npts;
    }


    // Get the submatrix map for batch
    std::vector< std::array<int32_t, 3> > submat_map;
    std::tie(submat_map, std::ignore) =
          gen_compressed_submat_map(basis_map, task.bfn_screening.shell_list, nbf, nbf);

    // Evaluate Collocation (+ Grad)
    if( func.is_gga() )
      lwd->eval_collocation_gradient( npts, nshells, nbe, points, basis, shell_list,
        basis_eval, dbasis_x_eval, dbasis_y_eval, dbasis_z_eval );
    else
      lwd->eval_collocation( npts, nshells, nbe, points, basis, shell_list,
        basis_eval );


    // Evaluate X matrix (fac * P * B) -> store in Z
    const auto xmat_fac = is_rks ? 2.0 : 1.0; // TODO Fix for spinor RKS input
    lwd->eval_xmat( npts, nbf, nbe, submat_map, xmat_fac, Ps, ldps, basis_eval, nbe,
      zmat, nbe, nbe_scr );

    // X matrix for Pz
    if(not is_rks) {
      lwd->eval_xmat( npts, nbf, nbe, submat_map, 1.0, Pz, ldpz, basis_eval, nbe,
        zmat_z, nbe, nbe_scr);
    }


    // Evaluate U and V variables
    if( func.is_gga() ) {
      if(is_rks) {
        lwd->eval_uvvar_gga_rks( npts, nbe, basis_eval, dbasis_x_eval, dbasis_y_eval,
          dbasis_z_eval, zmat, nbe, den_eval, dden_x_eval, dden_y_eval, dden_z_eval,
          gamma );
      } else if(is_uks) {
        lwd->eval_uvvar_gga_uks( npts, nbe, basis_eval, dbasis_x_eval, dbasis_y_eval,
          dbasis_z_eval, zmat, nbe, zmat_z, nbe, den_eval, dden_x_eval, 
          dden_y_eval, dden_z_eval, gamma );
      }
     } else {
      if(is_rks) {
        lwd->eval_uvvar_lda_rks( npts, nbe, basis_eval, zmat, nbe, den_eval );
      } else if(is_uks) {
        lwd->eval_uvvar_lda_uks( npts, nbe, basis_eval, zmat, nbe, zmat_z, nbe,
          den_eval );
      }
     }
    
    // Evaluate XC functional
    if( func.is_gga() )
      func.eval_exc_vxc( npts, den_eval, gamma, eps, vrho, vgamma );
    else
      func.eval_exc_vxc( npts, den_eval, eps, vrho );

    // Factor weights into XC results
    for( int32_t i = 0; i < npts; ++i ) {
      eps[i]  *= weights[i];
      vrho[spin_dim_scal*i] *= weights[i];
      if(not is_rks) vrho[spin_dim_scal*i+1] *= weights[i];
    }

    if( func.is_gga() ){
      for( int32_t i = 0; i < npts; ++i ) {
         vgamma[gga_dim_scal*i] *= weights[i];
         if(not is_rks) {
           vgamma[gga_dim_scal*i+1] *= weights[i];
           vgamma[gga_dim_scal*i+2] *= weights[i];
         }
      }
    }



    // Evaluate Z matrix for VXC
    if( func.is_gga() ) {
      if(is_rks) {
        lwd->eval_zmat_gga_vxc_rks( npts, nbe, vrho, vgamma, basis_eval, dbasis_x_eval,
                                dbasis_y_eval, dbasis_z_eval, dden_x_eval, dden_y_eval,
                                dden_z_eval, zmat, nbe);
      } else if(is_uks) {
        lwd->eval_zmat_gga_vxc_uks( npts, nbe, vrho, vgamma, basis_eval, dbasis_x_eval,
                                dbasis_y_eval, dbasis_z_eval, dden_x_eval, dden_y_eval,
                                dden_z_eval, zmat, nbe, zmat_z, nbe);
      }
    } else {
      if(is_rks) {
        lwd->eval_zmat_lda_vxc_rks( npts, nbe, vrho, basis_eval, zmat, nbe );
      } else if(is_uks) {
        lwd->eval_zmat_lda_vxc_uks( npts, nbe, vrho, basis_eval, zmat, nbe, zmat_z, nbe );
      }
    }


    // Incremeta LT of VXC
    #pragma omp critical
    {
      // Scalar integrations
      for( int32_t i = 0; i < npts; ++i ) {
        const auto den = is_rks ? den_eval[i] : (den_eval[2*i] + den_eval[2*i+1]);
        *N_EL += weights[i] * den;
        *EXC  += eps[i]     * den;
      }

      // Increment VXC
      lwd->inc_vxc( npts, nbf, nbe, basis_eval, submat_map, zmat, nbe, VXCs, ldvxcs,
        nbe_scr );
      if(not is_rks) {
        lwd->inc_vxc( npts, nbf, nbe, basis_eval, submat_map, zmat_z, nbe, VXCz, ldvxcz,
          nbe_scr);
      }

    }

  } // Loop over tasks

  } // End OpenMP region

  // Symmetrize VXC
  for( int32_t j = 0;   j < nbf; ++j ) {
    for( int32_t i = j+1; i < nbf; ++i ) {
      VXCs[ j + i*ldvxcs ] = VXCs[ i + j*ldvxcs ];
    }
  }
  if(not is_rks) {
    for( int32_t j = 0;   j < nbf; ++j ) {
      for( int32_t i = j+1; i < nbf; ++i ) {
        VXCz[ j + i*ldvxcz ] = VXCz[ i + j*ldvxcz ];
      }
    }
  }

}


template <typename ValueType>
void ReferenceReplicatedXCHostIntegrator<ValueType>::
  neo_exc_vxc_local_work_( const value_type* P1s, int64_t ldp1s,
                            const value_type* P2s, int64_t ldp2s,
                            const value_type* P2z, int64_t ldp2z,
                            value_type* VXC1s, int64_t ldvxc1s,
                            value_type* VXC2s, int64_t ldvxc2s,
                            value_type* VXC2z, int64_t ldvxc2z,
                            value_type* EXC1, value_type* EXC2, value_type *N_EL ) {
  
  // Cast LWD to LocalHostWorkDriver
  auto* lwd = dynamic_cast<LocalHostWorkDriver*>(this->local_work_driver_.get());

  // Setup Aliases
  const auto& func   = *this->func_;
  const auto& basis  = this->load_balancer_->basis();
  const auto& mol    = this->load_balancer_->molecule();

  // Get basis map
  BasisSetMap basis_map(basis,mol);
  
  const int32_t nbf = basis.nbf();

  //NEO basis
  const auto& basis2 = this->load_balancer_->basis2();
  BasisSetMap basis_map2(basis2,mol);
  const int32_t nbf2 = basis2.nbf();

  // Sort tasks on size (XXX: maybe doesnt matter?)
  auto task_comparator = []( const XCTask& a, const XCTask& b ) {
    return (a.points.size() * a.bfn_screening.nbe) > (b.points.size() * b.bfn_screening.nbe);
  };
  
  auto& tasks = this->load_balancer_->get_tasks();
  std::sort( tasks.begin(), tasks.end(), task_comparator );


  // Check that Partition Weights have been calculated
  auto& lb_state = this->load_balancer_->state();
  if( not lb_state.modified_weights_are_stored )  GAUXC_GENERIC_EXCEPTION("Weights Have Not Beed Modified");
  

  // Zero out integrands
  
  std::fill(VXC1s, VXC1s + nbf * ldvxc1s, 0.0);
  std::fill(VXC2s, VXC2s + nbf2 * ldvxc2s, 0.0);
  std::fill(VXC2z, VXC2z + nbf2 * ldvxc2z, 0.0);

  *EXC1 = 0.;
  *EXC2 = 0.;
 
    
  // Loop over tasks
  const size_t ntasks = tasks.size();

  #pragma omp parallel
  {

  XCHostData<value_type> host_data; // Thread local host data

  #pragma omp for schedule(dynamic)
  for( size_t iT = 0; iT < ntasks; ++iT ) {

    //std::cout << iT << "/" << ntasks << std::endl;
    // Alias current task
    const auto& task = tasks[iT];

    // Get tasks constants
    const int32_t  npts     = task.points.size();
    const int32_t  nbe     = task.bfn_screening.nbe;
    const int32_t  nshells = task.bfn_screening.shell_list.size();

    const auto* points      = task.points.data()->data();
    const auto* weights     = task.weights.data();
    const int32_t* shell_list = task.bfn_screening.shell_list.data();

    // Allocate enough memory for batch

    // Things that every calc needs
    host_data.nbe_scr .resize( nbe * nbe );
    host_data.zmat    .resize( npts * nbe );
    host_data.eps     .resize( npts );
    host_data.vrho    .resize( npts );

    // LDA data requirements
    if( func.is_lda() ){
      host_data.basis_eval .resize( npts * nbe );
      host_data.den_scr    .resize( npts );
    }

    // GGA data requirements
    if( func.is_gga() ){
      host_data.basis_eval .resize( 4 * npts * nbe );
      host_data.den_scr    .resize( 4 * npts );
      host_data.gamma      .resize( npts );
      host_data.vgamma     .resize( npts );
    }

    // Alias/Partition out scratch memory
    auto* basis_eval = host_data.basis_eval.data();
    auto* den_eval   = host_data.den_scr.data();
    auto* nbe_scr    = host_data.nbe_scr.data();
    auto* zmat       = host_data.zmat.data();

    auto* eps        = host_data.eps.data();
    auto* gamma      = host_data.gamma.data();
    auto* vrho       = host_data.vrho.data();
    auto* vgamma     = host_data.vgamma.data();

    value_type* dbasis_x_eval = nullptr;
    value_type* dbasis_y_eval = nullptr;
    value_type* dbasis_z_eval = nullptr;
    value_type* dden_x_eval = nullptr;
    value_type* dden_y_eval = nullptr;
    value_type* dden_z_eval = nullptr;

    if( func.is_gga() ) {
      dbasis_x_eval = basis_eval    + npts * nbe;
      dbasis_y_eval = dbasis_x_eval + npts * nbe;
      dbasis_z_eval = dbasis_y_eval + npts * nbe;
      dden_x_eval   = den_eval    + npts;
      dden_y_eval   = dden_x_eval + npts;
      dden_z_eval   = dden_y_eval + npts;
    }

    //----------------------Start Protonic System Setup------------------------
    // Get shell info
    const int32_t  nbe2     = nbf2;
    const int32_t  nshells2 = basis2.nshells();
    std::vector<int32_t> shell_list2_vector; 
    shell_list2_vector.reserve(basis2.nshells());
    for(auto iSh = 0ul; iSh < basis2.size(); ++iSh) shell_list2_vector.emplace_back( iSh );
    const int32_t* shell_list2 = shell_list2_vector.data();

    // Set Up Memory
    host_data.nbe2_scr   .resize( nbe2 * nbe2 * 2);
    host_data.zmat2      .resize( npts * nbe2 * 2);
    host_data.eps2       .resize( npts );
    host_data.vrho2      .resize( npts * 2);
    // LDA
    host_data.basis2_eval .resize( npts * nbe2 );
    host_data.den2_scr    .resize( npts * 2);
    // No GGA for NEO yet
    // Alias/Partition out scratch memory
    auto* basis2_eval = host_data.basis2_eval.data();
    auto* den2_eval   = host_data.den2_scr.data();
    auto* nbe2_scr    = host_data.nbe2_scr.data();
    auto* zmat2       = host_data.zmat2.data();
    auto* eps2        = host_data.eps2.data();
    auto* vrho2       = host_data.vrho2.data();

    // No GGA for NEO yet
    value_type* dbasis2_x_eval = nullptr;
    value_type* dbasis2_y_eval = nullptr;
    value_type* dbasis2_z_eval = nullptr;
    value_type* dden2_x_eval   = nullptr;
    value_type* dden2_y_eval   = nullptr;
    value_type* dden2_z_eval   = nullptr;
    //----------------------End Protonic System Setup------------------------




    //----------------------Start Calculating Electronic Density & UV Variable------------------------
    // Get the submatrix map for batch
    std::vector< std::array<int32_t, 3> > submat_map;
    std::tie(submat_map, std::ignore) =
          gen_compressed_submat_map(basis_map, task.bfn_screening.shell_list, nbf, nbf);

    // Evaluate Collocation (+ Grad)
    if( func.is_gga() )
      lwd->eval_collocation_gradient( npts, nshells, nbe, points, basis, shell_list, 
        basis_eval, dbasis_x_eval, dbasis_y_eval, dbasis_z_eval );
    else
      lwd->eval_collocation( npts, nshells, nbe, points, basis, shell_list, 
        basis_eval );
    
    // Evaluate X matrix (P * B) -> store in Z
    lwd->eval_xmat( npts, nbf, nbe, submat_map, P1s, ldp1s, basis_eval, nbe,
      zmat, nbe, nbe_scr );

    // Evaluate U and V variables
    if( func.is_gga() )
      lwd->eval_uvvar_gga( npts, nbe, basis_eval, dbasis_x_eval, dbasis_y_eval,
        dbasis_z_eval, zmat, nbe, den_eval, dden_x_eval, dden_y_eval, dden_z_eval,
        gamma );
     else
      lwd->eval_uvvar_lda( npts, nbe, basis_eval, zmat, nbe, den_eval );

    // Evaluate XC functional
    if( func.is_gga() )
      func.eval_exc_vxc( npts, den_eval, gamma, eps, vrho, vgamma );
    else
      func.eval_exc_vxc( npts, den_eval, eps, vrho );
    //----------------------End Calculating Electronic Density & UV Variable------------------------




    //----------------------Start Calculating Protonic Density & UV Variable------------------------
    // Get the submatrix map for batch
    std::vector< std::array<int32_t, 3> > submat_map2;
    std::tie(submat_map2, std::ignore) =
          gen_compressed_submat_map(basis_map2, shell_list2_vector, nbf2, nbf2);

    // Evaluate Collocation 
    lwd->eval_collocation( npts, nshells2, nbe2, points, basis2, shell_list2,
      basis2_eval );

    // Evaluate X matrix (P * B) -> store in Z
    lwd->eval_xmat( npts, nbf2, nbe2, submat_map2, P2s, ldp2s, basis2_eval, nbe2,
      zmat2, nbe2, nbe2_scr );
    lwd->eval_xmat( npts, nbf2, nbe2, submat_map2, P2z, ldp2z, basis2_eval, nbe2,
      zmat2 + npts*nbe2, nbe2, nbe2_scr + nbe2 * nbe2);

    // Evaluate U and V variables
    lwd->eval_uvvar_lda_uks( npts, nbe2, basis2_eval, zmat2, nbe2, den2_eval );

    // No protonic XC functional. Fill with eps and vrho to be 0.0
    std::fill_n(eps2,  npts,   0.);
    std::fill_n(vrho2, npts*2, 0.);
    //----------------------End Calculating Protonic Density & UV Variable------------------------




    if(this->load_balancer_->epc_functional() == EPCFunctional::EPC17){
      //----------------------Start epc-17-2 functional Evaluation------------------------
      for (int32_t iPt = 0; iPt < npts; iPt++ ){
        // Get Electronic density scalar (RKS)
        value_type total_erho = std::abs(den_eval[iPt] > 1e-15) ? den_eval[iPt] : 0.0;
        // Get Protonic density scalar (UKS)
        value_type total_prho = std::abs(den2_eval[2*iPt] + den2_eval[2*iPt+1]) > 1e-15? 
            den2_eval[2*iPt]+den2_eval[2*iPt+1] : 0.0;
        
        // Skip this point if the density is too small
        if(total_erho < 1e-15 | total_prho < 1e-15){
          eps2[iPt]      = 0.0;
          vrho2[2*iPt]   = 0.0;
          vrho2[2*iPt+1] = 0.0;
          continue;
        }

        // epc-17-2 denominator
        value_type dn = 2.35 - 2.4 * std::sqrt(total_erho*total_prho) + 6.6 * (total_erho*total_prho);

        // Update electronic eps and vxc
        eps[iPt]      += -1.0 * total_prho/dn;
        vrho[iPt]     +=  ( -1.0 * total_prho / dn + (-1.2 * std::sqrt(total_erho) * std::sqrt(total_prho) * total_prho 
                          + 6.6 * total_erho * total_prho * total_prho ) / (dn * dn) );

        // Assign protonic eps and vxc
        eps2[iPt]      = -1.0 * total_erho/dn;
        vrho2[2*iPt]   =  ( -1.0 * total_erho / dn + (-1.2 * std::sqrt(total_prho) * std::sqrt(total_erho) * total_erho 
                          + 6.6 * total_erho * total_erho * total_prho ) / (dn * dn) );
        vrho2[2*iPt+1] =  0.0;
      }
      //----------------------End epc-17-2 functional Evaluation------------------------  
    } else{
      GAUXC_GENERIC_EXCEPTION("Only EPC17 is supported in GauXC");
    }
 





    
    // Factor weights into XC results
    for( int32_t i = 0; i < npts; ++i ) {
      // Electronic
      eps[i]       *= weights[i];
      vrho[i]      *= weights[i];
      // Protonic
      eps2[i]      *= weights[i];
      vrho2[2*i]   *= weights[i];
      vrho2[2*i+1] *= weights[i];
    }

    if( func.is_gga() )
      for( int32_t i = 0; i < npts; ++i ) vgamma[i] *= weights[i];
      
    



    // Evaluate Z matrix for VXC 
    // Electronic
    if( func.is_gga() )
      lwd->eval_zmat_gga_vxc( npts, nbe, vrho, vgamma, basis_eval, dbasis_x_eval,
                              dbasis_y_eval, dbasis_z_eval, dden_x_eval, dden_y_eval,
                              dden_z_eval, zmat, nbe); 
    else
      lwd->eval_zmat_lda_vxc( npts, nbe, vrho, basis_eval, zmat, nbe ); 
    // Protonic
    lwd->eval_zmat_lda_vxc_uks( npts, nbe2, vrho2, basis2_eval, zmat2, nbe2 );


    // Incremeta LT of VXC
    #pragma omp critical
    {
      // Scalar integrations
      for( int32_t i = 0; i < npts; ++i ) {
        *N_EL  += weights[i] * den_eval[i];
        // Electronic XC (VXC+EPC)
        *EXC1  += eps[i]     * den_eval[i];
        // Protonic (EPC)
        *EXC2  += eps2[i]    * (den2_eval[2*i] +  den2_eval[2*i+1]);    
      }

      // Increment VXC
      // Electronic 
      lwd->inc_vxc( npts, nbf, nbe, basis_eval, submat_map, zmat, nbe, VXC1s, ldvxc1s,
        nbe_scr );
      // Protonic
      lwd->inc_vxc( npts, nbf2, nbe2, basis2_eval, submat_map2, zmat2, nbe2, VXC2s, ldvxc2s,
        nbe2_scr );
      lwd->inc_vxc( npts, nbf2, nbe2, basis2_eval, submat_map2, zmat2+ npts*nbe2, nbe2, VXC2z, ldvxc2z,
        nbe2_scr + nbe2 * nbe2);

    }

  } // Loop over tasks
  
  }  // End OpenMP region

  //std::cout << "N_EL = " << std::setprecision(12) << std::scientific << *N_EL << std::endl;
  //std::cout << "EXC1 = " << std::setprecision(12) << std::scientific << *EXC1 << std::endl
  //std::cout << "EXC2 = " << std::setprecision(12) << std::scientific << *EXC2 << std::endl;

  // Symmetrize Electronic VXC
  for( int32_t j = 0;   j < nbf; ++j )
  for( int32_t i = j+1; i < nbf; ++i )
    VXC1s[ j + i*nbf ] = VXC1s[ i + j*nbf ];

  // Symmetrize Protonic VXC
  for( int32_t j = 0;   j < nbf2; ++j ){
  for( int32_t i = j+1; i < nbf2; ++i ){
    VXC2s[ j + i*nbf2 ] = VXC2s[ i + j*nbf2 ];
    VXC2z[ j + i*nbf2 ] = VXC2z[ i + j*nbf2 ];
  }
  }
  

} 

template <typename ValueType>
void ReferenceReplicatedXCHostIntegrator<ValueType>::
  neo_exc_vxc_local_work_( const value_type* P1s, int64_t ldp1s,
                            const value_type* P1z, int64_t ldp1z,
                            const value_type* P2s, int64_t ldp2s,
                            const value_type* P2z, int64_t ldp2z,
                            value_type* VXC1s, int64_t ldvxc1s,
                            value_type* VXC1z, int64_t ldvxc1z,
                            value_type* VXC2s, int64_t ldvxc2s,
                            value_type* VXC2z, int64_t ldvxc2z,
                            value_type* EXC1, value_type* EXC2, value_type *N_EL ) {
  
  GAUXC_GENERIC_EXCEPTION("neo_exc_vxc_local_work_ UKS NYI");
}

}
}
