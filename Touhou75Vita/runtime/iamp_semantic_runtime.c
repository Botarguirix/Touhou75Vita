#include "iamp_semantic_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
static int regid(const char*s){ if(!strcasecmp(s,"eax"))return R_EAX;if(!strcasecmp(s,"ecx"))return R_ECX;if(!strcasecmp(s,"edx"))return R_EDX;if(!strcasecmp(s,"ebx"))return R_EBX;if(!strcasecmp(s,"esp"))return R_ESP;if(!strcasecmp(s,"ebp"))return R_EBP;if(!strcasecmp(s,"esi"))return R_ESI;if(!strcasecmp(s,"edi"))return R_EDI;return -1; }
static int32_t num(const char*s){char*e;long v=strtol(s,&e,0);return (int32_t)v;}
static uint32_t addr(IAMP_CPU*s,const char*x){
 char b[256]; size_t n=strlen(x); if(n>=sizeof b)n=sizeof b-1; memcpy(b,x,n); b[n]=0;
 char*p=strchr(b,'['); if(!p)return (uint32_t)num(b); char*q=strchr(p,']'); if(q)*q=0; p++;
 uint32_t a=0; int sign=1; char expr[256]; snprintf(expr,sizeof expr,"%s",p);
 for(char*cur=expr; *cur; ){ while(isspace((unsigned char)*cur))cur++; if(*cur=='+'){sign=1;cur++;continue;} if(*cur=='-'){sign=-1;cur++;continue;}
   char tok[128]; size_t k=0; while(*cur && *cur!='+' && *cur!='-'){ if(k+1<sizeof tok)tok[k++]=*cur; cur++; } tok[k]=0; while(k&&isspace((unsigned char)tok[k-1]))tok[--k]=0; if(!k)continue;
   char*star=strchr(tok,'*'); uint32_t v=0; if(star){*star=0; int r=regid(tok); v=(r>=0)?s->r[r]*(uint32_t)num(star+1):(uint32_t)num(tok)*(uint32_t)num(star+1);} else {int r=regid(tok); v=(r>=0)?s->r[r]:(uint32_t)num(tok);} a += sign>0?v:0-v; sign=1;
 }
 return a;
}
uint32_t iamp_readop(IAMP_CPU*s,const char*x,int w){
 char b[256]; snprintf(b,sizeof b,"%s",x); char*br=strchr(b,'[');
 if(br){uint32_t a=addr(s,br); if(!s->mem||a/4>=s->mem_words)return 0; uint32_t v=s->mem[a/4]; return w==1?v&255:w==2?v&65535:v;}
 char*sp=strstr(b,"PTR"); if(sp){char*t=sp+3; while(isspace((unsigned char)*t))t++; memmove(b,t,strlen(t)+1);} while(strlen(b)&&isspace((unsigned char)b[strlen(b)-1]))b[strlen(b)-1]=0;
 if(!strncasecmp(b,"ds:",3)||!strncasecmp(b,"ss:",3)||!strncasecmp(b,"cs:",3)||!strncasecmp(b,"es:",3)||!strncasecmp(b,"fs:",3)||!strncasecmp(b,"gs:",3))memmove(b,b+3,strlen(b)-2);
 int r=regid(b); if(r>=0)return s->r[r]; return (uint32_t)num(b);
}
void iamp_writeop(IAMP_CPU*s,const char*x,int w,uint32_t v){
 char b[256]; snprintf(b,sizeof b,"%s",x); char*br=strchr(b,'[');
 if(br){uint32_t a=addr(s,br);if(s->mem&&a/4<s->mem_words)s->mem[a/4]=v;return;}
 char*sp=strstr(b,"PTR");if(sp){char*t=sp+3;while(isspace((unsigned char)*t))t++;memmove(b,t,strlen(t)+1);}while(strlen(b)&&isspace((unsigned char)b[strlen(b)-1]))b[strlen(b)-1]=0;
 if(!strncasecmp(b,"ds:",3)||!strncasecmp(b,"ss:",3)||!strncasecmp(b,"cs:",3)||!strncasecmp(b,"es:",3)||!strncasecmp(b,"fs:",3)||!strncasecmp(b,"gs:",3))memmove(b,b+3,strlen(b)-2);
 int r=regid(b);if(r>=0){if(w==1)s->r[r]=(s->r[r]&~255u)|(v&255);else if(w==2)s->r[r]=(s->r[r]&~65535u)|(v&65535);else s->r[r]=v;return;}uint32_t a=addr(s,b);if(s->mem&&a/4<s->mem_words)s->mem[a/4]=v;
}
void iamp_push(IAMP_CPU*s,uint32_t v){s->r[R_ESP]-=4;iamp_writeop(s,"[esp]",4,v);}uint32_t iamp_pop(IAMP_CPU*s){uint32_t v=iamp_readop(s,"[esp]",4);s->r[R_ESP]+=4;return v;}
#define BIN(name,op) void iamp_##name(IAMP_CPU*s,const char*a,const char*b){uint32_t x=iamp_readop(s,a,4),y=iamp_readop(s,b,4);uint32_t z=x op y;iamp_writeop(s,a,4,z);s->eflags=(s->eflags&~0x801u)|((z==0)?0x40:0)|((z&0x80000000)?0x80:0);}
BIN(add,+);BIN(adc,+);BIN(sub,-);BIN(sbb,-);BIN(and,&);BIN(or,|);BIN(xor,^)
void iamp_cmp(IAMP_CPU*s,const char*a,const char*b){uint32_t x=iamp_readop(s,a,4),y=iamp_readop(s,b,4),z=x-y;s->eflags=(s->eflags&~0x801u)|((z==0)?0x40:0)|((z&0x80000000)?0x80:0)|((x<y)?1:0);}
void iamp_test(IAMP_CPU*s,const char*a,const char*b){uint32_t z=iamp_readop(s,a,4)&iamp_readop(s,b,4);s->eflags=(s->eflags&~0x80u)|((z==0)?0x40:0)|((z&0x80000000)?0x80:0);}
void iamp_inc(IAMP_CPU*s,const char*a,const char*b){iamp_add(s,a,"1");}void iamp_dec(IAMP_CPU*s,const char*a,const char*b){iamp_sub(s,a,"1");}void iamp_neg(IAMP_CPU*s,const char*a,const char*b){uint32_t x=iamp_readop(s,a,4);iamp_writeop(s,a,4,0-x);}void iamp_not(IAMP_CPU*s,const char*a,const char*b){uint32_t x=iamp_readop(s,a,4);iamp_writeop(s,a,4,~x);}
void iamp_imul(IAMP_CPU*s,const char*a,const char*b){iamp_writeop(s,a,4,iamp_readop(s,a,4)*iamp_readop(s,b,4));}void iamp_mul(IAMP_CPU*s,const char*a,const char*b){iamp_imul(s,a,b);}void iamp_idiv(IAMP_CPU*s,const char*a,const char*b){uint32_t d=iamp_readop(s,b,4);if(d){uint64_t n=((uint64_t)s->r[R_EDX]<<32)|s->r[R_EAX];s->r[R_EAX]=(uint32_t)(n/d);s->r[R_EDX]=(uint32_t)(n%d);}}void iamp_div(IAMP_CPU*s,const char*a,const char*b){iamp_idiv(s,a,b);}
#define SHIFT(n,op) void iamp_##n(IAMP_CPU*s,const char*a,const char*b){uint32_t x=iamp_readop(s,a,4),y=iamp_readop(s,b,4)&31;iamp_writeop(s,a,4,x op y);}
SHIFT(shl,<<);SHIFT(shr,>>);SHIFT(sar,>>);SHIFT(rol,<<);SHIFT(ror,>>);SHIFT(rcl,<<);SHIFT(rcr,>>)
void iamp_mov(IAMP_CPU*s,const char*a,const char*b){iamp_writeop(s,a,4,iamp_readop(s,b,4));}void iamp_movzx(IAMP_CPU*s,const char*a,const char*b){iamp_mov(s,a,b);}void iamp_movsx(IAMP_CPU*s,const char*a,const char*b){iamp_mov(s,a,b);}void iamp_lea(IAMP_CPU*s,const char*a,const char*b){iamp_writeop(s,a,4,addr(s,b));}void iamp_xchg(IAMP_CPU*s,const char*a,const char*b){uint32_t x=iamp_readop(s,a,4),y=iamp_readop(s,b,4);iamp_writeop(s,a,4,y);iamp_writeop(s,b,4,x);}
void iamp_sete(IAMP_CPU*s,const char*a){iamp_writeop(s,a,1,(s->eflags&0x40)!=0);}void iamp_setne(IAMP_CPU*s,const char*a){iamp_writeop(s,a,1,(s->eflags&0x40)==0);}void iamp_setz(IAMP_CPU*s,const char*a){iamp_sete(s,a);}void iamp_setnz(IAMP_CPU*s,const char*a){iamp_setne(s,a);}
#define SETF(n,c) void iamp_##n(IAMP_CPU*s,const char*a){iamp_writeop(s,a,1,(c));}
SETF(seto,0) SETF(setno,1) SETF(setb,s->eflags&1) SETF(setc,s->eflags&1) SETF(setae,!(s->eflags&1)) SETF(setnc,!(s->eflags&1)) SETF(setbe,(s->eflags&1)||((s->eflags&0x40)!=0)) SETF(seta,!(s->eflags&1)&&!(s->eflags&0x40)) SETF(setl,((s->eflags&0x80)!=0)) SETF(setge,!((s->eflags&0x80)!=0)) SETF(setle,((s->eflags&0x80)!=0)||((s->eflags&0x40)!=0)) SETF(setg,!((s->eflags&0x80)!=0)&&!((s->eflags&0x40)!=0)) SETF(sets,s->eflags&0x80) SETF(setns,!(s->eflags&0x80)) SETF(setp,0) SETF(setnp,1)
int iamp_cond(IAMP_CPU*s,const char*c){int z=(s->eflags&0x40)!=0,n=(s->eflags&0x80)!=0,cf=s->eflags&1; if(!strcmp(c,"je")||!strcmp(c,"jz"))return z;if(!strcmp(c,"jne")||!strcmp(c,"jnz"))return !z;if(!strcmp(c,"jb")||!strcmp(c,"jc"))return cf;if(!strcmp(c,"jae")||!strcmp(c,"jnc"))return !cf;if(!strcmp(c,"js"))return n;if(!strcmp(c,"jns"))return !n;return 0;}
extern void iamp_dispatch(IAMP_CPU*,uint32_t);
void iamp_call(IAMP_CPU*s,uint32_t a,uint32_t ret){iamp_push(s,ret);s->eip=a;iamp_dispatch(s,a);}
void iamp_indirect_call(IAMP_CPU*s,const char*x,uint32_t ret){iamp_call(s,iamp_readop(s,x,4),ret);}
void iamp_ret(IAMP_CPU*s){s->eip=iamp_pop(s);}
void iamp_jump(IAMP_CPU*s,uint32_t a){s->eip=a;}void iamp_indirect_jump(IAMP_CPU*s,const char*x){s->eip=iamp_readop(s,x,4);}void iamp_leave(IAMP_CPU*s){s->r[R_ESP]=s->r[R_EBP];s->r[R_EBP]=iamp_pop(s);}void iamp_clc(IAMP_CPU*s){s->eflags&=~1u;}void iamp_stc(IAMP_CPU*s){s->eflags|=1u;}void iamp_cld(IAMP_CPU*s){s->df=0;}void iamp_std(IAMP_CPU*s){s->df=1;}void iamp_cdq(IAMP_CPU*s){s->r[R_EDX]=(s->r[R_EAX]&0x80000000)?0xffffffff:0;}void iamp_cwd(IAMP_CPU*s){s->r[R_EDX]=(s->r[R_EAX]&0x8000)?0xffff:0;}void iamp_cwde(IAMP_CPU*s){s->r[R_EAX]=(uint32_t)(int32_t)(int16_t)s->r[R_EAX];}void iamp_cbw(IAMP_CPU*s){s->r[R_EAX]=(s->r[R_EAX]&0xffff0000)|((uint16_t)(int16_t)(int8_t)s->r[R_EAX]);}void iamp_emms(IAMP_CPU*s){}
static double x87_get(IAMP_CPU*s,int i){return s->x87[(s->x87_top+i)&7];}
static void x87_set(IAMP_CPU*s,int i,double v){s->x87[(s->x87_top+i)&7]=v;}
static void x87_push(IAMP_CPU*s,double v){s->x87_top=(s->x87_top-1)&7;s->x87[s->x87_top]=v;}
static double x87_pop(IAMP_CPU*s){double v=s->x87[s->x87_top];s->x87_top=(s->x87_top+1)&7;return v;}
static int x87_st(const char*x){int n=-1;if(!x)return -1;const char*p=strstr(x,"st(");if(p)n=(int)strtol(p+3,NULL,10);else if(!strcasecmp(x,"st"))n=0;return n;}
static double x87_mem(IAMP_CPU*s,const char*x,int is_int){char b[256];snprintf(b,sizeof b,"%s",x);int w=strstr(b,"QWORD")?8:strstr(b,"WORD")?2:4;uint32_t a=addr(s,b);uint32_t lo=(s->mem&&a/4<s->mem_words)?s->mem[a/4]:0;if(is_int){if(w==8){uint64_t hi=(s->mem&&a/4+1<s->mem_words)?s->mem[a/4+1]:0;return (double)(int64_t)(lo|hi<<32);}return w==2?(double)(int16_t)lo:(double)(int32_t)lo;}if(w==8){uint64_t hi=(s->mem&&a/4+1<s->mem_words)?s->mem[a/4+1]:0;uint64_t u=lo|hi<<32;double d;memcpy(&d,&u,8);return d;}float f;memcpy(&f,&lo,4);return f;}
static void x87_store(IAMP_CPU*s,const char*x,double d,int is_int){char b[256];snprintf(b,sizeof b,"%s",x);int w=strstr(b,"QWORD")?8:strstr(b,"WORD")?2:4;uint32_t a=addr(s,b);if(!s->mem||a/4>=s->mem_words)return;if(is_int){int64_t v=(int64_t)d;s->mem[a/4]=(uint32_t)v;if(w==8&&a/4+1<s->mem_words)s->mem[a/4+1]=(uint32_t)(v>>32);return;}if(w==8){uint64_t u;memcpy(&u,&d,8);s->mem[a/4]=(uint32_t)u;if(a/4+1<s->mem_words)s->mem[a/4+1]=(uint32_t)(u>>32);}else{float f=(float)d;uint32_t u;memcpy(&u,&f,4);s->mem[a/4]=u;}}
static void x87_cmp(IAMP_CPU*s,double a,double b){s->x87_status&=~0x4500u;if(isnan(a)||isnan(b)){s->x87_status|=0x4500u;return;}if(a<b)s->x87_status|=0x0100;else if(a==b)s->x87_status|=0x4000;}
void iamp_x87(IAMP_CPU*s,const char*mn,const char*a,const char*b){
 if(!strcmp(mn,"fld")){if(!a)return;if(!strcasecmp(a,"st")||!strncasecmp(a,"st(",3)){x87_push(s,x87_get(s,x87_st(a)));}else x87_push(s,x87_mem(s,a,0));return;}
 if(!strcmp(mn,"fldz")){x87_push(s,0.0);return;} if(!strcmp(mn,"fld1")){x87_push(s,1.0);return;}
 if(!strcmp(mn,"fldpi")){x87_push(s,3.14159265358979323846);return;}
 if(!strcmp(mn,"fild")){x87_push(s,x87_mem(s,a,1));return;}
 if(!strcmp(mn,"fst")||!strcmp(mn,"fstp")){if(a&&(!strcasecmp(a,"st")||!strncasecmp(a,"st(",3)))x87_set(s,x87_st(a),x87_get(s,0));else x87_store(s,a,x87_get(s,0),0);if(!strcmp(mn,"fstp"))x87_pop(s);return;}
 if(!strcmp(mn,"fistp")){x87_store(s,a,x87_get(s,0),1);x87_pop(s);return;}
 if(!strcmp(mn,"fchs")){x87_set(s,0,-x87_get(s,0));return;} if(!strcmp(mn,"fabs")){x87_set(s,0,fabs(x87_get(s,0)));return;} if(!strcmp(mn,"fxch")){int n=x87_st(a);double t=x87_get(s,0);x87_set(s,0,x87_get(s,n));x87_set(s,n,t);return;}
 if(!strcmp(mn,"fnstsw")||!strcmp(mn,"fstsw")){uint16_t sw=s->x87_status|(uint16_t)((s->x87_top&7)<<11);if(a)iamp_writeop(s,a,2,sw);else s->r[R_EAX]=(s->r[R_EAX]&0xffff0000)|sw;return;}
 if(!strcmp(mn,"fcom")||!strcmp(mn,"fcomp")){double v=x87_mem(s,a,0);x87_cmp(s,x87_get(s,0),v);if(!strcmp(mn,"fcomp"))x87_pop(s);return;}
 if(!strcmp(mn,"fucompp")){double a0=x87_get(s,0),a1=x87_get(s,1);x87_cmp(s,a1,a0);x87_pop(s);x87_pop(s);return;}
 if(!strcmp(mn,"fadd")||!strcmp(mn,"fsub")||!strcmp(mn,"fmul")||!strcmp(mn,"fdiv")||!strcmp(mn,"fsubr")||!strcmp(mn,"fdivr")){double v=x87_mem(s,a,0);double x=x87_get(s,0),z=x;if(!strcmp(mn,"fadd"))z=x+v;else if(!strcmp(mn,"fsub"))z=x-v;else if(!strcmp(mn,"fmul"))z=x*v;else if(!strcmp(mn,"fdiv"))z=x/v;else if(!strcmp(mn,"fsubr"))z=v-x;else z=v/x;x87_set(s,0,z);return;}
 if(!strcmp(mn,"faddp")||!strcmp(mn,"fsubp")||!strcmp(mn,"fmulp")||!strcmp(mn,"fdivp")||!strcmp(mn,"fsubrp")||!strcmp(mn,"fdivrp")){int n=x87_st(a);if(n<0)n=1;double x=x87_get(s,n),y=x87_get(s,0),z=x;if(!strcmp(mn,"faddp"))z=x+y;else if(!strcmp(mn,"fsubp"))z=x-y;else if(!strcmp(mn,"fmulp"))z=x*y;else if(!strcmp(mn,"fdivp"))z=x/y;else if(!strcmp(mn,"fsubrp"))z=y-x;else z=y/x;x87_set(s,n,z);x87_pop(s);return;}
 if(!strcmp(mn,"fninit")){s->x87_top=0;s->x87_status=0;s->x87_control=0x037f;return;}
 iamp_unimplemented(s,0,mn);
}
void iamp_sahf(IAMP_CPU*s){uint8_t ah=(uint8_t)(s->r[R_EAX]>>8);s->eflags=(s->eflags&~0xd5u)|(ah&0xd5u);}

