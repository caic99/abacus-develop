// #include "../src_pw/diago_cg.h"
#include "../module_base/constants.h"
#include "../module_base/timer.h"
#include "../module_xc/xc_functional.h"
#include "../src_parallel/parallel_reduce.h"
#include "surchem.h"

#include <cmath>

ModuleBase::matrix surchem::v_correction(const UnitCell &cell,
                                         ModulePW::PW_Basis *rho_basis,
                                         const int &nspin,
                                         const double *const *const rho)
{
    ModuleBase::TITLE("surchem", "v_correction");
    ModuleBase::timer::tick("surchem", "v_correction");

    double *Porter = new double[rho_basis->nrxx];
    for (int i = 0; i < rho_basis->nrxx; i++)
        Porter[i] = 0.0;
    const int nspin0 = (nspin == 2) ? 2 : 1;
    for (int is = 0; is < nspin0; is++)
        for (int ir = 0; ir < rho_basis->nrxx; ir++)
            Porter[ir] += rho[is][ir];

    complex<double> *Porter_g = new complex<double>[rho_basis->npw];
    ModuleBase::GlobalFunc::ZEROS(Porter_g, rho_basis->npw);

    GlobalC::UFFT.ToReciSpace(Porter, Porter_g, rho_basis);

    this->Porter_g_anchor = Porter_g[rho_basis->ig_gge0];

    complex<double> *N = new complex<double>[rho_basis->npw];
    complex<double> *TOTN = new complex<double>[rho_basis->npw];
    complex<double> *PS_TOTN = new complex<double>[rho_basis->npw];

    cal_totn(cell, rho_basis, Porter_g, N, TOTN);

    cal_pseudo(cell, rho_basis, Porter_g, PS_TOTN);

    ModuleBase::matrix v(nspin, rho_basis->nrxx);

    v += cal_vel(cell, rho_basis, TOTN, PS_TOTN, nspin);
    v += cal_vcav(cell, rho_basis, PS_TOTN, nspin);

    delete[] Porter;
    delete[] Porter_g;
    delete[] N;
    delete[] PS_TOTN;
    delete[] TOTN;

    ModuleBase::timer::tick("surchem", "v_correction");
    return v;
}

void surchem::add_comp_chg(const UnitCell &cell,
                           ModulePW::PW_Basis *rho_basis,
                           double q,
                           double l,
                           double center,
                           complex<double> *NG,
                           int dim)
{
    // x dim
    double tmp_q = 0.0;
    if (dim == 0)
    {
        double L = cell.a1[0];
        tmp_q = q / (cross(cell.a2, cell.a3).norm() * l);
        ModuleBase::GlobalFunc::ZEROS(NG, rho_basis->npw);
        for (int ig = 0; ig < rho_basis->npw; ig++)
        {
            if (ig == rho_basis->ig_gge0)
                continue;
            double GX = rho_basis->gcar[ig][0];
            double GY = rho_basis->gcar[ig][1];
            double GZ = rho_basis->gcar[ig][2];
            GX = GX * 2 * ModuleBase::PI;
            if (GY == 0 && GZ == 0 && GX != 0)
            {
                NG[ig] = exp(ModuleBase::NEG_IMAG_UNIT * GX * center)
                         * complex<double>(2.0 * tmp_q * sin(GX * l / 2.0) / (L * GX), 0.0);
            }
        }
    }
    // y dim
    else if (dim == 1)
    {
        double L = cell.a2[1];
        tmp_q = q / (cross(cell.a1, cell.a3).norm() * l);
        ModuleBase::GlobalFunc::ZEROS(NG, rho_basis->npw);
        for (int ig = 0; ig < rho_basis->npw; ig++)
        {
            if (ig == rho_basis->ig_gge0)
                continue;
            double GX = rho_basis->gcar[ig][0];
            double GY = rho_basis->gcar[ig][1];
            double GZ = rho_basis->gcar[ig][2];
            GY = GY * 2 * ModuleBase::PI;
            if (GX == 0 && GZ == 0 && GY != 0)
            {
                NG[ig] = exp(ModuleBase::NEG_IMAG_UNIT * GY * center)
                         * complex<double>(2.0 * tmp_q * sin(GY * l / 2.0) / (L * GY), 0.0);
            }
        }
    }
    // z dim
    else if (dim == 2)
    {
        double L = cell.a3[2];
        tmp_q = q / (cross(cell.a1, cell.a2).norm() * l);
        ModuleBase::GlobalFunc::ZEROS(NG, rho_basis->npw);
        for (int ig = 0; ig < rho_basis->npw; ig++)
        {
            if (ig == rho_basis->ig_gge0)
                continue;
            double GX = rho_basis->gcar[ig][0];
            double GY = rho_basis->gcar[ig][1];
            double GZ = rho_basis->gcar[ig][2];
            GZ = GZ * 2 * ModuleBase::PI;
            if (GX == 0 && GY == 0 && GZ != 0)
            {
                NG[ig] = exp(ModuleBase::NEG_IMAG_UNIT * GZ * center)
                         * complex<double>(2.0 * tmp_q * sin(GZ * l / 2.0) / (L * GZ), 0.0);
            }
        }
    }
}

