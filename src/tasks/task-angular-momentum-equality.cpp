//
// Copyright (c) 2017 CNRS
//
// This file is part of tsid
// tsid is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
// tsid is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Lesser Public License for more details. You should have
// received a copy of the GNU Lesser General Public License along with
// tsid If not, see
// <http://www.gnu.org/licenses/>.
//

#include "tsid/tasks/task-angular-momentum-equality.hpp"
#include "tsid/robots/robot-wrapper.hpp"
#include <pinocchio/algorithm/joint-configuration.hpp>
#include <pinocchio/algorithm/centroidal.hpp>

namespace tsid
{
  namespace tasks
  {
    using namespace math;
    using namespace trajectories;
    using namespace pinocchio;

    TaskAMEquality::TaskAMEquality(const std::string & name,
                                     RobotWrapper & robot):
      TaskBase(name, robot),
      m_constraint(name, 3, robot.nv())
    {
      m_Kp.setZero(3);
      m_Kd.setZero(3);
      m_L_error.setZero(3);
      m_dL_error.setZero(3);
      m_L.setZero(3);
      m_dL.setZero(3);
      m_dL_des.setZero(3);
    }

    int TaskAMEquality::dim() const
    {
      //return self._mask.sum ()
      return 3;
    }

    const Vector3 & TaskAMEquality::Kp(){ return m_Kp; }

    const Vector3 & TaskAMEquality::Kd(){ return m_Kd; }

    void TaskAMEquality::Kp(ConstRefVector Kp)
    {
      assert(Kp.size()==3);
      m_Kp = Kp;
    }

    void TaskAMEquality::Kd(ConstRefVector Kd)
    {
      assert(Kd.size()==3);
      m_Kd = Kd;
    }

    void TaskAMEquality::setReference(const TrajectorySample & ref)
    {
      m_ref = ref;
    }

    const TrajectorySample & TaskAMEquality::getReference() const
    {
      return m_ref;
    }

    const Vector3 & TaskAMEquality::getDesireddMomentum() const
    {
      return m_dL_des;
    }

    Vector3 TaskAMEquality::getdMomentum(ConstRefVector dv) const
    {
      return m_constraint.matrix()*dv - m_drift;
    }

    const Vector3 & TaskAMEquality::momentum_error() const
    {
      return m_L_error;
    }

    const Vector3 & TaskAMEquality::momentum() const
    {
      return m_L;
    }
    const Vector & TaskAMEquality::momentum_ref() const
    {
      return m_ref.vel;
    }

    const Vector & TaskAMEquality::dmomentum_ref() const
    {
      return m_ref.acc;
    }

    const ConstraintBase & TaskAMEquality::getConstraint() const
    {
      return m_constraint;
    }

    const ConstraintBase & TaskAMEquality::compute(const double ,
                                                    ConstRefVector q,
                                                    ConstRefVector v,
                                                    const Data & data)
    {
      // Compute errors
      // Get momentum jacobian
      const Matrix6x & J_am = m_robot.momentumJacobian(data);
      m_L = J_am.bottomRows(3) * v;
      m_L_error = m_L - m_ref.vel;

      m_dL_des = - m_Kp.cwiseProduct(m_L_error)
                + m_ref.acc;

#ifndef NDEBUG
      std::cout<<m_name<<" errors: "<<m_L_error.norm()<<" "
        <<m_dL_error.norm()<<std::endl;
#endif
      // Del Prete's quick and dirty way to compute drift
      // compute momentum Jacobian at next time step assuming zero acc
      double dt = 1e-3;
      const Vector & q_next = pinocchio::integrate(m_robot.model(), q, dt*v);
      Data data_ccrba = data;
      pinocchio::ccrba(m_robot.model(), data_ccrba, q_next, v);
      const Matrix6x & J_am_next = m_robot.momentumJacobian(data_ccrba);
      m_drift = (J_am_next.bottomRows(3) - J_am.bottomRows(3))* v / dt;

      m_constraint.setMatrix(J_am.bottomRows(3));
      m_constraint.setVector(m_dL_des - m_drift);

      return m_constraint;
    }
    
  }
}