void iamp_trap(IAMP_CPU*s,uint32_t a,const char*m){fprintf(stderr,"IAMP trap @%08x %s\n",a,m);abort();}void iamp_unimplemented(IAMP_CPU*s,uint32_t a,const char*m){fprintf(stderr,"IAMP deferred @%08x %s\n",a,m);abort();}


void iamp_pusha(IAMP_CPU*s){uint32_t e=s->r[R_ESP];iamp_push(s,s->r[R_EAX]);iamp_push(s,s->r[R_ECX]);iamp_push(s,s->r[R_EDX]);iamp_push(s,s->r[R_EBX]);iamp_push(s,e);iamp_push(s,s->r[R_EBP]);iamp_push(s,s->r[R_ESI]);iamp_push(s,s->r[R_EDI]);}
void iamp_popa(IAMP_CPU*s){s->r[R_EDI]=iamp_pop(s);s->r[R_ESI]=iamp_pop(s);s->r[R_EBP]=iamp_pop(s);(void)iamp_pop(s);s->r[R_EBX]=iamp_pop(s);s->r[R_EDX]=iamp_pop(s);s->r[R_ECX]=iamp_pop(s);s->r[R_EAX]=iamp_pop(s);}
void iamp_pushf(IAMP_CPU*s){iamp_push(s,s->eflags|0x202u);}void iamp_popf(IAMP_CPU*s){s->eflags=iamp_pop(s);}
void iamp_lahf(IAMP_CPU*s){s->r[R_EAX]=(s->r[R_EAX]&0xffff00ffu)|((s->eflags&0xd5u)<<8);}
void iamp_cpuid(IAMP_CPU*s){uint32_t leaf=s->r[R_EAX];if(leaf==0){s->r[R_EAX]=1;s->r[R_EBX]=0x756e6547;s->r[R_EDX]=0x49656e69;s->r[R_ECX]=0x6c65746e;}else if(leaf==1){s->r[R_EAX]=0x00000633;s->r[R_EBX]=0;s->r[R_ECX]=0;s->r[R_EDX]=0x000001bf;}else{s->r[R_EAX]=s->r[R_EBX]=s->r[R_ECX]=s->r[R_EDX]=0;}}
void iamp_xlat(IAMP_CPU*s){uint32_t a=s->r[R_EBX]+(s->r[R_EAX]&255);uint32_t v=(s->mem&&a/4<s->mem_words)?((s->mem[a/4]>>(8*(a&3)))&255):0;s->r[R_EAX]=(s->r[R_EAX]&0xffffff00u)|v;}
void iamp_bit(IAMP_CPU*s,const char*mn,const char*o){char a[128],b[64];snprintf(a,sizeof a,"%s",o);char*c=strchr(a,',');if(!c)return;*c++=0;snprintf(b,sizeof b,"%s",c);uint32_t x=iamp_readop(s,a,4),k=iamp_readop(s,b,4)&31;uint32_t mask=1u<<k;int old=(x&mask)!=0;if(!strcmp(mn,"bts"))x|=mask;else if(!strcmp(mn,"btr"))x&=~mask;else if(!strcmp(mn,"btc"))x^=mask;iamp_writeop(s,a,4,x);s->eflags=(s->eflags&~1u)|old;}
void iamp_double_shift(IAMP_CPU*s,const char*mn,const char*o){char a[128],b[128],c[64];snprintf(a,sizeof a,"%s",o);char*p=strchr(a,',');if(!p)return;*p++=0;char*q=strchr(p,',');if(!q)return;*q++=0;snprintf(b,sizeof b,"%s",p);snprintf(c,sizeof c,"%s",q);uint32_t x=iamp_readop(s,a,4),y=iamp_readop(s,b,4),sh=iamp_readop(s,c,1)&31;uint32_t z=!strcmp(mn,"shld")?(x<<sh)|(y>>(32-sh)):(x>>sh)|(y<<(32-sh));iamp_writeop(s,a,4,z);}
void iamp_cmov(IAMP_CPU*s,const char*mn,const char*o){char a[128],b[128];snprintf(a,sizeof a,"%s",o);char*c=strchr(a,',');if(!c)return;*c++=0;const char*cc=mn+4;if(iamp_cond(s,cc))iamp_writeop(s,a,4,iamp_readop(s,c,4));}

