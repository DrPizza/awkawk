#include "scopeguard.h"

namespace Loki
{
    template <class Obj, typename MemFun, typename P1, typename P2, typename P3>
    class ObjScopeGuardImpl3 : public ScopeGuardImplBase
    {
    public:
        static ObjScopeGuardImpl3<Obj, MemFun, P1, P2, P3> MakeObjGuard(Obj& obj, MemFun memFun, P1 p1, P2 p2, P3 p3)
        {
            return ObjScopeGuardImpl3<Obj, MemFun, P1, P2, P3>(obj, memFun, p1, p2, p3);
        }

        ~ObjScopeGuardImpl3() throw()
        {
            SafeExecute(*this);
        }

        void Execute()
        {
            (obj_.*memFun_)(p1_, p2_, p3_);
        }

    protected:
        ObjScopeGuardImpl3(Obj& obj, MemFun memFun, P1 p1, P2 p2, P3 p3) : obj_(obj), memFun_(memFun), p1_(p1), p2_(p2), p3_(p3)
        {}

        Obj& obj_;
        MemFun memFun_;
        const P1 p1_;
        const P2 p2_;
        const P3 p3_;
    };

    template <class Obj, typename MemFun, typename P1, typename P2, typename P3>
    inline ObjScopeGuardImpl3<Obj, MemFun, P1, P2, P3> MakeObjGuard(Obj& obj, MemFun memFun, P1 p1, P2 p2, P3 p3)
    {
        return ObjScopeGuardImpl3<Obj, MemFun, P1, P2, P3>::MakeObjGuard(obj, memFun, p1, p2, p3);
    }

    template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a, typename P2b, typename P3a, typename P3b>
    inline ObjScopeGuardImpl3<Obj1,Ret(Obj2::*)(P1a,P2a,P3a),P1b,P2b,P3b> MakeGuard(Ret(Obj2::*memFun)(P1a,P2a,P3a), Obj1 &obj, P1b p1, P2b p2, P3b p3)
    {
      return ObjScopeGuardImpl3<Obj1,Ret(Obj2::*)(P1a,P2a,P3a),P1b,P2b,P3b>::MakeObjGuard(obj,memFun,p1,p2,p3);
    }

    template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a, typename P2b, typename P3a, typename P3b>
    inline ObjScopeGuardImpl3<Obj1,Ret(Obj2::*)(P1a,P2a,P3a),P1b,P2b,P3b> MakeGuard(Ret(Obj2::*memFun)(P1a,P2a,P3a), Obj1 *obj, P1b p1, P2b p2, P3b p3)
    {
      return ObjScopeGuardImpl3<Obj1,Ret(Obj2::*)(P1a,P2a,P3a),P1b,P2b,P3b>::MakeObjGuard(*obj,memFun,p1,p2,p3);
    }
}

#define ON_BLOCK_EXIT      Loki::ScopeGuard LOKI_ANONYMOUS_VARIABLE(scopeGuard) = Loki::MakeGuard
#define ON_BLOCK_EXIT_OBJ  Loki::ScopeGuard LOKI_ANONYMOUS_VARIABLE(scopeGuard) = Loki::MakeObjGuard
