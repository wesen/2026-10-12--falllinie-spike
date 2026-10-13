#pragma once
#include "game.hpp"
namespace fl {
struct IkResult {Vec3 mid,end;};
IkResult ik2(Vec3 origin,Vec3 target,double l1,double l2,Vec3 pole);
struct Pose {Vec3 pelvis,board_position;Quat torso,head,board;std::array<Vec3,2> hands,feet;std::optional<Vec3> knee_pole;};
Pose base_pose(double crouch,double lean,double steer,int grab,double tweak,double amount,double look);
Pose bind_feet(Pose pose);Pose uber_pose(int id,double t);Pose blend_pose(const Pose&,const Pose&,double weight);
struct RiderRig {
 Vec3 origin,pelvis,chest,neck,head,board_position;Quat orientation,head_orientation,board_orientation;
 std::array<Vec3,2> hip,knee,ankle,shoulder,elbow,hand;
};
RiderRig compute_rig(const Game&);
struct RiderMesh {CpuMesh mesh;std::vector<std::array<uint32_t,3>> groups;};
// Reusable original primitive topology; only vertex transforms change per frame.
class RiderGeometry {
public:
 RiderGeometry();void update(const RiderRig& rig,RiderMesh& output)const;
private:
 CpuMesh cylinder_,sphere_,box_,board_;
};
}