ModuleBase::matrix surchem::v_compensating(const UnitCell &cell,
                                           ModulePW::PW_Basis *rho_basis,
                                           const int &nspin,
                                           const double *const *const rho)
{
    ModuleBase::TITLE("surchem", "v_compensating");
    ModuleBase::timer::tick("surchem", "v_compensating");

    // calculating v_comp also need TOTN_real
    double *Porter = new double[rho_basis->nrxx];
    for (int i = 0; i < rho_basis->nrxx; i++)
        Porter[i] = 0.0;
    const int nspin0 = (nspin == 2) ? 2 : 1;
    for (int is = 0; is < nspin0; is++)
        for (int ir = 0; ir < rho_basis->nrxx; ir++)
            Porter[ir] += rho[is][ir];

    complex<double> *Porter_g = new complex<double>[rho_basis->npw];
    ModuleBase::GlobalFunc::ZEROS(Porter_g, rho_basis->npw);

    GlobalC::UFFT.ToReciSpace(Porter, Porter_g, rho_basis);

    this->Porter_g_anchor = Porter_g[rho_basis->ig_gge0];

    complex<double> *N = new complex<double>[rho_basis->npw];
    complex<double> *TOTN = new complex<double>[rho_basis->npw];

    cal_totn(cell, rho_basis, Porter_g, N, TOTN);

    // save TOTN in real space
    GlobalC::UFFT.ToRealSpace(TOTN, this->TOTN_real, rho_basis);

    complex<double> *comp_reci = new complex<double>[rho_basis->npw];
    complex<double> *phi_comp_G = new complex<double>[rho_basis->npw];
    // double *phi_comp_R = new double[rho_basis->nrxx];

    ModuleBase::GlobalFunc::ZEROS(comp_reci, rho_basis->npw);
    ModuleBase::GlobalFunc::ZEROS(phi_comp_G, rho_basis->npw);
    ModuleBase::GlobalFunc::ZEROS(phi_comp_R, rho_basis->nrxx);
    // get compensating charge in reci space
    add_comp_chg(cell, rho_basis, comp_q, comp_l, comp_center, comp_reci, comp_dim);
    // save compensating charge in real space
    GlobalC::UFFT.ToRealSpace(comp_reci, this->comp_real, rho_basis);

    for (int ig = 0; ig < rho_basis->npw; ig++)
    {
        if (rho_basis->gg[ig] >= 1.0e-12) // LiuXh 20180410
        {
            const double fac = ModuleBase::e2 * ModuleBase::FOUR_PI / (cell.tpiba2 * rho_basis->gg[ig]);
            phi_comp_G[ig] = fac * comp_reci[ig];
        }
    }

    GlobalC::UFFT.ToRealSpace(phi_comp_G, phi_comp_R, rho_basis);
    ModuleBase::matrix v_comp(GlobalV::NSPIN, rho_basis->nrxx);
    if (GlobalV::NSPIN == 4)
    {
        for (int ir = 0; ir < rho_basis->nrxx; ir++)
            v_comp(0, ir) = phi_comp_R[ir];
    }
    else
    {
        for (int is = 0; is < GlobalV::NSPIN; is++)
            for (int ir = 0; ir < rho_basis->nrxx; ir++)
                v_comp(is, ir) = phi_comp_R[ir];
    }

    delete[] comp_reci;
    delete[] phi_comp_G;
    // delete[] phi_comp_R;
    delete[] Porter;
    delete[] Porter_g;
    delete[] N;
    delete[] TOTN;

    ModuleBase::timer::tick("surchem", "v_compensating");
    return v_comp;
}

