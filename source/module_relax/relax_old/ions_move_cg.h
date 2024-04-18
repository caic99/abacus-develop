#ifndef IONS_MOVE_CG_H
#define IONS_MOVE_CG_H

#include "module_base/matrix.h"
#include "module_cell/unitcell.h"
class Ions_Move_CG
{
  public:
    Ions_Move_CG();
    ~Ions_Move_CG();

    void allocate(void);
    void start(UnitCell &ucell, const ModuleBase::matrix &force, const double &etot);

    static double RELAX_CG_THR;
    int sd_step{};
    int cg_step{};

  private:
    double *pos0;
    double *grad0;
    double *cg_grad0;
    double *move0;
    double e0{};
    // setup gradients.
    static void setup_cg_grad(const double *grad,
                       const double *grad0,
                       double *cg_grad,
                       const double *cg_grad0,
                       const int &ncggrad,
                       int &flag); // LiuXh fix bug of lpf, 20180515
    static void setup_move(double *move, const double *cg_gradn, const double &trust_radius);
    static void Brent(double &fa, double &fb, double &fc, double &xa, double &xb, double &xc, double &best_x, double &xpt);
    static void f_cal(const double *g0, const double *g1, const int &dim, double &f_value);
    static void third_order(const double &e0,
                     const double &e1,
                     const double &fa,
                     const double &fb,
                     const double x,
                     double &best_x);
};

#endif
