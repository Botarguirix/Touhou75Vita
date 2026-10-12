#pragma once
#include <cstdint>
#include <cstdio>

namespace th075 {
// Read-only observations at the validated main frame-event wait. Seeing a new
// owner is evidence of a handoff, not proof that selection or combat works.
class SceneTrace {
public:
    void observe(uint32_t object,uint32_t vtable,uint32_t update,uint32_t draw,unsigned frame,FILE* log) {
        if(!object || !vtable)return;
        ++observations_;
        if(object==object_ && vtable==vtable_)return;
        if(observed_){++changes_;if(vtable_==0x00658308 && vtable!=0x00658308)++title_handoffs_;}
        fprintf(log,"startup_session_scene_change=frame:%u from_object:0x%08X from_vtable:0x%08X object:0x%08X vtable:0x%08X update:0x%08X draw:0x%08X scope:read_only_owner_transition\n",frame,object_,vtable_,object,vtable,update,draw);
        object_=object;vtable_=vtable;observed_=true;
    }
    void log(FILE* file) const {
        fprintf(file,"startup_session_scene_summary=observations:%u changes:%u title_handoffs:%u last_object:0x%08X last_vtable:0x%08X combat:unverified\n",observations_,changes_,title_handoffs_,object_,vtable_);
    }
private:
    uint32_t object_=0,vtable_=0;unsigned observations_=0,changes_=0,title_handoffs_=0;bool observed_=false;
};
}