// void cal_comp_force_bak(ModuleBase::matrix &force_comp, ModulePW::PW_Basis *rho_basis)
// {
//     ModuleBase::timer::tick("surchem", "cal_comp_force");

//     std::complex<double> *aux = new std::complex<double>[rho_basis->nmaxgr];
//     ModuleBase::GlobalFunc::ZEROS(aux, rho_basis->nrxx);

//     // get nuclear chg
//     complex<double> *vloc_g = new complex<double>[rho_basis->npw];
//     complex<double> *n_reci = new complex<double>[rho_basis->npw];
//     double *n_real = new double[rho_basis->nrxx];
//     GlobalC::UFFT.ToReciSpace(GlobalC::pot.vltot, vloc_g, rho_basis); // now n is vloc in Recispace
//     for (int ig = 0; ig < rho_basis->npw; ig++) {
//         if(ig==rho_basis->ig_gge0)
//         {
//             n_reci[ig] = Porter_g_anchor; // saved zero item
//             continue;
//         }
//         const double fac = ModuleBase::e2 * ModuleBase::FOUR_PI /
//                             (GlobalC::ucell.tpiba2 * rho_basis->gg[ig]);

//         n_reci[ig] = -vloc_g[ig] / fac;
//     }

//     GlobalC::UFFT.ToRealSpace(n_reci, n_real, rho_basis);

//     // for (int i = 0; i < rho_basis->nz; i++)
//     // {
//     //     cout << n_real[i] << endl;
//     // }

//     for (int is = 0; is < GlobalV::NSPIN; is++)
//     {
//         for (int ir = 0; ir < rho_basis->nrxx; ir++)
//         {
//             aux[ir] += std::complex<double>(n_real[ir], 0.0);
//         }
//     }

//     // to G space.
//     rho_basis->real2recip(aux, aux);

//     int iat = 0;
//     for (int it = 0; it < GlobalC::ucell.ntype; it++)
//     {
//         for (int ia = 0; ia < GlobalC::ucell.atoms[it].na; ia++)
//         {
//             for (int ig = 0; ig < rho_basis->npw; ig++)
//             {
//                 const double phase = ModuleBase::TWO_PI * (rho_basis->gcar[ig] * GlobalC::ucell.atoms[it].tau[ia]);
//                 const double factor
//                     = phi_comp_R[rho_basis->ig2igg[ig]] * (cos(phase) * aux[ig].imag() + sin(phase) * aux[ig].real());
//                 force_comp(iat, 0) += rho_basis->gcar[ig][0] * factor;
//                 force_comp(iat, 1) += rho_basis->gcar[ig][1] * factor;
//                 force_comp(iat, 2) += rho_basis->gcar[ig][2] * factor;
//             }
//             for (int ipol = 0; ipol < 3; ipol++)
//             {
//                 force_comp(iat, ipol) *= (GlobalC::ucell.tpiba * GlobalC::ucell.omega);
//             }
//             ++iat;
//         }
//     }
//     // for (int i = 0; i < rho_basis->nz; i++)
//     // {
//     //     cout << GlobalC::CHR.rho[0][i] << endl;
//     // }
//     // cout << force_comp(0, 0) << " ";
//     // cout << force_comp(0, 1) << " ";
//     // cout << force_comp(0, 2) << endl;
//     // cout << force_comp(1, 0) << " ";
//     // cout << force_comp(1, 1) << " ";
//     // cout << force_comp(1, 2) << endl;
//     // this->print(GlobalV::ofs_running, "comp forces", force_comp);
//     Parallel_Reduce::reduce_double_pool(force_comp.c, force_comp.nr * force_comp.nc);
//     delete[] aux;
//     delete[] vloc_g;
//     delete[] n_real;
//     delete[] n_reci;
//     ModuleBase::timer::tick("surchem", "cal_comp_force");
//     return;
// }

