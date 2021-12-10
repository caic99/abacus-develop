#include "pw_basis.h"
#include "../module_base/global_function.h"
#ifdef __MPI
#include "mpi.h"
#include "../src_parallel/parallel_global.h"
#endif
namespace ModulePW
{
//
//in: (nplane,nxy) out:(nz,nst)
//in and out should be in different places 
//in[] will be changed
//
void PW_Basis:: gatherp_scatters(complex<double> *in, complex<double> *out)
{
    if(this->poolnproc == 1) //In this case nst=nstot, nz = nplane, 
    {
        for(int is = 0 ; is < this->nst ; ++is)
        {
            int bigixy = this->istot2bigixy[is];
            //int bigixy = (ixy / ny)*bigny + ixy % ny;
            for(int iz = 0 ; iz < this->nz ; ++iz)
            {
                out[is*nz+iz] = in[bigixy*nz+iz];
            }
        }
        return;
    }
#ifdef __MPI
    //change (nplane nxy) to (nplane,nstot)
    // Hence, we can send them at one time.  
	for (int istot = 0;istot < nstot; ++istot)
	{
		int bigixy = this->istot2bigixy[istot];
        //int bigixy = (ixy / ny)*bigny + ixy % ny;
		for (int iz = 0; iz < nplane; ++iz)
		{
			out[istot*nplane+iz] = in[bigixy*nplane+iz];
		}
	}

    //exchange data
    //(nplane,nstot) to (numz[ip],ns, poolnproc)
	MPI_Alltoallv(out, numr, startr, mpicomplex, in, numg, startg, mpicomplex, POOL_WORLD);

    // change (nz,ns) to (numz[ip],ns, poolnproc)
    for (int ip = 0; ip < this->poolnproc ;++ip)
	{
        int nzip = this->numz[ip];
		for (int is = 0; is < this->nst; ++is)
		{
			for (int izip = 0; izip < nzip; ++izip)
			{
				out[ is * nz + startz[ip] + izip] = in[startg[ip] + is*nzip + izip];
			}
		}
	}
   
#endif
    return;
}

//
//in: (nz,nst) out:(nplane,nxy)
//in and out should be in different places 
//in[] will be changed
//
void PW_Basis:: gathers_scatterp(complex<double> *in, complex<double> *out)
{
    
    if(this->poolnproc == 1) //In this case nrxx=nx*ny*nz, nst = nstot, 
    {
        ModuleBase::GlobalFunc::ZEROS(out, this->nrxx);
        for(int is = 0 ; is < this->nst ; ++is)
        {
            int bigixy = istot2bigixy[is];
            //int bigixy = (ixy / ny)*bigny + ixy % ny;
            for(int iz = 0 ; iz < this->nz ; ++iz)
            {
                out[bigixy*nz+iz] = in[is*nz+iz];
            }
        }
        return;
    }
#ifdef __MPI
    // change (nz,ns) to (numz[ip],ns, poolnproc)
    // Hence, we can send them at one time.  
    for (int ip = 0; ip < this->poolnproc ;++ip)
	{
        int nzip = this->numz[ip];
		for (int is = 0; is < this->nst; ++is)
		{
			for (int izip = 0; izip < nzip; ++izip)
			{
				out[startg[ip] + is*nzip + izip] = in[ is * nz + startz[ip] + izip];
			}
		}
	}

	//exchange data
    //(numz[ip],ns, poolnproc) to (nplane,nstot)
	MPI_Alltoallv(out, numg, startg, mpicomplex, in, numr, startr, mpicomplex, POOL_WORLD);

    ModuleBase::GlobalFunc::ZEROS(out, this->nrxx);
    //change (nplane,nstot) to (nplane nxy)
	for (int istot = 0;istot < nstot; ++istot)
	{
		int bigixy = this->istot2bigixy[istot];
        //int bigixy = (ixy / ny)*bigny + ixy % ny;
		for (int iz = 0; iz < nplane; ++iz)
		{
			out[bigixy*nplane+iz] = in[istot*nplane+iz];
		}
	}

#endif
    return;
}



}