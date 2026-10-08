#include "startup_services.h"
#include "worker_probe.h"
#include "cp932_data.h"
#include "pe_resources.h"
#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include "runtime/guest_thread_ctx.h"
#include <psp2/rtc.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/stat.h>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <cstring>

namespace {
constexpr uint32_t kStack = 0x00800000, kStackEnd = 0x00A00000;
constexpr uint32_t kMajor = 5, kMinor = 1, kBuild = 2600, kPlatform = 2;
bool in_stack(uint32_t p, uint32_t size) {
    return (p >= kStack && uint64_t(p) + size <= kStackEnd) ||
        (p >= 0x00A00000 && uint64_t(p)+size <= 0x00A40000);
}
void put32(uint8_t* out, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) out[i] = uint8_t(value >> (8 * i));
}
const Cp932Type* cp932_entry(uint16_t unit) {
    const auto* end = std::end(kCp932Types);
    const auto* entry = std::lower_bound(std::begin(kCp932Types), end, unit,
        [](const Cp932Type& item, uint16_t value) { return item.unit < value; });
    return entry == end || entry->unit != unit ? nullptr : entry;
}
bool cp932_ctype1(uint16_t unit, uint16_t& flags) {
    const auto* entry = cp932_entry(unit);
    if (!entry) return false;
    flags = entry->flags;
    return true;
}
}