void surchem::cal_comp_force(ModuleBase::matrix &force_comp, ModulePW::PW_Basis *rho_basis)
{
    int iat = 0;
    std::complex<double> *N = new std::complex<double>[rho_basis->npw];
    std::complex<double> *phi_comp_G = new complex<double>[rho_basis->npw];
    std::complex<double> *vloc_at = new std::complex<double>[rho_basis->npw];
    GlobalC::UFFT.ToReciSpace(phi_comp_R, phi_comp_G, rho_basis);

    for (int it = 0; it < GlobalC::ucell.ntype; it++)
    {
        for (int ia = 0; ia < GlobalC::ucell.atoms[it].na; ia++)
        {

            cout << GlobalC::ucell.atoms[it].zv << endl;
            for (int ig = 0; ig < rho_basis->npw; ig++)
            {   
                complex<double> phase = exp( ModuleBase::NEG_IMAG_UNIT *ModuleBase::TWO_PI * ( rho_basis->gcar[ig] * GlobalC::ucell.atoms[it].tau[ia]));
                //vloc for each atom
                vloc_at[ig] = GlobalC::ppcell.vloc(it, rho_basis->ig2igg[ig]) * phase;
                if(rho_basis->ig_gge0 == ig)
                {
                    N[ig] = GlobalC::ucell.atoms[it].zv / GlobalC::ucell.omega;
                }
                else
                {
                    const double fac = ModuleBase::e2 * ModuleBase::FOUR_PI /
                               (GlobalC::ucell.tpiba2 * rho_basis->gg[ig]);

                    N[ig] = -vloc_at[ig] / fac;
                }
                
                //force for each atom
                force_comp(iat, 0) += rho_basis->gcar[ig][0] * imag(conj(phi_comp_G[ig]) * N[ig]);
                force_comp(iat, 1) += rho_basis->gcar[ig][1] * imag(conj(phi_comp_G[ig]) * N[ig]);
                force_comp(iat, 2) += rho_basis->gcar[ig][2] * imag(conj(phi_comp_G[ig]) * N[ig]);
            }
                
            force_comp(iat, 0) *= (GlobalC::ucell.tpiba * GlobalC::ucell.omega);
            force_comp(iat, 1) *= (GlobalC::ucell.tpiba * GlobalC::ucell.omega);
            force_comp(iat, 2) *= (GlobalC::ucell.tpiba * GlobalC::ucell.omega);

            force_comp(iat, 0) *= 2;
            force_comp(iat, 1) *= 2;
            force_comp(iat, 2) *= 2;

            cout << "Force1(Ry / Bohr)" << iat << ":"
                 << " " << force_comp(iat, 0) << " " << force_comp(iat, 1) << " " << force_comp(iat, 2) << endl;

            ++iat;
        }
    }
    delete[] vloc_at;
    delete[] N;
    delete[] phi_comp_G;
}