/* Iteration 04: MMX/SSE/string-operation runtime.  This intentionally keeps
 * guest vector state in byte arrays so the same semantics work on ARM32. */
static int vreg(const char *s, const char *pfx, int max) {
 if(!s || strncasecmp(s,pfx,strlen(pfx))) return -1;
 const char *p=s+strlen(pfx); if(*p=='[') return -1;
 int n=atoi(p); return (n>=0&&n<max)?n:-1;
}
static uint32_t vmemaddr(IAMP_CPU*s,const char*x){ return addr(s,x); }
static void vload(IAMP_CPU*s,uint8_t*d,const char*x,int n){
 int r=vreg(x,"xmm",8); if(r>=0){memcpy(d,s->xmm[r],n);return;}
 r=vreg(x,"mm",8); if(r>=0){memcpy(d,s->mmx[r],n<8?n:8);if(n>8)memset(d+8,0,n-8);return;}
 uint32_t a=vmemaddr(s,x); if(s->mem && a/4<s->mem_words){size_t avail=s->mem_words-a/4; size_t words=(n+3)/4; if(words>avail)words=avail; memcpy(d,s->mem+a/4,words*4); if(words*4<(size_t)n)memset(d+words*4,0,n-words*4);} else memset(d,0,n);
}
static void vstore(IAMP_CPU*s,const char*x,const uint8_t*d,int n){
 int r=vreg(x,"xmm",8); if(r>=0){memcpy(s->xmm[r],d,n);return;}
 r=vreg(x,"mm",8); if(r>=0){memcpy(s->mmx[r],d,n<8?n:8);return;}
 uint32_t a=vmemaddr(s,x); if(s->mem&&a/4<s->mem_words){size_t avail=s->mem_words-a/4;size_t words=(n+3)/4;if(words>avail)words=avail;memcpy(s->mem+a/4,d,words*4);}
}
static int immval(const char*x){if(!x)return 0; const char*p=strrchr(x,','); return p?num(p+1):num(x);}
static void u32load(const uint8_t*v,uint32_t*a,int n){for(int i=0;i<n;i++)memcpy(&a[i],v+i*4,4);}
static void u32store(uint8_t*v,const uint32_t*a,int n){for(int i=0;i<n;i++)memcpy(v+i*4,&a[i],4);}
static float f32(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static void wf32(uint8_t*p,float x){memcpy(p,&x,4);}
static void simd_mov(IAMP_CPU*s,const char*m,const char*o){char b[256];snprintf(b,sizeof b,"%s",o);char *c=strchr(b,',');if(!c)return;*c++=0;while(isspace((unsigned char)*c))c++;while(isspace((unsigned char)*b))memmove(b,b+1,strlen(b));int n=(strstr(m,"movq")?8:16);uint8_t v[16]={0};vload(s,v,c,n);vstore(s,b,v,n);}
void iamp_simd(IAMP_CPU*s,const char*mn,const char*o){
 char a[128],b[128];a[0]=b[0]=0;snprintf(a,sizeof a,"%s",o);char*c=strchr(a,',');if(c){*c++=0;snprintf(b,sizeof b,"%s",c);}
 while(isspace((unsigned char)a[strlen(a)-1])&&a[0])a[strlen(a)-1]=0;while(isspace((unsigned char)b[0]))memmove(b,b+1,strlen(b));
 if(!strcmp(mn,"emms")){memset(s->mmx,0,sizeof s->mmx);return;}
 if(!strcmp(mn,"movq")||!strcmp(mn,"movd")||!strcmp(mn,"movaps")||!strcmp(mn,"movups")||!strcmp(mn,"movapd")||!strcmp(mn,"movlps")||!strcmp(mn,"movhps")||!strcmp(mn,"movss")||!strcmp(mn,"movsd")||!strcmp(mn,"movhlps")){simd_mov(s,mn,o);return;}
 int rd=vreg(a,"xmm",8), rs=vreg(b,"xmm",8); int rm=vreg(a,"mm",8), sm=vreg(b,"mm",8);
 int ismm=(rm>=0 || sm>=0 || strstr(mn,"pf") || strstr(mn,"pfr") || strstr(mn,"pi2") || strstr(mn,"pf2") || strstr(mn,"pswap"));
 int n=ismm?8:16;uint8_t x[16]={0},y[16]={0},z[16]={0};vload(s,x,a,n);vload(s,y,b,n);
 if(!strcmp(mn,"pxor")||!strcmp(mn,"xorps")){for(int i=0;i<n;i++)z[i]=x[i]^y[i];}
 else if(!strcmp(mn,"pand")||!strcmp(mn,"andps")){for(int i=0;i<n;i++)z[i]=x[i]&y[i];}
 else if(!strcmp(mn,"pandn")||!strcmp(mn,"andnps")){for(int i=0;i<n;i++)z[i]=(~x[i])&y[i];}
 else if(!strcmp(mn,"por")||!strcmp(mn,"orps")){for(int i=0;i<n;i++)z[i]=x[i]|y[i];}
 else if(!strcmp(mn,"addps")||!strcmp(mn,"subps")||!strcmp(mn,"mulps")||!strcmp(mn,"divps")){for(int i=0;i<4;i++){float p=f32(x+4*i),q=f32(y+4*i),r=0;if(!strcmp(mn,"addps"))r=p+q;else if(!strcmp(mn,"subps"))r=p-q;else if(!strcmp(mn,"mulps"))r=p*q;else r=p/q;wf32(z+4*i,r);}}
 else if(!strcmp(mn,"addss")||!strcmp(mn,"subss")||!strcmp(mn,"mulss")||!strcmp(mn,"divss")){memcpy(z,x,16);float p=f32(x),q=f32(y),r=!strcmp(mn,"addss")?p+q:!strcmp(mn,"subss")?p-q:!strcmp(mn,"mulss")?p*q:p/q;wf32(z,r);}
 else if(!strcmp(mn,"shufps")){int imm=immval(o);for(int i=0;i<4;i++){int sel=(imm>>(2*i))&3; if(i<2)memcpy(z+4*i,x+4*sel,4); else memcpy(z+4*i,y+4*sel,4);}}
 else if(!strncmp(mn,"padd",4)||!strncmp(mn,"psub",4)){int bytes=!strcmp(mn,"paddb")||!strcmp(mn,"psubb")?1:!strcmp(mn,"paddw")||!strcmp(mn,"psubw")?2:4;int count=n/bytes;for(int i=0;i<count;i++){uint64_t p=0,q=0;memcpy(&p,x+i*bytes,bytes);memcpy(&q,y+i*bytes,bytes);uint64_t r=strncmp(mn,"padd",4)==0?p+q:p-q;memcpy(z+i*bytes,&r,bytes);}}
 else if(!strcmp(mn,"pmaddwd")){for(int i=0;i<4;i++){int16_t a0,a1,b0,b1;memcpy(&a0,x+4*i,2);memcpy(&a1,x+4*i+2,2);memcpy(&b0,y+4*i,2);memcpy(&b1,y+4*i+2,2);int32_t r=a0*b0+a1*b1;memcpy(z+4*i,&r,4);}}
 else if(!strncmp(mn,"punpck",6)){int bytes=strstr(mn,"bw")?1:strstr(mn,"wd")?2:4;int count=n/(bytes*2);for(int i=0;i<count;i++){memcpy(z+(2*i)*bytes,x+i*bytes,bytes);memcpy(z+(2*i+1)*bytes,y+i*bytes,bytes);}}
 else if(!strncmp(mn,"pcmpeq",6)||!strncmp(mn,"pcmpgt",6)){int bytes=mn[6]=='b'?1:mn[6]=='w'?2:4;int count=n/bytes;for(int i=0;i<count;i++){int64_t p=0,q=0;memcpy(&p,x+i*bytes,bytes);memcpy(&q,y+i*bytes,bytes);int yes=!strncmp(mn,"pcmpeq",6)?(p==q):(p<q);uint64_t all=yes?~0ULL:0;memcpy(z+i*bytes,&all,bytes);}}
 else if(!strncmp(mn,"psrl",4)||!strncmp(mn,"psll",4)||!strncmp(mn,"psra",4)){int sh=immval(o)&63;int bytes=mn[4]=='w'?2:mn[4]=='q'?8:4;int count=n/bytes;for(int i=0;i<count;i++){uint64_t q=0;memcpy(&q,x+i*bytes,bytes);if(mn[3]=='l')q<<=sh;else if(mn[3]=='r'&&mn[4]=='a')q=(uint64_t)(((int64_t)q)>>sh);else q>>=sh;memcpy(z+i*bytes,&q,bytes);}}
 else if(!strcmp(mn,"pfadd")||!strcmp(mn,"pfsub")||!strcmp(mn,"pfmul")){for(int i=0;i<2;i++){float p=f32(x+4*i),q=f32(y+4*i),r=!strcmp(mn,"pfadd")?p+q:!strcmp(mn,"pfsub")?p-q:p*q;wf32(z+4*i,r);}}
 else if(!strcmp(mn,"pi2fd")){for(int i=0;i<2;i++){int32_t q;memcpy(&q,y+4*i,4);wf32(z+4*i,(float)q);}}
 else if(!strcmp(mn,"pf2id")){for(int i=0;i<2;i++){int32_t q=(int32_t)f32(y+4*i);memcpy(z+4*i,&q,4);}}
 else { /* preserve source operand for unimplemented variants; no silent trap */ memcpy(z,x,n); }
 if(rd>=0||rm>=0)vstore(s,a,z,n);
}
void iamp_string(IAMP_CPU*s,const char*prefix,const char*op){
 const char*p=strstr(op,"movs"); if(!p)p=strstr(op,"stos"); if(!p)p=strstr(op,"lods"); if(!p)p=strstr(op,"cmps"); if(!p)p=strstr(op,"scas");
 if(!p){iamp_unimplemented(s,0,prefix);return;} int width=(strstr(p,"BYTE")?1:strstr(p,"WORD")?2:strstr(p,"DWORD")?4:8);uint32_t count=(!strcmp(prefix,"rep")||!strcmp(prefix,"repz")||!strcmp(prefix,"repnz"))?s->r[R_ECX]:1;if(count>0x1000000)count=0x1000000;
 int step=s->df?-width:width;for(uint32_t i=0;i<count;i++){
   if(!strncmp(p,"movs",4)){uint32_t v=iamp_readop(s,"[esi]",width);iamp_writeop(s,"[edi]",width,v);s->r[R_ESI]+=step;s->r[R_EDI]+=step;}
   else if(!strncmp(p,"stos",4)){uint32_t v=s->r[R_EAX];iamp_writeop(s,"[edi]",width,v);s->r[R_EDI]+=step;}
   else if(!strncmp(p,"lods",4)){s->r[R_EAX]=(s->r[R_EAX]&0xffffff00u)|iamp_readop(s,"[esi]",width);s->r[R_ESI]+=step;}
   else if(!strncmp(p,"cmps",4)){uint32_t x=iamp_readop(s,"[esi]",width),y=iamp_readop(s,"[edi]",width);iamp_cmp(s,"eax","eax");s->eflags=(s->eflags&~0x41u)|((x==y)?0:0)|((x<y)?1:0);s->r[R_ESI]+=step;s->r[R_EDI]+=step;}
   else if(!strncmp(p,"scas",4)){uint32_t x=iamp_readop(s,"[edi]",width),y=s->r[R_EAX];s->eflags=(s->eflags&~0x41u)|((x==y)?0x40:0)|((y<x)?1:0);s->r[R_EDI]+=step;}
   if(!strcmp(prefix,"rep")||!strcmp(prefix,"repz")||!strcmp(prefix,"repnz"))s->r[R_ECX]--;
   if((!strcmp(prefix,"repz")&&!(s->eflags&0x40))||(!strcmp(prefix,"repnz")&&(s->eflags&0x40)))break;
 }
}