StartupServiceResult StartupServices::call(const d2rt::ImportRef& import) {
    const bool metrics = import.dll == "USER32.dll" && import.name == "GetSystemMetrics";
    const bool icon_load = import.dll == "USER32.dll" && import.name == "LoadIconA";
    const bool cursor_load = import.dll == "USER32.dll" && import.name == "LoadCursorA";
    const bool class_register = import.dll == "USER32.dll" && import.name == "RegisterClassExA";
    const bool window_create = import.dll == "USER32.dll" && import.name == "CreateWindowExA";
    const bool window_default = import.dll == "USER32.dll" && import.name == "DefWindowProcA";
    const bool window_show = import.dll == "USER32.dll" && import.name == "ShowWindow";
    const bool window_update = import.dll == "USER32.dll" && import.name == "UpdateWindow";
    const bool com_init = import.dll == "ole32.dll" && import.name == "CoInitialize";
    const bool com_uninit = import.dll == "ole32.dll" && import.name == "CoUninitialize";
    const bool com = com_init || com_uninit;
    const bool stock = import.dll == "GDI32.dll" && import.name == "GetStockObject";
    const bool gui = icon_load || cursor_load || class_register || stock || window_create || window_default || window_show || window_update;
    const bool winmm = import.dll == "WINMM.dll";
    const bool timer_begin = winmm && import.name == "timeBeginPeriod";
    const bool timer_end = winmm && import.name == "timeEndPeriod";
    const bool timer_time = winmm && import.name == "timeGetTime";
    const bool multimedia_timer = timer_begin || timer_end || timer_time;
    if (import.dll != "KERNEL32.dll" && !multimedia_timer && !metrics && !gui && !com) return StartupServiceResult::Unsupported;
    const bool cwd_set = import.name == "SetCurrentDirectoryA";
    const bool cwd_get = import.name == "GetCurrentDirectoryA";
    const bool cwd = cwd_set || cwd_get;
    const bool version = import.name == "GetVersionExA";
    const bool library_load = import.name == "LoadLibraryA";
    const bool module = import.name == "GetModuleHandleA" || library_load;
    const bool proc_address = import.name == "GetProcAddress";
    const bool critical_plain = import.name == "InitializeCriticalSection";
    const bool critical_init = critical_plain || import.name == "InitializeCriticalSectionAndSpinCount";
    const bool critical_enter = import.name == "EnterCriticalSection";
    const bool critical_try = import.name == "TryEnterCriticalSection";
    const bool critical_leave = import.name == "LeaveCriticalSection";
    const bool critical_delete = import.name == "DeleteCriticalSection";
    const bool critical_op = critical_enter || critical_try || critical_leave || critical_delete;
    const bool tls_alloc = import.name == "TlsAlloc";
    const bool tls_get = import.name == "TlsGetValue";
    const bool tls_set = import.name == "TlsSetValue";
    const bool tls_free = import.name == "TlsFree";
    const bool tls = tls_alloc || tls_get || tls_set || tls_free;
    const bool get_error = import.name == "GetLastError";
    const bool set_error = import.name == "SetLastError";
    const bool thread_id = import.name == "GetCurrentThreadId";
    const bool process_id = import.name == "GetCurrentProcessId";
    const bool file_time = import.name == "GetSystemTimeAsFileTime";
    const bool tick_count = import.name == "GetTickCount";
    const bool performance_counter = import.name == "QueryPerformanceCounter";
    const bool performance_frequency = import.name == "QueryPerformanceFrequency";
    const bool clock = file_time || tick_count || performance_counter || performance_frequency;
    const bool startup_info = import.name == "GetStartupInfoA";
    const bool command_line = import.name == "GetCommandLineA";
    const bool module_filename = import.name == "GetModuleFileNameA";
    const bool processor_feature = import.name == "IsProcessorFeaturePresent";
    const bool exception_filter = import.name == "SetUnhandledExceptionFilter";
    const bool std_handle = import.name == "GetStdHandle";
    const bool file_type = import.name == "GetFileType";
    const bool file_open = import.name == "CreateFileA";
    const bool file_size = import.name == "GetFileSize";
    const bool file_read = import.name == "ReadFile";
    const bool file_write = import.name == "WriteFile";
    const bool file_seek = import.name == "SetFilePointer";
    const bool file_attributes = import.name == "GetFileAttributesA";
    const bool file_io = file_open || file_size || file_read || file_write || file_seek || file_attributes;
    const bool handle_count = import.name == "SetHandleCount";
    const bool environment_get = import.name == "GetEnvironmentStringsW" ||
        import.name == "GetEnvironmentStringsA" || import.name == "GetEnvironmentStrings";
    const bool environment_free = import.name == "FreeEnvironmentStringsW" ||
        import.name == "FreeEnvironmentStringsA";
    const bool environment = environment_get || environment_free;
    const bool wide_to_bytes = import.name == "WideCharToMultiByte";
    const bool bytes_to_wide = import.name == "MultiByteToWideChar";
    const bool code_page_query = import.name == "GetACP" || import.name == "GetOEMCP";
    const bool code_page_info = import.name == "GetCPInfo";
    const bool string_type = import.name == "GetStringTypeW";
    const bool case_map = import.name == "LCMapStringW";
    const bool process = startup_info || command_line || std_handle || file_type || handle_count || module_filename || processor_feature || exception_filter;
    const bool heap_create = import.name == "HeapCreate";
    const bool heap_alloc = import.name == "HeapAlloc";
    const bool heap_free = import.name == "HeapFree";
    const bool heap_size = import.name == "HeapSize";
    const bool heap_realloc = import.name == "HeapReAlloc";
    const bool event_create = import.name == "CreateEventA";
    const bool event_set = import.name == "SetEvent";
    const bool event_reset = import.name == "ResetEvent";
    const bool event_close = import.name == "CloseHandle";
    const bool event_wait = import.name == "WaitForSingleObject";
    const bool event = event_create || event_set || event_reset || event_close || event_wait;
    const bool priority_set = import.name == "SetThreadPriority";
    const bool priority_get = import.name == "GetThreadPriority";
    const bool priority = priority_set || priority_get;
    if (!version && !module && !proc_address && !critical_init && !critical_op && !tls &&
        !get_error && !set_error && !thread_id && !process_id && !clock && !process && !environment && !wide_to_bytes &&
        !code_page_query && !code_page_info && !string_type && !bytes_to_wide && !case_map &&
        !heap_create && !heap_alloc && !heap_free && !heap_size && !heap_realloc && !multimedia_timer && !event && !priority && !cwd && !metrics && !gui && !com && !file_io)
        return StartupServiceResult::Unsupported;
    const uint32_t esp = cpu_.reg(d2rt::R_ESP);
    const int preserved[] = {d2rt::R_EBX, d2rt::R_EBP, d2rt::R_ESI, d2rt::R_EDI};
    uint32_t before[4];
    for (unsigned i = 0; i < 4; ++i) before[i] = cpu_.reg(preserved[i]);
    uint32_t ret = 0, arg = 0;
    uint32_t expected_eax = 0;
    const uint32_t parameter_count = window_create ? 12u : wide_to_bytes ? 8u : file_open ? 7u : (bytes_to_wide || case_map) ? 6u : (file_read || file_write) ? 5u : (file_seek || window_default || string_type || heap_realloc || event_create) ? 4u :
        ((com_uninit || tls_alloc || get_error || thread_id || process_id || tick_count || timer_time || command_line || environment_get || code_page_query) ? 0u :
        ((file_attributes || com_init || window_update || class_register || stock || cwd_set || metrics || version || module || tls_get || tls_free || set_error || priority_get || event_set || event_reset || event_close || timer_begin || timer_end || file_time || performance_counter || performance_frequency || startup_info || std_handle || file_type || handle_count || environment_free || critical_plain || critical_op || processor_feature || exception_filter) ? 1u :
        ((file_size || window_show || icon_load || cursor_load || cwd_get || proc_address || critical_init || tls_set || code_page_info || event_wait || priority_set) ? 2u : 3u)));
    const uint32_t cleanup = 4u * (1u + parameter_count);
    if (!in_stack(esp, cleanup) || !cpu_.read(esp, &ret, 4) ||
        (parameter_count && !cpu_.read(esp + 4, &arg, 4))) {
        fprintf(log_, "startup_service_error=invalid_call_frame\n");
        return StartupServiceResult::ContractFailure;
    }
    if(file_io || (event_close && files_.count(arg)) || (file_type && files_.count(arg))) {
        auto path_for=[&](uint32_t pointer,std::string& native) {
            std::string path;bool ended=false;
            for(unsigned i=0;i<260;++i) {char ch=0;
                if(uint64_t(pointer)+i>0xFFFFFFFFull || !cpu_.read(pointer+i,&ch,1))return false;
                if(!ch) {ended=true;break;}path.push_back(ch=='\\' ? '/':ch);
            }
            if(!ended || path.empty())return false;
            if(path.rfind("C:/TH075/",0)==0)path.erase(0,9);
            if(path.rfind("./",0)==0)path.erase(0,2);
            if(path.empty() || path.front()=='/' || path.find(':')!=std::string::npos)return false;
            for(size_t begin=0;begin<path.size();) {
                size_t end=path.find('/',begin);if(end==std::string::npos)end=path.size();
                const auto part=path.substr(begin,end-begin);
                if(part.empty() || part=="." || part=="..")return false;
                begin=end+1;
            }
            native="ux0:data/TH075Vita/"+path;
            fprintf(log_,"startup_file_guest=%s\n",path.c_str());return true;
        };
        if(file_open || file_attributes) {
            std::string native;
            expected_eax=0xFFFFFFFFu;
            if(!path_for(arg,native))wx86_set_lasterr(cpu_,123);
            else if(file_attributes) {
                SceIoStat info{};
                if(sceIoGetstat(native.c_str(),&info)<0)wx86_set_lasterr(cpu_,2);
                else expected_eax=SCE_S_ISDIR(info.st_mode) ? 0x10u:0x20u;
            } else {
                uint32_t args[7]{};
                if(!cpu_.read(esp+4,args,28))return StartupServiceResult::ContractFailure;
                fprintf(log_,"startup_file_open_access=0x%08X disposition=%u share=%u flags=0x%08X\n",args[1],args[4],args[2],args[5]);
                const bool readonly=args[1]==0x80000000u && args[4]==3;
                const bool game_log=args[1]==0x40000000u && args[2]==0 &&
                    (args[4]==2 || args[4]==3) && args[5]==0x80 && native=="ux0:data/TH075Vita/log.txt";
                if((!readonly && !game_log) || args[2]>7 || args[3] ||
                   (args[5]&~0x08000080u) || args[6])return StartupServiceResult::Unsupported;
                if(files_.size()>=32)wx86_set_lasterr(cpu_,4);
                else {
                    SceIoStat info{};
                    const bool existed=sceIoGetstat(native.c_str(),&info)>=0;
                    FILE* file=fopen(native.c_str(),readonly ? "rb" : args[4]==2 ? "wb" : "r+b");
                    if(!file)wx86_set_lasterr(cpu_,existed || args[4]==2 ? 5:2);
                    else {expected_eax=next_file_handle_;next_file_handle_+=4;files_.emplace(expected_eax,file);
                        if(readonly) {
                            auto& buffer=readonly_file_buffers_[expected_eax];
                            const int buffered=setvbuf(file,buffer.data(),_IOFBF,buffer.size());
                            fprintf(log_,"startup_file_read_buffer=handle:0x%08X bytes:%u rc:%d\n",expected_eax,unsigned(buffer.size()),buffered);
                        }
                        if(game_log) {
                            writable_files_.insert(expected_eax);
                            if(args[4]==2)wx86_set_lasterr(cpu_,existed ? 183:0);
                            fprintf(log_,"startup_file_write_scope=original_game_log_txt\n");
                        }
                    }
                }
            }
        } else {
            uint32_t write_args[5]{};
            if(file_write) {
                uint32_t zero=0;
                if(!cpu_.read(esp+4,write_args,20))return StartupServiceResult::ContractFailure;
                if(write_args[4] || write_args[2]>4u*1024u*1024u)return StartupServiceResult::Unsupported;
                if(!write_args[3] || !cpu_.write(write_args[3],&zero,4))return StartupServiceResult::ContractFailure;
            }
            const auto it=files_.find(arg);
            if(it==files_.end()) {wx86_set_lasterr(cpu_,6);expected_eax=file_size || file_seek ? 0xFFFFFFFFu:0u;}
            else if(event_close) {
                const int rc=fclose(it->second);readonly_file_buffers_.erase(arg);writable_files_.erase(arg);files_.erase(it);expected_eax=rc==0 ? 1u:0u;
                if(rc)wx86_set_lasterr(cpu_,5);
            } else if(file_type)expected_eax=1;
            else if(file_size) {
                uint32_t high_pointer=0,zero=0;
                if(!cpu_.read(esp+8,&high_pointer,4) || (high_pointer && !cpu_.write(high_pointer,&zero,4)))
                    return StartupServiceResult::ContractFailure;
                const long position=ftell(it->second);
                if(position<0 || fseek(it->second,0,SEEK_END))return StartupServiceResult::ContractFailure;
                const long size=ftell(it->second);
                if(size<0 || fseek(it->second,position,SEEK_SET))return StartupServiceResult::ContractFailure;
                expected_eax=uint32_t(size);
            } else if(file_seek) {
                uint32_t args[4]{};int32_t high=0;
                if(!cpu_.read(esp+4,args,16) || (args[2] && !cpu_.read(args[2],&high,4)))
                    return StartupServiceResult::ContractFailure;
                if(high || args[3]>2)return StartupServiceResult::Unsupported;
                expected_eax=0xFFFFFFFFu;
                const int origins[3]={SEEK_SET,SEEK_CUR,SEEK_END};
                if(fseek(it->second,int32_t(args[1]),origins[args[3]]))wx86_set_lasterr(cpu_,131);
                else {const long position=ftell(it->second);
                    if(position<0)wx86_set_lasterr(cpu_,131);
                    else {expected_eax=uint32_t(position);uint32_t zero=0;
                        if(args[2] && !cpu_.write(args[2],&zero,4))return StartupServiceResult::ContractFailure;
                        wx86_set_lasterr(cpu_,0);
                    }
                }
            } else if(file_write) {
                if(!writable_files_.count(arg)) {expected_eax=0;wx86_set_lasterr(cpu_,5);}
                else {
                    std::vector<uint8_t> bytes(write_args[2]);
                    if(uint64_t(write_args[1])+write_args[2]>0x02000000ull ||
                        (write_args[2] && !cpu_.read(write_args[1],bytes.data(),write_args[2])))
                        return StartupServiceResult::ContractFailure;
                    const uint32_t written=bytes.empty() ? 0u:uint32_t(fwrite(bytes.data(),1,bytes.size(),it->second));
                    const int flush=fflush(it->second);
                    if(!cpu_.write(write_args[3],&written,4))return StartupServiceResult::ContractFailure;
                    expected_eax=written==write_args[2] && flush==0 && !ferror(it->second) ? 1u:0u;
                    if(!expected_eax)wx86_set_lasterr(cpu_,29);
                    fprintf(log_,"startup_file_write_bytes=%u requested=%u flushed=%s\n",written,write_args[2],flush==0 ? "yes":"no");
                }
            } else if(file_read && writable_files_.count(arg)) {expected_eax=0;wx86_set_lasterr(cpu_,5);
            } else if(file_read) {
                uint32_t args[5]{};
                if(!cpu_.read(esp+4,args,20))return StartupServiceResult::ContractFailure;
                if(args[4] || args[2]>4u*1024u*1024u)return StartupServiceResult::Unsupported;
                if(!args[3] || uint64_t(args[1])+args[2]>0x02000000ull)return StartupServiceResult::ContractFailure;
                uint32_t total=0;
                if(!cpu_.write(args[3],&total,4))return StartupServiceResult::ContractFailure;
                std::array<uint8_t,4096> buffer{};
                while(total<args[2]) {
                    const size_t wanted=std::min<size_t>(buffer.size(),args[2]-total);
                    const size_t got=fread(buffer.data(),1,wanted,it->second);
                    if(got && !cpu_.write(args[1]+total,buffer.data(),uint32_t(got)))return StartupServiceResult::ContractFailure;
                    total+=uint32_t(got);if(got<wanted)break;
                }
                if(!cpu_.write(args[3],&total,4))return StartupServiceResult::ContractFailure;
                expected_eax=ferror(it->second) ? 0u:1u;
                if(!expected_eax)wx86_set_lasterr(cpu_,30);
                fprintf(log_,"startup_file_read_bytes=%u\n",total);
            }
        }
        ++process_calls_;cpu_.trap_epilogue(expected_eax,cleanup,ret);
        fprintf(log_,"startup_file_scope=owned_application_mount_readonly_assets_writable_log\n");
        fprintf(log_,"startup_serviced_import=KERNEL32.dll!%s\n",import.name.c_str());
    } else if(com) {
        const uint32_t tib=wx86_cur_tib();
        if(com_init) {
            if(arg)expected_eax=0x80070057u;
            else {auto& references=com_apartments_[tib];expected_eax=references ? 1u:0u;++references;}
        } else {
            auto it=com_apartments_.find(tib);
            if(it!=com_apartments_.end() && !--it->second)com_apartments_.erase(it);
        }
        fprintf(log_,"startup_com_tib=0x%08X\nstartup_com_scope=owned_sta_registration_no_servers\n",tib);
        ++process_calls_;cpu_.trap_epilogue(expected_eax,cleanup,ret);
        fprintf(log_,"startup_serviced_import=ole32.dll!%s\n",import.name.c_str());
    } else if(gui) {
        auto guest_string=[&](uint32_t pointer,std::string& text) {
            text.clear();if(!pointer)return false;
            for(unsigned i=0;i<128;++i) {char ch=0;
                if(uint64_t(pointer)+i>0xFFFFFFFFull || !cpu_.read(pointer+i,&ch,1))return false;
                if(!ch)return !text.empty();
                text.push_back(ch);
            }
            return false;
        };
        if(window_update) {
            if(!window_.created || arg!=window_.handle || window_pending_)return StartupServiceResult::Unsupported;
            if(!window_.invalidated)expected_eax=1;
            else {
                window_.frame=esp;window_.return_address=ret;
                window_paint_pending_=true;window_pending_=true;
                fprintf(log_,"startup_window_update=deferred_original_wm_paint\n");
                return StartupServiceResult::Deferred;
            }
        } else if(window_show) {
            uint32_t command=0;
            if(!cpu_.read(esp+8,&command,4))return StartupServiceResult::ContractFailure;
            // SW_SHOWDEFAULT resolves to normal in our STARTUPINFO profile
            // (no STARTF_USESHOWWINDOW override).
            if(!window_.created || arg!=window_.handle || window_pending_ || (command!=1 && command!=5 && command!=10))
                return StartupServiceResult::Unsupported;
            if(window_.visible)expected_eax=1;
            else {
                window_.frame=esp;window_.return_address=ret;
                window_was_visible_=window_.visible;window_show_pending_=true;window_pending_=true;
                fprintf(log_,"startup_window_show_command=%u\n",command);
                return StartupServiceResult::Deferred;
            }
        } else if(window_create) {
            if(window_pending_ || window_.handle)return StartupServiceResult::Unsupported;
            std::array<uint32_t,12> args{};
            if(!cpu_.read(esp+4,args.data(),48))return StartupServiceResult::ContractFailure;
            std::string name,title;
            auto cls=window_classes_.end();
            if(args[1]<=0xFFFF) {
                for(auto it=window_classes_.begin();it!=window_classes_.end();++it)
                    if(it->second.atom==args[1]) {cls=it;break;}
            } else {
                if(!guest_string(args[1],name))return StartupServiceResult::ContractFailure;
                cls=window_classes_.find(name);
            }
            if(cls==window_classes_.end()) {
                wx86_set_lasterr(cpu_,1407);
                cpu_.trap_epilogue(0,cleanup,ret);
                return StartupServiceResult::Serviced;
            }
            if(!guest_string(args[2],title))return StartupServiceResult::ContractFailure;
            // First observed top-level, hidden window in the borderless desktop.
            if(args[0]!=0x40000 || args[3]!=0xC80000 || args[4]!=0x80000000 ||
               args[5]!=0x80000000 || args[6]!=640 || args[7]!=480 || args[8] || args[9] ||
               args[10]!=cls->second.fields[5] || args[11])return StartupServiceResult::Unsupported;
            window_.handle=0x00AB7000;window_.procedure=cls->second.fields[2];
            window_.frame=esp;window_.return_address=ret;window_.args=args;
            window_.surface.assign(640u*480u,0xFFFFFFFFu);
            window_pending_=true;
            fprintf(log_,"startup_window_creation=deferred_guest_callbacks\n");
            fprintf(log_,"startup_window_surface=owned_640x480_abgr_rendering_pending\n");
            return StartupServiceResult::Deferred;
        } else if(window_default) {
            uint32_t params[4]{};
            if(!cpu_.read(esp+4,params,16))return StartupServiceResult::ContractFailure;
            if(!window_pending_ || params[0]!=window_.handle || params[1]!=window_callback_message_ ||
               params[2]!=window_callback_wparam_ || params[3]!=window_callback_parameter_)return StartupServiceResult::Unsupported;
            if(params[1]==0x81)expected_eax=1;
            else if(params[1]==0x83) {
                const uint32_t rectangle[4]={0,0,640,480};
                if(!cpu_.write(params[3],rectangle,16))return StartupServiceResult::ContractFailure;
            } else if(params[1]==0x0F) {
                if(!window_paint_pending_)return StartupServiceResult::Unsupported;
                std::fill(window_.surface.begin(),window_.surface.end(),0xFFFFFFFFu);
                window_.invalidated=false;
                fprintf(log_,"startup_window_paint=owned_white_brush_surface_validated\n");
            } else if(params[1]!=0x18 && params[1]!=5 && params[1]!=3) return StartupServiceResult::Unsupported;
            fprintf(log_,"startup_window_default_message=0x%04X\n",params[1]);
        } else if(icon_load) {
            uint32_t identifier=0;
            if(!cpu_.read(esp+8,&identifier,4))return StartupServiceResult::ContractFailure;
            if(arg!=image_.load_base())return StartupServiceResult::Unsupported;
            std::string name;
            if(identifier>0xFFFF && !guest_string(identifier,name))return StartupServiceResult::ContractFailure;
            const std::string key=name.empty() ? "#"+std::to_string(identifier):name;
            fprintf(log_,"startup_icon_requested=%s\n",key.c_str());
            const auto existing=icons_.find(key);
            if(existing!=icons_.end())expected_eax=existing->second.handle;
            else {
                th075::ResourceBlob group{},bitmap{};
                const auto& bytes=image_.image();
                if(!th075::resource(bytes,14,name,uint16_t(identifier),group)) {
                    wx86_set_lasterr(cpu_,1814);
                    fprintf(log_,"startup_icon_error=group_missing_or_invalid\n");
                } else {
                    const uint8_t* data=bytes.data()+group.rva;uint16_t reserved=0,type=0,count=0;
                    if(group.size<6)return StartupServiceResult::ContractFailure;
                    std::memcpy(&reserved,data,2);std::memcpy(&type,data+2,2);std::memcpy(&count,data+4,2);
                    if(reserved || type!=1 || !count || count>64 || group.size!=6u+14u*count)
                        return StartupServiceResult::ContractFailure;
                    uint16_t id=0;uint32_t declared=0;
                    for(unsigned i=0;i<count;++i) {
                        const uint8_t* entry=data+6+14*i;uint16_t bpp=0;
                        std::memcpy(&bpp,entry+6,2);
                        if(entry[0]==32 && entry[1]==32 && bpp==8) {
                            std::memcpy(&declared,entry+8,4);std::memcpy(&id,entry+12,2);break;
                        }
                    }
                    if(!id || !th075::resource(bytes,3,"",id,bitmap) || bitmap.size!=declared)
                        return StartupServiceResult::Unsupported;
                    std::vector<uint32_t> pixels;
                    if(!th075::decode_icon(bytes,bitmap,pixels))return StartupServiceResult::Unsupported;
                    if(icons_.size()>=16)return StartupServiceResult::Unsupported;
                    expected_eax=0x00AB5000+4*uint32_t(icons_.size());
                    icons_.emplace(key,IconObject{expected_eax,pixels});
                    th075::icon_preview=pixels;
                    fprintf(log_,"startup_icon_resource_id=%u\nstartup_icon_language=%u\n",id,bitmap.language);
                    fprintf(log_,"startup_icon_decoded_pixels=%u\nstartup_icon_resource=validated_and_decoded\n",unsigned(pixels.size()));
                }
            }
            fprintf(log_,"startup_icon_handle=0x%08X\n",expected_eax);
        } else if(cursor_load) {
            uint32_t id=0;if(!cpu_.read(esp+8,&id,4))return StartupServiceResult::ContractFailure;
            if(arg || id!=32512)return StartupServiceResult::Unsupported;
            if(arrow_cursor_.empty()) {
                arrow_cursor_.resize(1024,0);
                for(unsigned y=0;y<20;++y)for(unsigned x=0;x<=y/2;++x)
                    arrow_cursor_[y*32+x]=(x==0 || x==y/2 || y==19) ? 0xFF000000:0xFFFFFFFF;
            }
            expected_eax=0x00AB5800;
            fprintf(log_,"startup_cursor_object=bounded_system_arrow_32x32_hotspot_0_0\n");
        } else if(stock) {
            if(arg!=0)return StartupServiceResult::Unsupported;
            expected_eax=0x00AB6000;
            fprintf(log_,"startup_stock_object=white_solid_brush\n");
        } else {
            std::array<uint32_t,12> fields{};std::string name;
            if(!cpu_.read(arg,fields.data(),sizeof(fields)) || fields[0]!=48 ||
               !guest_string(fields[10],name))return StartupServiceResult::ContractFailure;
            uint8_t first=0;
            const bool procedure=fields[2]>=image_.load_base() &&
                uint64_t(fields[2])<uint64_t(image_.load_base())+image_.image_size() && cpu_.read(fields[2],&first,1);
            auto valid_icon=[&](uint32_t handle) {
                if(!handle)return true;
                for(const auto& icon:icons_)if(icon.second.handle==handle)return true;
                return false;
            };
            if(!procedure || fields[3] || fields[4] || fields[5]!=image_.load_base() ||
               !valid_icon(fields[6]) || !valid_icon(fields[11]) ||
               fields[7]!=0x00AB5800 || arrow_cursor_.empty() || fields[8]!=0x00AB6000 || fields[9])
                return StartupServiceResult::Unsupported;
            if(window_classes_.count(name))wx86_set_lasterr(cpu_,1410);
            else {
                if(window_classes_.size()>=16)return StartupServiceResult::Unsupported;
                expected_eax=0xC001+uint32_t(window_classes_.size());
                window_classes_.emplace(name,WindowClass{uint16_t(expected_eax),fields});
                fprintf(log_,"startup_window_class_registered=yes\nstartup_window_class_atom=0x%04X\n",expected_eax);
                fprintf(log_,"startup_window_procedure=0x%08X\nstartup_window_class_scope=guest_registry_window_creation_pending\n",fields[2]);
            }
        }
        ++process_calls_;cpu_.trap_epilogue(expected_eax,cleanup,ret);
        fprintf(log_,"startup_serviced_import=%s!%s\n",import.dll.c_str(),import.name.c_str());
    } else if (cwd) {
        // The only mounted guest directory is the application's data directory.
        // Never change the native process CWD or accept an unmounted guest path.
        constexpr char directory[] = "C:\\TH075";
        if(cwd_set) {
            if(!arg) { wx86_set_lasterr(cpu_,87); }
            else {
                char path[260] = {}; bool terminated=false;
                for(unsigned i=0;i<sizeof(path);++i) {
                    if(uint64_t(arg)+i>0xFFFFFFFFull || !cpu_.read(arg+i,path+i,1))
                        return StartupServiceResult::ContractFailure;
                    if(!path[i]) {terminated=true;break;}
                }
                if(!terminated) { wx86_set_lasterr(cpu_,206); }
                else {
                    std::string normalized(path);
                    for(char& ch:normalized) {
                        if(ch=='/')ch='\\';
                        if(ch>='a' && ch<='z')ch=char(ch-'a'+'A');
                    }
                    if(normalized.size()>3 && normalized.back()=='\\')normalized.pop_back();
                    fprintf(log_,"startup_cwd_requested=%s\n",path);
                    if(normalized!=directory && normalized!=".") {
                        fprintf(log_,"startup_cwd_boundary=unmounted_path\n");
                        return StartupServiceResult::Unsupported;
                    }
                    SceIoStat native{};
                    const int rc=sceIoGetstat("ux0:data/TH075Vita",&native);
                    if(rc<0 || !SCE_S_ISDIR(native.st_mode)) wx86_set_lasterr(cpu_,3);
                    else expected_eax=1;
                    fprintf(log_,"startup_cwd_native_stat_rc=0x%08X\n",unsigned(rc));
                }
            }
        } else {
            uint32_t destination=0;
            if(!cpu_.read(esp+8,&destination,4))return StartupServiceResult::ContractFailure;
            expected_eax=sizeof(directory);
            if(arg>=sizeof(directory)) {
                std::array<char,sizeof(directory)> check{};
                if(!cpu_.write(destination,directory,sizeof(directory)) ||
                   !cpu_.read(destination,check.data(),check.size()) ||
                   std::memcmp(check.data(),directory,sizeof(directory)))
                    return StartupServiceResult::ContractFailure;
                expected_eax=sizeof(directory)-1;
            }
        }
        fprintf(log_,"startup_cwd_guest=C:\\TH075\nstartup_cwd_native=ux0:data/TH075Vita\n");
        fprintf(log_,"startup_cwd_scope=single_mounted_directory\n");
        ++process_calls_;
        cpu_.trap_epilogue(expected_eax,cleanup,ret);
        fprintf(log_,"startup_serviced_import=KERNEL32.dll!%s\n",import.name.c_str());
    } else if(metrics) {
        // Logical game desktop, borderless. Actual rendering remains pending.
        switch(arg) {
            case 0: expected_eax=640;break;
            case 1: expected_eax=480;break;
            case 11: case 12: case 13: case 14: expected_eax=32;break;
            case 4: case 5: case 6: case 7: case 8: case 45: case 46: expected_eax=0;break;
            default:
                fprintf(log_,"startup_metrics_boundary=unimplemented_index_%u\n",arg);
                return StartupServiceResult::Unsupported;
        }
        fprintf(log_,"startup_metrics_index=%u\nstartup_metrics_value=%u\n",arg,expected_eax);
        fprintf(log_,"startup_metrics_profile=logical_640x480_borderless_rendering_pending\n");
        ++process_calls_;
        cpu_.trap_epilogue(expected_eax,cleanup,ret);
        fprintf(log_,"startup_serviced_import=USER32.dll!GetSystemMetrics\n");
    } else if (priority) {
        StartupWorker* selected=worker_ && arg==worker_->handle?worker_:audio_worker_;
        if (!selected || arg != selected->handle) {
            wx86_set_lasterr(cpu_,6);
            expected_eax = priority_get ? 0x7FFFFFFFu : 0;
        } else if (priority_get) expected_eax = uint32_t(selected->priority);
        else {
            int32_t requested=0;
            if(!cpu_.read(esp+8,&requested,4)) return StartupServiceResult::ContractFailure;
            if(requested != -15 && requested != 15 && (requested < -2 || requested > 2)) {
                wx86_set_lasterr(cpu_,87); expected_eax=0;
            } else { selected->priority=requested; expected_eax=1; }
            fprintf(log_,"startup_thread_requested_priority=%d\n",requested);
        }
        if(selected) fprintf(log_,"startup_thread_priority=%d\n",selected->priority);
        fprintf(log_,"startup_priority_scope=bounded_guest_worker_dispatch\n");
        cpu_.trap_epilogue(expected_eax,cleanup,ret);
        ++priority_calls_;
        fprintf(log_,"startup_serviced_import=KERNEL32.dll!%s\n",import.name.c_str());
    } else if (event) {
        if (event_create) {
            uint32_t args[4] = {};
            if (!cpu_.read(esp + 4, args, sizeof(args))) return StartupServiceResult::ContractFailure;
            fprintf(log_, "startup_event_security_va=0x%08X\n", args[0]);
            fprintf(log_, "startup_event_manual_reset=%s\n", args[1] ? "yes" : "no");
            fprintf(log_, "startup_event_initial_signaled=%s\n", args[2] ? "yes" : "no");
            fprintf(log_, "startup_event_name_va=0x%08X\n", args[3]);
            if (args[0] || args[3]) {
                fprintf(log_, "startup_event_boundary=security_or_named_event\n");
                return StartupServiceResult::Unsupported;
            }
            if (events_.size() >= 64 || next_event_handle_ >= 0x00AB3000) {
                wx86_set_lasterr(cpu_, 8);
            } else {
                expected_eax = next_event_handle_;
                next_event_handle_ += 4; // Never recycle stale handles in this run.
                events_.emplace(expected_eax, EventState{args[1] != 0, args[2] != 0});
                fprintf(log_, "startup_event_handle=0x%08X\n", expected_eax);
                fprintf(log_, "startup_event_handle_kind=tracked_unnamed_event\n");
            }
        } else {
            const auto found = events_.find(arg);
            uint32_t timeout = 0;
            if (event_wait && !cpu_.read(esp + 8, &timeout, 4)) return StartupServiceResult::ContractFailure;
            fprintf(log_, "startup_event_handle=0x%08X\n", arg);
            if (found == events_.end()) {
                // Handles from other subsystems need their own wait/close dispatch.
                if (arg < 0x00AB2000 || arg >= next_event_handle_ || (arg & 3)) {
                    fprintf(log_, "startup_event_boundary=foreign_handle_type\n");
                    return StartupServiceResult::Unsupported;
                }
                wx86_set_lasterr(cpu_, 6);
                expected_eax = event_wait ? 0xFFFFFFFFu : 0;
                fprintf(log_, "startup_event_error=closed_handle\n");
            } else if (event_wait) {
                fprintf(log_, "startup_event_wait_timeout_ms=%u\n", timeout);
                if (found->second.signaled) {
                    expected_eax = 0; // WAIT_OBJECT_0
                    if (!found->second.manual_reset) found->second.signaled = false;
                } else if (!timeout) {
                    expected_eax = 0x102; // Immediate WAIT_TIMEOUT.
                } else {
                    fprintf(log_, "startup_event_boundary=blocking_wait_requires_guest_scheduler\n");
                    return StartupServiceResult::Unsupported;
                }
                fprintf(log_, "startup_event_wait_result=0x%08X\n", expected_eax);
            } else if (event_close) {
                events_.erase(found);
                expected_eax = 1;
                fprintf(log_, "startup_event_closed=yes\n");
            } else {
                found->second.signaled = event_set;
                expected_eax = 1;
            }
            const auto current = events_.find(arg);
            if (current != events_.end())
                fprintf(log_, "startup_event_signaled=%s\n", current->second.signaled ? "yes" : "no");
        }
        fprintf(log_, "startup_event_live_count=%u\n", unsigned(events_.size()));
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++event_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (multimedia_timer) {
        if (timer_time) {
            expected_eax = uint32_t(sceKernelGetProcessTimeWide() / 1000);
            fprintf(log_, "startup_timer_time_ms=%u\n", expected_eax);
            fprintf(log_, "startup_timer_clock_source=vita_process_uptime\n");
        } else {
            fprintf(log_, "startup_timer_requested_period_ms=%u\n", arg);
            // Bounded profile for the observed 1 ms request. The existing
            // microsecond clock needs no resolution switch; this does not
            // implement periodic callbacks or certify scheduling precision.
            if (arg != 1) {
                fprintf(log_, "startup_timer_boundary=unsupported_period\n");
                return StartupServiceResult::Unsupported;
            }
            if (timer_begin) {
                if (timer_period_requests_ == 0xFFFFFFFFu)
                    return StartupServiceResult::ContractFailure;
                ++timer_period_requests_;
            } else {
                if (!timer_period_requests_) {
                    fprintf(log_, "startup_service_error=unmatched_timer_period_release\n");
                    return StartupServiceResult::ContractFailure;
                }
                --timer_period_requests_;
            }
            fprintf(log_, "startup_timer_period_requests=%u\n", timer_period_requests_);
            fprintf(log_, "startup_timer_profile=1ms_clock_no_periodic_callbacks\n");
            expected_eax = 0; // TIMERR_NOERROR for the supported period.
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++multimedia_calls_;
        fprintf(log_, "startup_serviced_import=WINMM.dll!%s\n", import.name.c_str());
    } else if (case_map) {
        uint32_t args[6] = {};
        if (!cpu_.read(esp + 4, args, sizeof(args))) return StartupServiceResult::ContractFailure;
        const uint32_t locale = args[0], flags = args[1], source = args[2];
        const uint32_t count = args[3], dest = args[4], capacity = args[5];
        fprintf(log_, "startup_case_locale=0x%08X\n", locale);
        fprintf(log_, "startup_case_flags=0x%08X\n", flags);
        fprintf(log_, "startup_case_input_units=%u\n", count);
        // Explicit Japanese profile, including default locale aliases and CRT probe 0.
        if (locale != 0 && locale != 0x411 && locale != 0x400 && locale != 0x800) {
            fprintf(log_, "startup_case_boundary=unsupported_locale\n");
            return StartupServiceResult::Unsupported;
        }
        if (flags != 0x100 && flags != 0x200) {
            fprintf(log_, "startup_case_boundary=unsupported_mapping_flags\n");
            return StartupServiceResult::Unsupported;
        }
        uint32_t error = (!source || !count || capacity > 0x7FFFFFFFu || (capacity && !dest)) ? 87 : 0;
        std::vector<uint16_t> output;
        if (!error) {
            const bool terminated_input = count > 0x7FFFFFFFu;
            constexpr uint32_t max_units = 16384;
            if (!terminated_input && count > max_units) return StartupServiceResult::Unsupported;
            const uint32_t limit = terminated_input ? max_units : count;
            bool terminated = false;
            for (uint32_t i = 0; i < limit; ++i) {
                uint16_t unit = 0;
                if (uint64_t(source) + 2ull*i + 2 > 0x100000000ull ||
                    !cpu_.read(source + 2*i, &unit, 2)) return StartupServiceResult::ContractFailure;
                const auto* entry = cp932_entry(unit);
                const uint16_t mapped = entry ? (flags == 0x100 ? entry->lower : entry->upper) : 0xFFFF;
                if (mapped == 0xFFFF) {
                    fprintf(log_, "startup_case_boundary=unmapped_or_expanding_character\n");
                    fprintf(log_, "startup_case_unsupported_unit=0x%04X\n", unsigned(unit));
                    return StartupServiceResult::Unsupported;
                }
                output.push_back(mapped);
                if (!unit && terminated_input) { terminated = true; break; }
            }
            if (terminated_input && !terminated) return StartupServiceResult::Unsupported;
            if (capacity && capacity < output.size()) error = 122;
            const uint32_t size = uint32_t(output.size()) * 2;
            if (!error && capacity) {
                if (uint64_t(dest) + size > 0x100000000ull ||
                    (dest != source && uint64_t(source) < uint64_t(dest) + size && uint64_t(dest) < uint64_t(source) + size)) error = 87;
                else {
                    std::vector<uint16_t> readback(output.size());
                    if (!cpu_.read(dest, readback.data(), size) ||
                        !cpu_.write(dest, output.data(), size) ||
                        !cpu_.read(dest, readback.data(), size) || readback != output)
                        return StartupServiceResult::ContractFailure;
                    fprintf(log_, "startup_case_readback=passed\n");
                }
            }
            if (!error) expected_eax = uint32_t(output.size());
        }
        if (error) wx86_set_lasterr(cpu_, error);
        fprintf(log_, "startup_case_error=%u\n", error);
        fprintf(log_, "startup_case_mode=%s\n", capacity ? "write" : "size_query");
        fprintf(log_, "startup_case_output_units=%u\n", expected_eax);
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++case_map_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!LCMapStringW\n");
    } else if (bytes_to_wide) {
        uint32_t args[6] = {};
        if (!cpu_.read(esp + 4, args, sizeof(args))) return StartupServiceResult::ContractFailure;
        const uint32_t page = args[0], flags = args[1], source = args[2];
        const uint32_t count = args[3], dest = args[4], capacity = args[5];
        fprintf(log_, "startup_decode_code_page=%u\n", page);
        fprintf(log_, "startup_decode_flags=0x%08X\n", flags);
        fprintf(log_, "startup_decode_input_bytes=%u\n", count);
        if (page != 0 && page != 1 && page != 3 && page != 932) return StartupServiceResult::Unsupported;
        if (flags & 6u) { // Composite and glyph modes need separate mapping data.
            fprintf(log_, "startup_decode_boundary=composite_or_glyph_mode\n");
            return StartupServiceResult::Unsupported;
        }
        uint32_t error = (flags & ~9u) ? 1004u : 0u;
        if (!source || !count || (count > 0x7FFFFFFFu && count != 0xFFFFFFFFu) ||
            capacity > 0x7FFFFFFFu || (capacity && (!dest || dest == source))) error = 87;
        std::vector<uint8_t> input;
        std::vector<uint16_t> output;
        if (!error) {
            constexpr uint32_t max_bytes = 16384;
            if (count != 0xFFFFFFFFu && count > max_bytes) return StartupServiceResult::Unsupported;
            const uint32_t limit = count == 0xFFFFFFFFu ? max_bytes : count;
            bool terminated = false;
            for (uint32_t i = 0; i < limit; ++i) {
                uint8_t byte = 0;
                if (uint64_t(source) + i + 1 > 0x100000000ull ||
                    !cpu_.read(source + i, &byte, 1)) return StartupServiceResult::ContractFailure;
                input.push_back(byte);
                if (count == 0xFFFFFFFFu && !byte) { terminated = true; break; }
            }
            if (count == 0xFFFFFFFFu && !terminated) return StartupServiceResult::Unsupported;
            for (size_t i = 0; i < input.size(); ++i) {
                const uint8_t byte = input[i];
                uint16_t unit = kCp932Single[byte];
                const bool lead = (byte >= 0x81 && byte <= 0x9F) || (byte >= 0xE0 && byte <= 0xFC);
                if (lead) {
                    unit = 0xFFFF;
                    if (i + 1 < input.size()) {
                        const uint8_t trail = input[i + 1];
                        if ((trail >= 0x40 && trail <= 0x7E) || (trail >= 0x80 && trail <= 0xFC)) {
                            const unsigned row = byte <= 0x9F ? byte - 0x81 : byte - 0xE0 + 31;
                            const unsigned col = trail <= 0x7E ? trail - 0x40 : trail - 0x80 + 63;
                            unit = kCp932Pairs[row * 188 + col];
                            if (unit != 0xFFFF) ++i;
                        }
                    }
                }
                if (unit == 0xFFFF) {
                    fprintf(log_, "startup_decode_invalid_byte_offset=%u\n", unsigned(i));
                    if (flags & 8u) { error = 1113; break; }
                    // Do not approximate XP's drop/recovery behavior on malformed sequences.
                    fprintf(log_, "startup_decode_boundary=invalid_sequence_xp_recovery_pending\n");
                    return StartupServiceResult::Unsupported;
                }
                output.push_back(unit);
            }
            if (!error && capacity && capacity < output.size()) error = 122;
            const uint32_t output_bytes = uint32_t(output.size()) * 2;
            if (!error && capacity && (uint64_t(dest) + output_bytes > 0x100000000ull ||
                (uint64_t(source) < uint64_t(dest) + output_bytes && uint64_t(dest) < uint64_t(source) + input.size()))) error = 87;
            if (!error && capacity) {
                std::vector<uint16_t> readback(output.size());
                if (!cpu_.read(dest, readback.data(), output_bytes) ||
                    !cpu_.write(dest, output.data(), output_bytes) ||
                    !cpu_.read(dest, readback.data(), output_bytes) || readback != output)
                    return StartupServiceResult::ContractFailure;
                fprintf(log_, "startup_decode_readback=passed\n");
            }
            if (!error) expected_eax = uint32_t(output.size());
        }
        if (error) wx86_set_lasterr(cpu_, error);
        fprintf(log_, "startup_decode_error=%u\n", error);
        fprintf(log_, "startup_decode_mode=%s\n", capacity ? "write" : "size_query");
        fprintf(log_, "startup_decode_output_units=%u\n", expected_eax);
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++conversion_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!MultiByteToWideChar\n");
    } else if (string_type) {
        uint32_t args[4] = {};
        if (!cpu_.read(esp + 4, args, sizeof(args))) return StartupServiceResult::ContractFailure;
        const uint32_t source = args[1], count = args[2], dest = args[3];
        fprintf(log_, "startup_string_type_kind=%u\n", args[0]);
        fprintf(log_, "startup_string_type_source_va=0x%08X\n", source);
        fprintf(log_, "startup_string_type_input_units=%u\n", count);
        if (args[0] != 1) {
            fprintf(log_, "startup_string_type_boundary=ctype2_or_ctype3_not_implemented\n");
            return StartupServiceResult::Unsupported;
        }
        if (!source || !dest || source == dest || !count) {
            wx86_set_lasterr(cpu_, 87);
        } else {
            const bool terminated_input = count > 0x7FFFFFFFu;
            constexpr uint32_t max_units = 16384;
            if (!terminated_input && count > max_units) return StartupServiceResult::Unsupported;
            const uint32_t limit = terminated_input ? max_units : count;
            std::vector<uint16_t> types;
            bool terminated = false;
            for (uint32_t i = 0; i < limit; ++i) {
                uint16_t unit = 0;
                if (uint64_t(source) + 2ull*i + 2 > 0x100000000ull ||
                    !cpu_.read(source + 2*i, &unit, 2)) return StartupServiceResult::ContractFailure;
                uint16_t type = 0;
                if (!cp932_ctype1(unit, type)) {
                    fprintf(log_, "startup_string_type_boundary=outside_cp932_repertoire\n");
                    fprintf(log_, "startup_string_type_unsupported_unit=0x%04X\n", unsigned(unit));
                    return StartupServiceResult::Unsupported;
                }
                types.push_back(type);
                if (!unit && terminated_input) { terminated = true; break; }
            }
            if (terminated_input && !terminated) return StartupServiceResult::Unsupported;
            const uint32_t size = uint32_t(types.size()) * 2;
            std::vector<uint16_t> readback(types.size());
            if (uint64_t(dest) + size > 0x100000000ull ||
                (uint64_t(source) < uint64_t(dest) + size && uint64_t(dest) < uint64_t(source) + size)) {
                wx86_set_lasterr(cpu_, 87);
            } else {
                if (!cpu_.read(dest, readback.data(), size) ||
                    !cpu_.write(dest, types.data(), size) ||
                    !cpu_.read(dest, readback.data(), size) || readback != types)
                    return StartupServiceResult::ContractFailure;
                expected_eax = 1;
                fprintf(log_, "startup_string_type_output_units=%u\n", unsigned(types.size()));
                fprintf(log_, "startup_string_type_readback=passed\n");
            }
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++string_type_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetStringTypeW\n");
    } else if (code_page_query || code_page_info) {
        // Explicit Japanese Windows compatibility profile; independent of host locale.
        constexpr uint32_t japanese_code_page = 932;
        if (code_page_query) {
            expected_eax = japanese_code_page;
            fprintf(log_, "startup_code_page_profile=japanese_cp932\n");
        } else {
            uint32_t dest = 0;
            if (!cpu_.read(esp + 8, &dest, 4)) return StartupServiceResult::ContractFailure;
            const uint32_t page = arg <= 1 ? japanese_code_page : arg;
            fprintf(log_, "startup_code_page_requested=%u\n", arg);
            if (page != japanese_code_page) {
                fprintf(log_, "startup_code_page_boundary=unsupported_page\n");
                return StartupServiceResult::Unsupported;
            }
            if (!dest) {
                wx86_set_lasterr(cpu_, 87);
                expected_eax = 0;
            } else {
                // x86 CPINFO: UINT + DefaultChar[2] + LeadByte[12] + padding.
                std::array<uint8_t, 20> info{}, readback{};
                put32(info.data(), 2);
                info[4] = '?';
                info[6] = 0x81; info[7] = 0x9F;
                info[8] = 0xE0; info[9] = 0xFC;
                if (uint64_t(dest) + info.size() > 0x100000000ull ||
                    !cpu_.read(dest, readback.data(), uint32_t(readback.size())) ||
                    !cpu_.write(dest, info.data(), uint32_t(info.size())) ||
                    !cpu_.read(dest, readback.data(), uint32_t(readback.size())) || readback != info)
                    return StartupServiceResult::ContractFailure;
                expected_eax = 1;
                fprintf(log_, "startup_code_page_info_readback=passed\n");
            }
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++code_page_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (version) {
        // This port advertises an explicit XP 5.1/2600 compatibility profile.
        // It isn't host OS detection. Only the observed 148-byte ANSI layout
        // and the first call site are admitted in this iteration.
        uint32_t size = 0;
        if (version_calls_ || module_calls_ || import.iat_va != 0x00657090 || ret != 0x00642352 ||
            !in_stack(arg, 148) || !cpu_.read(arg, &size, 4) || size != 148) {
            fprintf(log_, "startup_service_error=unsupported_version_call\n");
            return StartupServiceResult::ContractFailure;
        }
        std::array<uint8_t, 148> info{};
        put32(info.data(), 148);
        put32(info.data() + 4, kMajor);
        put32(info.data() + 8, kMinor);
        put32(info.data() + 12, kBuild);
        put32(info.data() + 16, kPlatform);
        // szCSDVersion is a fully initialized empty ANSI string (no SP).
        std::array<uint8_t, 148> readback{};
        if (!cpu_.write(arg, info.data(), uint32_t(info.size())) ||
            !cpu_.read(arg, readback.data(), uint32_t(readback.size())) || readback != info) {
            fprintf(log_, "startup_service_error=version_buffer_write_or_readback_failed\n");
            return StartupServiceResult::ContractFailure;
        }
        fprintf(log_, "startup_version_profile=winxp_5.1_2600_no_service_pack\n");
        fprintf(log_, "startup_version_info_size=148\n");
        fprintf(log_, "startup_version_buffer_va=0x%08X\n", arg);
        fprintf(log_, "startup_version_buffer_readback=passed\n");
        fprintf(log_, "startup_version_return_va=0x%08X\n", ret);
        cpu_.trap_epilogue(1, 8, ret); // BOOL TRUE; stdcall ret 4
        expected_eax = 1;
        ++version_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetVersionExA\n");
    } else if (module) {
        // The runtime implements KERNEL32 imports; identify that compatibility
        // module with an opaque handle. It is not an executable Windows DLL.
        if (arg) {
            char name[64] = {};
            unsigned n = 0;
            for (; n < sizeof(name); ++n) {
                if (uint64_t(arg) + n > 0xFFFFFFFFull || !cpu_.read(arg + n, &name[n], 1))
                    return StartupServiceResult::ContractFailure;
                if (!name[n]) break;
                if (name[n] >= 'A' && name[n] <= 'Z') name[n] += 'a' - 'A';
            }
            if (n == sizeof(name)) return StartupServiceResult::ContractFailure;
            fprintf(log_, "startup_module_requested=%s\n", name);
            const std::string module_name(name);
            uint32_t handle = 0;
            if(module_name=="kernel32.dll" || module_name=="kernel32") {
                if(library_load)return StartupServiceResult::Unsupported;
                handle=0x00AB1000;
            } else if(module_name=="d3d8.dll" || module_name=="d3d8") {
                handle=0x00AB1200;
                if(library_load){if(d3d8_module_refs_==0xFFFFFFFFu)return StartupServiceResult::ContractFailure;++d3d8_module_refs_;}
                fprintf(log_,"startup_module_backend=d3d8_compatibility refs:%u\n",d3d8_module_refs_);
            } else if(module_name=="d3d8d.dll" || module_name=="d3d8d") {
                wx86_set_lasterr(cpu_,126); // No debug runtime installed.
                fprintf(log_,"startup_module_availability=debug_d3d8_not_installed\n");
            } else return StartupServiceResult::Unsupported;
            cpu_.trap_epilogue(handle, 8, ret);
            expected_eax = handle;
            ++module_calls_;
            fprintf(log_, "startup_module_base_returned=0x%08X\n", handle);
            fprintf(log_, "startup_module_handle_kind=runtime_opaque\n");
            fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n",import.name.c_str());
        } else {
            if(library_load)return StartupServiceResult::ContractFailure;
            // NULL always identifies the current executable. The CRT queries
            // it again immediately before passing HINSTANCE to game startup.
            // This API contract must not depend on call count or return site.
            uint16_t mz = 0;
            uint32_t nt_offset = 0, signature = 0;
            const uint32_t base = image_.load_base();
            if (!cpu_.read(base, &mz, 2) || mz != 0x5A4D ||
                !cpu_.read(base + 0x3C, &nt_offset, 4) ||
                uint64_t(nt_offset) + 4 > image_.image_size() ||
                !cpu_.read(base + nt_offset, &signature, 4) || signature != 0x4550) {
                fprintf(log_, "startup_service_error=module_headers_invalid\n");
                return StartupServiceResult::ContractFailure;
            }
            fprintf(log_, "startup_module_base_returned=0x%08X\n", base);
            fprintf(log_, "startup_module_headers=passed\n");
            fprintf(log_, "startup_module_return_va=0x%08X\n", ret);
            fprintf(log_, "startup_module_handle_kind=current_executable\n");
            if (ret == 0x006424AA) {
                fprintf(log_, "startup_crt_game_call=next_after_module_return\n");
                fprintf(log_, "startup_game_function_va=0x00602A60\n");
                fprintf(log_, "startup_game_function_execution=not_yet_verified\n");
            }
            cpu_.trap_epilogue(base, 8, ret); // HMODULE; stdcall ret 4
            expected_eax = base;
            ++module_calls_;
            fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetModuleHandleA\n");
        }
    } else if (proc_address) {
        uint32_t symbol = 0;
        if (!in_stack(esp, 12) || !cpu_.read(esp + 8, &symbol, 4) ||
            (arg != 0x00AB1000 && arg != 0x00AB1200) || symbol <= 0xFFFFu) {
            fprintf(log_, "startup_service_error=unsupported_export_frame\n");
            return StartupServiceResult::ContractFailure;
        }
        char name[128] = {};
        unsigned n = 0;
        for (; n < sizeof(name); ++n) {
            if (uint64_t(symbol) + n > 0xFFFFFFFFull ||
                !cpu_.read(symbol + n, &name[n], 1))
                return StartupServiceResult::ContractFailure;
            if (!name[n]) break;
        }
        if (n == sizeof(name)) return StartupServiceResult::ContractFailure;
        fprintf(log_, "startup_export_requested=%s\n", name);
        const std::string symbol_name(name);
        if(arg==0x00AB1200) {
            // The D3DX helper checks this optional diagnostic export before
            // calling it. Our backend has no Windows debug output to mute.
            if(symbol_name!="DebugSetMute")return StartupServiceResult::Unsupported;
            expected_eax=0;wx86_set_lasterr(cpu_,127);
            ++unavailable_export_calls_;
            fprintf(log_,"startup_export_availability=optional_debug_export_not_implemented\n");
        } else if (symbol_name == "FlsAlloc" || symbol_name == "FlsFree" ||
            symbol_name == "FlsGetValue" || symbol_name == "FlsSetValue") {
            // The advertised XP profile uses the CRT's existing TLS fallback.
            expected_eax = 0;
            wx86_set_lasterr(cpu_, 127); // ERROR_PROC_NOT_FOUND
            ++unavailable_export_calls_;
            fprintf(log_, "startup_export_availability=not_available_xp_profile\n");
        } else if (symbol_name == "InitializeCriticalSectionAndSpinCount") {
            expected_eax = critical_init_trap;
        } else if (symbol_name == "IsProcessorFeaturePresent") {
            expected_eax = processor_feature_trap;
        } else return StartupServiceResult::Unsupported;
        cpu_.trap_epilogue(expected_eax, 12, ret);
        ++proc_address_calls_;
        fprintf(log_, "startup_export_resolved_va=0x%08X\n", expected_eax);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!GetProcAddress\n");
    } else if (critical_init) {
        uint32_t spin = 0;
        uint32_t previous[6] = {};
        if ((!critical_plain && !cpu_.read(esp + 8, &spin, 4)) || !arg || (arg & 3u) ||
            critical_sections_.count(arg) || !cpu_.read(arg, previous, sizeof(previous))) {
            fprintf(log_, "startup_service_error=invalid_critical_section_frame\n");
            return StartupServiceResult::ContractFailure;
        }
        // x86 RTL_CRITICAL_SECTION: DebugInfo, LockCount, RecursionCount,
        // OwningThread, LockSemaphore, SpinCount. Current scope: one guest thread.
        const uint32_t state[6] = {0, 0xFFFFFFFFu, 0, 0, 0, spin};
        uint32_t copy[6] = {};
        if (!cpu_.write(arg, state, sizeof(state)) || !cpu_.read(arg, copy, sizeof(copy)) ||
            !std::equal(std::begin(state), std::end(state), std::begin(copy))) {
            fprintf(log_, "startup_service_error=critical_section_write_failed\n");
            return StartupServiceResult::ContractFailure;
        }
        critical_sections_.insert(arg);
        ++critical_init_calls_;
        expected_eax = critical_plain ? 0 : 1;
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_critical_section_va=0x%08X\n", arg);
        fprintf(log_, "startup_critical_section_spin=%u\n", spin);
        fprintf(log_, "startup_critical_section_readback=passed\n");
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (critical_op) {
        uint32_t state[6] = {}, tid = 0;
        if (!critical_sections_.count(arg) || !cpu_.read(arg, state, sizeof(state)) ||
            !wx86_cur_tib() || !cpu_.read(wx86_cur_tib() + 0x24, &tid, 4) || !tid) {
            fprintf(log_, "startup_service_error=uninitialized_critical_section\n");
            return StartupServiceResult::ContractFailure;
        }
        if (state[1] != state[2] - 1u || (state[2] == 0) != (state[3] == 0)) {
            fprintf(log_, "startup_service_error=critical_section_state_mismatch\n");
            return StartupServiceResult::ContractFailure;
        }
        bool changed = true;
        if (critical_enter || critical_try) {
            if (state[3] && state[3] != tid) {
                if (!critical_try) {
                    fprintf(log_, "startup_sync_scope=other_thread_wait_not_implemented\n");
                    return StartupServiceResult::Unsupported;
                }
                changed = false; // TryEnter returns FALSE without acquiring.
            } else {
                if (state[2] >= 0x7FFFFFFFu) return StartupServiceResult::ContractFailure;
                ++state[1]; ++state[2]; state[3] = tid;
                expected_eax = critical_try ? 1 : 0;
            }
        } else if (critical_leave) {
            if (!state[2] || state[3] != tid) {
                fprintf(log_, "startup_service_error=critical_section_not_owned\n");
                return StartupServiceResult::ContractFailure;
            }
            --state[1]; --state[2];
            if (!state[2]) state[3] = 0;
        } else {
            if (state[2]) {
                fprintf(log_, "startup_service_error=delete_owned_critical_section\n");
                return StartupServiceResult::ContractFailure;
            }
            for (auto& word : state) word = 0;
        }
        if (changed) {
            uint32_t copy[6] = {};
            if (!cpu_.write(arg, state, sizeof(state)) || !cpu_.read(arg, copy, sizeof(copy)) ||
                !std::equal(std::begin(state), std::end(state), std::begin(copy)))
                return StartupServiceResult::ContractFailure;
            if (critical_delete) critical_sections_.erase(arg);
        }
        ++sync_calls_;
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_sync_section_va=0x%08X\n", arg);
        fprintf(log_, "startup_sync_recursion=%u\n", state[2]);
        fprintf(log_, "startup_sync_owner=%u\n", state[3]);
        fprintf(log_, "startup_sync_state_readback=passed\n");
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (tls) {
        const uint32_t slots = wx86_cur_tib() + 0xE10;
        if (!wx86_cur_tib()) return StartupServiceResult::ContractFailure;
        if (tls_alloc) {
            expected_eax = 0xFFFFFFFFu;
            for (uint32_t i = 0; i < tls_allocated_.size(); ++i) {
                if (tls_allocated_[i]) continue;
                const uint32_t zero = 0;
                if (!cpu_.write(slots + 4 * i, &zero, 4))
                    return StartupServiceResult::ContractFailure;
                tls_allocated_[i] = true;
                expected_eax = i;
                fprintf(log_, "startup_tls_index=%u\n", i);
                break;
            }
            if (expected_eax == 0xFFFFFFFFu) wx86_set_lasterr(cpu_, 259);
        } else if (arg >= tls_allocated_.size() || !tls_allocated_[arg]) {
            wx86_set_lasterr(cpu_, 87);
            expected_eax = 0;
        } else if (tls_get) {
            if (!cpu_.read(slots + 4 * arg, &expected_eax, 4))
                return StartupServiceResult::ContractFailure;
            wx86_set_lasterr(cpu_, 0);
        } else {
            uint32_t value = 0;
            if (tls_set && !cpu_.read(esp + 8, &value, 4))
                return StartupServiceResult::ContractFailure;
            if (!cpu_.write(slots + 4 * arg, &value, 4))
                return StartupServiceResult::ContractFailure;
            if (tls_free) tls_allocated_[arg] = false;
            expected_eax = 1;
        }
        ++tls_calls_;
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (clock) {
        uint64_t value = 0;
        if (file_time) {
            SceRtcTick tick{};
            SceDateTime date{};
            SceUInt64 filetime = 0;
            const int tick_rc = sceRtcGetCurrentTick(&tick);
            const int date_rc = tick_rc < 0 ? tick_rc : sceRtcSetTick(&date, &tick);
            const int file_rc = date_rc < 0 ? date_rc : sceRtcGetWin32FileTime(&date, &filetime);
            fprintf(log_, "startup_clock_rtc_rc=0x%08X\n", unsigned(file_rc));
            if (file_rc < 0) {
                fprintf(log_, "startup_service_error=rtc_filetime_unavailable\n");
                return StartupServiceResult::ContractFailure;
            }
            value = filetime;
            // VOID API: EAX is unspecified; do not advertise a success BOOL.
            fprintf(log_, "startup_clock_source=vita_rtc_utc_win32_filetime\n");
        } else {
            // A virtual uptime starting at native process creation. The same
            // monotonic microsecond clock feeds QPC and its 1 MHz frequency.
            value = performance_frequency ? 1000000ull : sceKernelGetProcessTimeWide();
            expected_eax = tick_count ? uint32_t(value / 1000) : 1;
            fprintf(log_, "startup_clock_source=%s\n", performance_frequency ?
                "fixed_1000000_hz" : "vita_process_uptime_us");
        }
        if (!tick_count) {
            uint8_t bytes[8] = {}, readback[8] = {};
            put32(bytes, uint32_t(value));
            put32(bytes + 4, uint32_t(value >> 32));
            if (!arg || uint64_t(arg) + 8 > 0x100000000ull ||
                !cpu_.read(arg, readback, 8) || !cpu_.write(arg, bytes, 8) ||
                !cpu_.read(arg, readback, 8) ||
                !std::equal(std::begin(bytes), std::end(bytes), std::begin(readback))) {
                fprintf(log_, "startup_service_error=invalid_clock_output_buffer\n");
                return StartupServiceResult::ContractFailure;
            }
            fprintf(log_, "startup_clock_output_readback=passed\n");
        }
        fprintf(log_, "startup_clock_value=%llu\n", static_cast<unsigned long long>(value));
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++clock_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (get_error || set_error || thread_id || process_id) {
        if (get_error) expected_eax = wx86_get_lasterr(cpu_);
        if (set_error) wx86_set_lasterr(cpu_, arg);
        if (thread_id && (!wx86_cur_tib() || !cpu_.read(wx86_cur_tib() + 0x24, &expected_eax, 4)))
            return StartupServiceResult::ContractFailure;
        if (process_id) {
            if (!wx86_cur_tib() || !cpu_.read(wx86_cur_tib() + 0x20, &expected_eax, 4) || !expected_eax)
                return StartupServiceResult::ContractFailure;
            fprintf(log_, "startup_guest_process_id=%u\n", expected_eax);
            ++process_calls_;
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (wide_to_bytes) {
        uint32_t args[8] = {};
        if (!cpu_.read(esp + 4, args, sizeof(args))) return StartupServiceResult::ContractFailure;
        const uint32_t page = args[0], flags = args[1], source = args[2];
        const uint32_t count = args[3], dest = args[4], capacity = args[5], used = args[7];
        fprintf(log_, "startup_conversion_code_page=%u\n", page);
        fprintf(log_, "startup_conversion_input_units=%u\n", count);
        if (page != 0 && page != 1 && page != 932 && page != 1252 && page != 65001)
            return StartupServiceResult::Unsupported;
        if (flags != 0) return StartupServiceResult::Unsupported;
        uint32_t error = 0;
        if (!source || !count || (count > 0x7FFFFFFFu && count != 0xFFFFFFFFu) ||
            capacity > 0x7FFFFFFFu || (capacity && (!dest || dest == source)) ||
            (page == 65001 && (args[6] || used))) error = 87;
        std::vector<uint8_t> bytes;
        const uint32_t max_units = 16384;
        if (!error) {
            if (count != 0xFFFFFFFFu && count > max_units) {
                fprintf(log_, "startup_conversion_scope=input_limit\n");
                return StartupServiceResult::Unsupported;
            }
            const uint32_t limit = count == 0xFFFFFFFFu ? max_units : count;
            bool terminated = false;
            for (uint32_t i = 0; i < limit; ++i) {
                uint16_t unit = 0;
                if (uint64_t(source) + 2ull*i + 2 > 0x100000000ull ||
                    !cpu_.read(source + 2*i, &unit, 2)) return StartupServiceResult::ContractFailure;
                if (page == 0 || page == 1 || page == 932) {
                    const auto* entry = cp932_entry(unit);
                    if (!entry || entry->encoded == 0xFFFF) {
                        fprintf(log_, "startup_conversion_scope=cp932_unmappable_character\n");
                        return StartupServiceResult::Unsupported;
                    }
                    if (entry->encoded > 0xFF) bytes.push_back(uint8_t(entry->encoded >> 8));
                    bytes.push_back(uint8_t(entry->encoded));
                } else if (unit > 127) {
                    fprintf(log_, "startup_conversion_scope=non_ascii_not_implemented\n");
                    return StartupServiceResult::Unsupported;
                } else bytes.push_back(uint8_t(unit));
                if (count == 0xFFFFFFFFu && unit == 0) { terminated = true; break; }
            }
            if (count == 0xFFFFFFFFu && !terminated) return StartupServiceResult::Unsupported;
            if (capacity && capacity < bytes.size()) error = 122;
        }
        if (error) {
            wx86_set_lasterr(cpu_, error);
        } else {
            uint32_t previous_used = 0;
            if (used && !cpu_.read(used, &previous_used, 4)) return StartupServiceResult::ContractFailure;
            if (capacity) {
                if (uint64_t(dest) + bytes.size() > 0x100000000ull)
                    return StartupServiceResult::ContractFailure;
                std::vector<uint8_t> copy(bytes.size());
                if (!cpu_.read(dest, copy.data(), uint32_t(copy.size())) ||
                    !cpu_.write(dest, bytes.data(), uint32_t(bytes.size())) ||
                    !cpu_.read(dest, copy.data(), uint32_t(copy.size())) || copy != bytes)
                    return StartupServiceResult::ContractFailure;
                fprintf(log_, "startup_conversion_readback=passed\n");
            }
            const uint32_t false_value = 0;
            if (used && !cpu_.write(used, &false_value, 4)) return StartupServiceResult::ContractFailure;
            expected_eax = uint32_t(bytes.size());
            fprintf(log_, "startup_conversion_mode=%s\n", capacity ? "write" : "size_query");
            fprintf(log_, "startup_conversion_output_bytes=%u\n", expected_eax);
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++conversion_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!WideCharToMultiByte\n");
    } else if (environment) {
        const bool wide = import.name.back() == 'W';
        if (environment_get) {
            // Owned copy of the empty process environment: two terminating NULs.
            if (!heap_ready_) return StartupServiceResult::ContractFailure;
            if (environment_free_blocks_.empty() && uint64_t(heap_next_) + 16 > 0x01400000u) {
                wx86_set_lasterr(cpu_, 8); // ERROR_NOT_ENOUGH_MEMORY
            } else {
                const uint32_t zero = 0;
                uint32_t copy = 1;
                const uint32_t address = environment_free_blocks_.empty() ?
                    heap_next_ : environment_free_blocks_.back();
                if (!cpu_.write(address, &zero, 4) ||
                    !cpu_.read(address, &copy, 4) || copy != 0)
                    return StartupServiceResult::ContractFailure;
                expected_eax = address;
                if (environment_free_blocks_.empty()) heap_next_ += 16;
                else environment_free_blocks_.pop_back();
                environment_blocks_.emplace(expected_eax, wide);
                fprintf(log_, "startup_environment_block_va=0x%08X\n", expected_eax);
                fprintf(log_, "startup_environment_profile=empty\n");
                fprintf(log_, "startup_environment_readback=passed\n");
            }
        } else {
            const auto block = environment_blocks_.find(arg);
            if (block == environment_blocks_.end() || block->second != wide) {
                wx86_set_lasterr(cpu_, 87);
            } else {
                environment_free_blocks_.push_back(arg);
                environment_blocks_.erase(block);
                expected_eax = 1;
                fprintf(log_, "startup_environment_release=passed\n");
            }
        }
        ++environment_calls_;
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (process) {
        if (exception_filter) {
            // Store guest callback state only. Fault dispatch remains a separate boundary.
            if (arg) {
                uint8_t first = 0;
                if (arg < image_.load_base() || uint64_t(arg) >= uint64_t(image_.load_base()) + image_.image_size() ||
                    !cpu_.read(arg, &first, 1)) {
                    fprintf(log_, "startup_exception_filter_boundary=callback_outside_loaded_image\n");
                    return StartupServiceResult::Unsupported;
                }
            }
            expected_eax = unhandled_filter_;
            unhandled_filter_ = arg;
            fprintf(log_, "startup_exception_filter_previous_va=0x%08X\n", expected_eax);
            fprintf(log_, "startup_exception_filter_registered_va=0x%08X\n", unhandled_filter_);
            fprintf(log_, "startup_exception_filter_scope=registration_only\n");
            fprintf(log_, "startup_exception_dispatch=not_implemented\n");
        } else if (processor_feature) {
            fprintf(log_, "startup_processor_feature_requested=%u\n", arg);
            if (arg != 0) {
                fprintf(log_, "startup_processor_feature_boundary=unimplemented_feature\n");
                return StartupServiceResult::Unsupported;
            }
            // PF_FLOATING_POINT_PRECISION_ERRATA: no Pentium FDIV erratum is
            // advertised by this translated CPU profile. This does not certify x87.
            expected_eax = 0;
            fprintf(log_, "startup_processor_feature_profile=no_pentium_precision_erratum\n");
        } else if (startup_info) {
            // GUI process without inherited CRT handles or reserved startup data.
            const uint32_t info[17] = {68};
            uint32_t old[17] = {}, copy[17] = {};
            if (!arg || uint64_t(arg) + sizeof(info) > 0x100000000ull ||
                !cpu_.read(arg, old, sizeof(old)) || !cpu_.write(arg, info, sizeof(info)) ||
                !cpu_.read(arg, copy, sizeof(copy)) ||
                !std::equal(std::begin(info), std::end(info), std::begin(copy))) {
                fprintf(log_, "startup_service_error=invalid_startup_info_buffer\n");
                return StartupServiceResult::ContractFailure;
            }
            fprintf(log_, "startup_info_bytes=68\n");
            fprintf(log_, "startup_info_readback=passed\n");
        } else if (module_filename) {
            uint32_t dest = 0, capacity = 0;
            if (!cpu_.read(esp + 8, &dest, 4) || !cpu_.read(esp + 12, &capacity, 4))
                return StartupServiceResult::ContractFailure;
            fprintf(log_, "startup_module_filename_handle=0x%08X\n", arg);
            fprintf(log_, "startup_module_filename_capacity=%u\n", capacity);
            if (arg != 0 && arg != image_.load_base()) {
                fprintf(log_, "startup_module_filename_boundary=other_module\n");
                return StartupServiceResult::Unsupported;
            }
            // Same virtual Windows pathname used by ThreadSmoke's process parameters.
            const std::string path = "C:\\TH075\\TH075.exe";
            const bool truncated = capacity <= path.size();
            const uint32_t size = uint32_t(std::min<size_t>(capacity, path.size() + 1));
            if (!capacity) wx86_set_lasterr(cpu_, 0); // Explicit XP zero-size behavior.
            else if (!dest || uint64_t(dest) + size > 0x100000000ull) {
                wx86_set_lasterr(cpu_, 87);
            } else {
                std::vector<uint8_t> bytes(path.begin(), path.end());
                bytes.push_back(0);
                bytes.resize(size); // XP truncation excludes NUL if capacity <= length.
                std::vector<uint8_t> readback(size);
                if (!cpu_.read(dest, readback.data(), size) ||
                    !cpu_.write(dest, bytes.data(), size) ||
                    !cpu_.read(dest, readback.data(), size) || readback != bytes)
                    return StartupServiceResult::ContractFailure;
                expected_eax = truncated ? capacity : uint32_t(path.size());
                if (truncated) wx86_set_lasterr(cpu_, 0);
                fprintf(log_, "startup_module_filename_virtual_path=%s\n", path.c_str());
                fprintf(log_, "startup_module_filename_truncated=%s\n", truncated ? "yes" : "no");
                fprintf(log_, "startup_module_filename_readback=passed\n");
            }
        } else if (command_line) {
            // ThreadSmoke owns the persistent, NUL-terminated ANSI command line.
            expected_eax = 0x00732000;
            char first = 0;
            if (!cpu_.read(expected_eax, &first, 1) || first != '"') {
                fprintf(log_, "startup_service_error=command_line_not_initialized\n");
                return StartupServiceResult::ContractFailure;
            }
            fprintf(log_, "startup_command_line_va=0x%08X\n", expected_eax);
        } else if (std_handle) {
            if (arg != 0xFFFFFFF6u && arg != 0xFFFFFFF5u && arg != 0xFFFFFFF4u) {
                expected_eax = 0xFFFFFFFFu;
                wx86_set_lasterr(cpu_, 87);
            } else {
                // NULL means that this GUI process has no attached console.
                expected_eax = 0;
                fprintf(log_, "startup_standard_handle_profile=no_console\n");
            }
        } else if (handle_count) {
            // Legacy API is a no-op on NT; return the requested count.
            expected_eax = arg;
            fprintf(log_, "startup_handle_count_requested=%u\n", arg);
        } else {
            if (arg != 0 && arg != 0xFFFFFFFFu) return StartupServiceResult::Unsupported;
            expected_eax = 0; // FILE_TYPE_UNKNOWN, invalid NULL/INVALID_HANDLE_VALUE
            wx86_set_lasterr(cpu_, 6);
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        ++process_calls_;
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    } else if (heap_create) {
        uint32_t initial = 0, maximum = 0;
        if (heap_create_calls_ || import.iat_va != 0x00657160 || ret != 0x0064974C ||
            !in_stack(esp, 16) || !cpu_.read(esp + 8, &initial, 4) ||
            !cpu_.read(esp + 12, &maximum, 4) || arg != 0 || initial != 0x1000 || maximum != 0) {
            fprintf(log_, "startup_service_error=unsupported_heap_create_call\n");
            return StartupServiceResult::ContractFailure;
        }
        if (!cpu_.map(heap_next_, 0x00800000, nullptr, d2rt::P_RW)) {
            fprintf(log_, "startup_service_error=heap_map_failed\n");
            return StartupServiceResult::ContractFailure;
        }
        heap_ready_ = true;
        ++heap_create_calls_;
        // trap_epilogue consumes the return address plus all three stdcall arguments.
        cpu_.trap_epilogue(0x00AB0000, 16, ret);
        expected_eax = 0x00AB0000;
        fprintf(log_, "startup_heap_created_handle=0x00AB0000\n");
        fprintf(log_, "startup_heap_initial_bytes=%u\n", initial);
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!HeapCreate\n");
    } else if (heap_alloc || heap_free || heap_size || heap_realloc) {
        uint32_t flags = 0, pointer_or_size = 0, new_size = 0;
        if (!heap_ready_ || arg != 0x00AB0000 || !cpu_.read(esp + 8, &flags, 4) ||
            !cpu_.read(esp + 12, &pointer_or_size, 4) ||
            (heap_realloc && !cpu_.read(esp + 16, &new_size, 4)))
            return StartupServiceResult::ContractFailure;
        const uint32_t allowed_flags = heap_realloc ? 0x19u : heap_alloc ? 9u : 1u;
        if (flags & ~allowed_flags) {
            fprintf(log_, "startup_heap_boundary=unsupported_flags\n");
            return StartupServiceResult::Unsupported;
        }
        fprintf(log_, "startup_heap_flags=0x%08X\n", flags);
        if (heap_alloc) {
            const uint32_t size = pointer_or_size;
            const uint64_t capacity64 = std::max<uint64_t>(16, (uint64_t(size) + 15) & ~15ull);
            const uint32_t address = capacity64<=0x00800000u ? reserve_heap(uint32_t(capacity64)):0;
            if (address) {
                const uint32_t capacity = uint32_t(capacity64);
                if (flags & 8u) {
                    std::vector<uint8_t> zero(capacity), readback(capacity);
                    if (!cpu_.write(address, zero.data(), capacity) ||
                        !cpu_.read(address, readback.data(), capacity) || readback != zero)
                        return StartupServiceResult::ContractFailure;
                }
                heap_blocks_.emplace(address, HeapBlock{size, capacity});
                expected_eax = address;
            }
            if(!address){fprintf(log_,"startup_heap_boundary=capacity_exhausted requested:%u high_water:0x%08X free_ranges:%u\n",size,heap_next_,unsigned(heap_free_ranges_.size()));return StartupServiceResult::Unsupported;}
            ++heap_alloc_calls_;
            fprintf(log_, "startup_heap_alloc_va=0x%08X\n", expected_eax);
            fprintf(log_, "startup_heap_alloc_size=%u\n", size);
        } else {
            const auto block = heap_blocks_.find(pointer_or_size);
            if (block == heap_blocks_.end()) {
                fprintf(log_, "startup_heap_invalid_block_va=0x%08X\n", pointer_or_size);
                // An invalid/double-freed guest pointer is a runtime contract error.
                return StartupServiceResult::ContractFailure;
            }
            const HeapBlock old = block->second;
            fprintf(log_, "startup_heap_block_va=0x%08X\n", pointer_or_size);
            fprintf(log_, "startup_heap_block_size=%u\n", old.size);
            if (heap_size) expected_eax = old.size;
            else if (heap_free) {
                heap_blocks_.erase(block);
                reclaim_heap(pointer_or_size,old.capacity);
                expected_eax = 1;
            } else {
                const uint64_t capacity64 = std::max<uint64_t>(16, (uint64_t(new_size) + 15) & ~15ull);
                const bool in_place = new_size <= old.capacity;
                const uint32_t moved = !in_place && !(flags&0x10u) && capacity64<=0x00800000u ? reserve_heap(uint32_t(capacity64)):0;
                if (in_place || moved) {
                    const uint32_t address = in_place ? pointer_or_size : moved;
                    const uint32_t capacity = in_place ? old.capacity : uint32_t(capacity64);
                    const uint32_t preserved_size = std::min(old.size, new_size);
                    std::vector<uint8_t> preserved_bytes(preserved_size), readback(preserved_size);
                    if (preserved_size && (!cpu_.read(pointer_or_size, preserved_bytes.data(), preserved_size) ||
                        (!in_place && !cpu_.write(address, preserved_bytes.data(), preserved_size))))
                        return StartupServiceResult::ContractFailure;
                    if ((flags & 8u) && new_size > old.size) {
                        std::vector<uint8_t> zero(new_size - old.size), check(zero.size());
                        if (!cpu_.write(address + old.size, zero.data(), uint32_t(zero.size())) ||
                            !cpu_.read(address + old.size, check.data(), uint32_t(check.size())) || check != zero)
                            return StartupServiceResult::ContractFailure;
                    }
                    if (preserved_size && (!cpu_.read(address, readback.data(), preserved_size) || readback != preserved_bytes))
                        return StartupServiceResult::ContractFailure;
                    if (in_place) block->second.size = new_size;
                    else {
                        heap_blocks_.emplace(address, HeapBlock{new_size, capacity});
                        heap_blocks_.erase(block);
                        reclaim_heap(pointer_or_size,old.capacity);
                    }
                    expected_eax = address;
                    fprintf(log_, "startup_heap_realloc_preserved_readback=passed\n");
                    fprintf(log_, "startup_heap_realloc_mode=%s\n", in_place ? "in_place" : "moved");
                } else fprintf(log_, "startup_heap_realloc_mode=failed_original_retained\n");
                fprintf(log_, "startup_heap_realloc_new_size=%u\n", new_size);
                fprintf(log_, "startup_heap_realloc_result_va=0x%08X\n", expected_eax);
            }
            ++heap_other_calls_;
        }
        cpu_.trap_epilogue(expected_eax, cleanup, ret);
        fprintf(log_, "startup_heap_live_blocks=%u\n", unsigned(heap_blocks_.size()));
        fprintf(log_, "startup_serviced_import=KERNEL32.dll!%s\n", import.name.c_str());
    }
    // Check the bridge ABI on each returned API, before another guest block.
    fprintf(log_, "startup_service_stack_bytes=%u\n", cleanup);
    bool abi_ok = cpu_.reg(d2rt::R_ESP) == esp + cleanup && cpu_.reg(d2rt::R_EIP) == ret &&
        cpu_.reg(d2rt::R_EAX) == expected_eax;
    for (unsigned i = 0; i < 4; ++i) abi_ok = abi_ok && cpu_.reg(preserved[i]) == before[i];
    if (!abi_ok) {
        fprintf(log_, "startup_service_error=stdcall_epilogue_mismatch\n");
        return StartupServiceResult::ContractFailure;
    }
    fprintf(log_, "startup_service_abi=passed\n");
    return StartupServiceResult::Serviced;
}

bool StartupServices::finish_window_creation() {
    if(!window_pending_ || cpu_.reg(d2rt::R_ESP)!=window_.frame ||
       window_.frame<kStack+0x2000 || window_.procedure!=0x00603650)return false;
    d2rt::X86Context main{};
    cpu_.save_context(main);
    const uint32_t tib=wx86_cur_tib();
    const uint32_t data=window_.frame-128, frame=window_.frame-512;
    constexpr uint32_t sentinel=0x00BFFFC0;
    // CREATESTRUCTA: lpCreateParams, instance, menu, parent, cy, cx, y, x,
    // style, window name, class name, exstyle. Default desktop origin is 0,0.
    const auto& a=window_.args;
    const uint32_t create[12]={a[11],a[10],a[9],a[8],a[7],a[6],0,0,a[3],a[2],a[1],a[0]};
    const uint32_t rect[4]={0,0,640,480};
    bool ok=cpu_.write(data,create,48) && cpu_.write(data+48,rect,16);
    const bool showing=window_show_pending_,painting=window_paint_pending_;
    const uint32_t messages[3]={painting ? 0x0Fu:showing ? 0x18u:0x81u, showing ? 5u:0x83u, showing ? 3u:1u};
    const int preserved[4]={d2rt::R_EBX,d2rt::R_EBP,d2rt::R_ESI,d2rt::R_EDI};
    for(unsigned i=0;i<(painting ? 1u:3u) && ok;++i) {
        cpu_.load_context(main);
        window_callback_message_=messages[i];
        window_callback_wparam_=showing && messages[i]==0x18 ? 1u:0u;
        window_callback_parameter_=painting ? 0u:showing ? (messages[i]==5 ? (480u<<16)|640u:0u):messages[i]==0x83 ? data+48:data;
        const uint32_t args[5]={sentinel,window_.handle,messages[i],window_callback_wparam_,window_callback_parameter_};
        ok=cpu_.write(frame,args,20);
        bool returned=false, import_failed=false;
        cpu_.set_trap(0x00B00000,0x00C00000,[&](d2rt::Cpu&,uint32_t trap) {
            if(trap==sentinel) {returned=true;return false;}
            if(trap<0x00B00000 || (trap-0x00B00000)%16 ||
               (trap-0x00B00000)/16>=image_.imports().size()) {import_failed=true;return false;}
            const auto& imp=image_.imports()[(trap-0x00B00000)/16];
            if(imp.dll!="USER32.dll" || imp.name!="DefWindowProcA" ||
               call(imp)!=StartupServiceResult::Serviced) {import_failed=true;return false;}
            return true;
        });
        cpu_.set_reg(d2rt::R_ESP,frame);
        cpu_.set_reg(d2rt::R_EIP,window_.procedure);
        cpu_.take_limit_hit();cpu_.set_run_limit(4096);
        const char* fault=nullptr;
        const bool stopped=ok && cpu_.run(window_.procedure,&fault);
        const bool limit=cpu_.take_limit_hit();
        const uint32_t result=cpu_.reg(d2rt::R_EAX);
        ok=stopped && !limit && returned && !import_failed &&
           cpu_.reg(d2rt::R_ESP)==frame+20 && wx86_cur_tib()==tib;
        for(int reg:preserved)ok=ok && cpu_.reg(reg)==main.gpr[reg];
        ok=ok && (messages[i]==0x81 ? result!=0:messages[i]==1 ? result!=0xFFFFFFFFu:result==0);
        fprintf(log_,"startup_window_callback_message=0x%04X\n",messages[i]);
        fprintf(log_,"startup_window_callback_return=0x%08X\n",result);
        fprintf(log_,"startup_window_callback_result=%s\n",ok ? "passed":"failed");
        if(!stopped)fprintf(log_,"startup_window_callback_fault=%s\n",fault ? fault:"unknown");
    }
    uint8_t created=0;
    uint32_t calculated[4]{};
    ok=ok && cpu_.read(0x0068D674,&created,1) && created==1 &&
       cpu_.read(data+48,calculated,16) && !std::memcmp(calculated,rect,16);
    fprintf(log_,"startup_window_original_create_flag=%u\n",unsigned(created));
    cpu_.load_context(main);
    window_callback_message_=window_callback_parameter_=window_callback_wparam_=0;
    window_pending_=false;
    // Remove the callback closure before its stack captures go out of scope.
    cpu_.set_trap(0,0,[](d2rt::Cpu&,uint32_t){return false;});
    if(!ok) {
        window_.surface.clear();window_.handle=0;
        fprintf(log_,"startup_window_creation=callback_contract_failed\n");
        return false;
    }
    window_.created=true;
    if(showing) {window_.visible=true;window_.invalidated=true;}
    if(painting && window_.invalidated)return false;
    window_show_pending_=false;
    window_paint_pending_=false;
    const uint32_t result=painting ? 1u:showing ? uint32_t(window_was_visible_):window_.handle;
    const uint32_t cleanup=painting ? 8u:showing ? 12u:52u;
    cpu_.trap_epilogue(result,cleanup,window_.return_address);
    ok=cpu_.reg(d2rt::R_ESP)==window_.frame+cleanup &&
       cpu_.reg(d2rt::R_EIP)==window_.return_address && cpu_.reg(d2rt::R_EAX)==result;
    fprintf(log_,"startup_window_handle=0x%08X\n",window_.handle);
    fprintf(log_,"%s=%s\n",painting ? "startup_window_update":showing ? "startup_window_show":"startup_window_creation",ok ? "guest_callbacks_completed":"epilogue_failed");
    fprintf(log_,"startup_window_rendering=not_yet_connected\n");
    return ok;
}

StartupServices::~StartupServices() {
    for(const auto& file:files_)fclose(file.second);
}

bool StartupServices::version_globals_match() {
    // These globals are filled by the ORIGINAL instructions after GetVersionExA.
    // Matching them establishes that the guest consumed our returned struct.
    const uint32_t addresses[] = {0x0068E2E8, 0x0068E2EC, 0x0068E2F0, 0x0068E2F4, 0x0068E2F8};
    const uint32_t expected[] = {kPlatform, kBuild, (kMajor << 8) + kMinor, kMajor, kMinor};
    bool ok = true;
    for (unsigned i = 0; i < 5; ++i) {
        uint32_t value = 0;
        const bool read = cpu_.read(addresses[i], &value, 4);
        fprintf(log_, "startup_version_global=0x%08X value=0x%08X expected=0x%08X\n",
            addresses[i], value, expected[i]);
        ok = ok && read && value == expected[i];
    }
    fprintf(log_, "startup_version_globals=%s\n", ok ? "passed" : "failed");
    return ok;
}

uint32_t StartupServices::reserve_heap(uint32_t capacity) {
    for(auto it=heap_free_ranges_.begin();it!=heap_free_ranges_.end();++it) {
        if(it->second<capacity)continue;
        const uint32_t address=it->first,remainder=it->second-capacity;
        heap_free_ranges_.erase(it);
        if(remainder)heap_free_ranges_.emplace(address+capacity,remainder);
        fprintf(log_,"startup_heap_reuse=address:0x%08X bytes:%u\n",address,capacity);
        return address;
    }
    if(uint64_t(heap_next_)+capacity>0x01400000u)return 0;
    const uint32_t address=heap_next_;heap_next_+=capacity;return address;
}
void StartupServices::reclaim_heap(uint32_t address,uint32_t capacity) {
    auto next=heap_free_ranges_.lower_bound(address);
    if(next!=heap_free_ranges_.begin()) {
        auto previous=std::prev(next);
        if(uint64_t(previous->first)+previous->second==address){address=previous->first;capacity+=previous->second;heap_free_ranges_.erase(previous);}
    }
    next=heap_free_ranges_.lower_bound(address);
    if(next!=heap_free_ranges_.end() && uint64_t(address)+capacity==next->first){capacity+=next->second;heap_free_ranges_.erase(next);}
    if(uint64_t(address)+capacity==heap_next_)heap_next_=address;
    else heap_free_ranges_.emplace(address,capacity);
    fprintf(log_,"startup_heap_reclaim=address:0x%08X bytes:%u high_water:0x%08X ranges:%u\n",address,capacity,heap_next_,unsigned(heap_free_ranges_.size()));
}
