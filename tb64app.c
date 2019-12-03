/**
Copyright (c) 2016-2019, Powturbo
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    - homepage : https://sites.google.com/site/powturbo/
    - github   : https://github.com/powturbo
    - twitter  : https://twitter.com/powturbo
    - email    : powturbo [_AT_] gmail [_DOT_] com
**/
// Turbo Base64: TB64app.c - Benchmark app

#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
  #if !defined(_WIN32)  
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
  #else
#include <io.h> 
#include <fcntl.h>
  #endif

#ifdef __APPLE__
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
  #ifdef _MSC_VER
#include "vs/getopt.h"
  #else
#include <getopt.h> 
#endif

#include "conf.h"
#include "turbob64.h"
#include "time_.h"

  #ifdef BASE64
#include "xb64test.h"
  #endif

//------------------------------- malloc ------------------------------------------------
#define USE_MMAP
  #if __WORDSIZE == 64
#define MAP_BITS 30
  #else
#define MAP_BITS 28
  #endif

void *_valloc(size_t size, unsigned a) {
  if(!size) return NULL;
    #if defined(_WIN32)
  return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #elif defined(USE_MMAP) && !defined(__APPLE__)
  void *ptr = mmap(NULL/*0(size_t)a<<MAP_BITS*/, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if(ptr == MAP_FAILED) return NULL;                                                        
  return ptr;
    #else
  return malloc(size); 
    #endif
}

void _vfree(void *p, size_t size) {
  if(!p) return;
    #if defined(_WIN32)
  VirtualFree(p, 0, MEM_RELEASE);
    #elif defined(USE_MMAP) && !defined(__APPLE__)
  munmap(p, size);
    #else
  free(p);
    #endif
} 

int memcheck(unsigned char *in, unsigned n, unsigned char *cpy) { 
  int i;
  for(i = 0; i < n; i++)
    if(in[i] != cpy[i]) {
      printf("ERROR in[%d]=%x, dec[%d]=%x\n", i, in[i], i, cpy[i]);
      return i+1; 
    }
  return 0;
}

void pr(unsigned l, unsigned n) { double r = (double)l*100.0/n; if(r>0.1) printf("%10u %6.2f%% ", l, r);else printf("%10u %7.3f%%", l, r); fflush(stdout); }

#define ID_MEMCPY 16
unsigned bench(unsigned char *in, unsigned n, unsigned char *out, unsigned char *cpy, int id) { 
  unsigned l=0;
    #ifndef _MSC_VER
  memrcpy(cpy,in,n); 
    #endif
  switch(id) {
    case 1:                    TMBENCH("",l=tb64senc(   in, n, out),n); pr(l,n); TMBENCH2("tb64s",    tb64sdec(out,  l, cpy), l);   break;
    case 2:                    TMBENCH("",l=tb64xenc(   in, n, out),n); pr(l,n); TMBENCH2("tb64x",    tb64xdec( out, l, cpy), l);   break;
      #if defined(__i386__) || defined(__x86_64__) || defined(__ARM_NEON) || defined(__powerpc64__)
    case 3:                    TMBENCH("",l=tb64enc(    in, n, out),n); pr(l,n); TMBENCH2("tb64auto", tb64dec(out, l, cpy), l);      break;
    case 4:if(cpuini(0)>=33) { TMBENCH("",l=tb64sseenc( in, n, out),n); pr(l,n); TMBENCH2("tb64sse",  tb64ssedec(out, l, cpy), l); } break;
      #else
    case 3: case 4:return 0;  
      #endif
      #if defined(__i386__) || defined(__x86_64__)
    case 5:if(cpuini(0)>=50) { TMBENCH("",l=tb64avxenc( in, n, out),n); pr(l,n); TMBENCH2("tb64avx",  tb64avxdec( out, l, cpy), l); } break;
    case 6:if(cpuini(0)>=60) { TMBENCH("",l=tb64avx2enc(in, n, out),n); pr(l,n); TMBENCH2("tb64avx2", tb64avx2dec(out, l, cpy), l); } break;
      #else
    case 5: case 6:return 0;  
      #endif
      break;
      #ifdef BASE64
    #include "xtb64test.c"
      #endif
    case ID_MEMCPY:           TMBENCH( "", memcpy(out,in,n) ,n);       pr(n,n); TMBENCH2("memcpy", memcpy(cpy,out,n), n);  l=n; break;
    default: return 0;
  }
  if(l) { printf(" %10d\n", n); memcheck(in,n,cpy); }
  return l;
}

void usage(char *pgm) {
  fprintf(stderr, "\nTurboBase64 Copyright (c) 2016-2019 Powturbo %s\n", __DATE__);
  fprintf(stderr, "Usage: %s [options] [file]\n", pgm);
  fprintf(stderr, " -e#      # = function ids separated by ',' or ranges '#-#' (default='1-%d')\n", ID_MEMCPY);
  fprintf(stderr, " -B#s     # = max. benchmark filesize (default 1GB) ex. -B4G\n");
  fprintf(stderr, "          s = modifier s:B,K,M,G=(1, 1000, 1.000.000, 1.000.000.000) s:k,m,h=(1024,1Mb,1Gb). (default m) ex. 64k or 64K\n");
  fprintf(stderr, "Benchmark:\n");
  fprintf(stderr, " -i#/-j#  # = Minimum  de/compression iterations per run (default=auto)\n");
  fprintf(stderr, " -I#/-J#  # = Number of de/compression runs (default=3)\n");
  fprintf(stderr, " -e#      # = function id\n");
  fprintf(stderr, " -q#      # = cpuid (33:ssse3, 50:avx, 60:avx2 default:auto detect). Only for tb64enc/tb64dec)\n");
  fprintf(stderr, "Ex. turbob64 file\n");
  fprintf(stderr, "    turbob64 -e3 file\n");
  fprintf(stderr, "    turbob64 -q33 file\n");
  fprintf(stderr, "    turbob64 -q33 file -I15 -J15\n");
  exit(0);
} 

int main(int argc, char* argv[]) {                              
  unsigned cmp=1, b = 1 << 30, esize=4, lz=0, fno,id=0,fuzz=3,bid=0;
  char     *scmd = NULL;

  int      c, digit_optind = 0, this_option_optind = optind ? optind : 1, option_index = 0;
  static struct option long_options[] = { {"blocsize",  0, 0, 'b'}, {0, 0, 0}  };
  for(;;) {
    if((c = getopt_long(argc, argv, "B:ce:f:I:J:kq:", long_options, &option_index)) == -1) break;
    switch(c) {
      case  0 : printf("Option %s", long_options[option_index].name); if(optarg) printf (" with arg %s", optarg);  printf ("\n"); break;                                
      case 'B': b = argtoi(optarg,1);   break;
      case 'c': cmp++;                  break;
      case 'k': bid++;                  break;
      case 'f': fuzz = atoi(optarg);    break;
      case 'e': scmd = optarg;          break;
      case 'I': if((tm_Rep  = atoi(optarg))<=0) tm_rep =tm_Rep=1; break;
      case 'J': if((tm_Rep2 = atoi(optarg))<=0) tm_rep2=tm_Rep2=1; break;
      case 'q': cpuini(atoi(optarg));  break;
      default: 
        usage(argv[0]);
        exit(0); 
    }
  }

   tb64ini(0); 
   printf("detected simd (id=%d->'%s')\n\n", cpuini(0), cpustr(cpuini(0))); 
   printf("  E MB/s    size     ratio%%   D MB/s   function\n");  
   char _scmd[33]; sprintf(_scmd, "1-%d", ID_MEMCPY);

  if(argc - optind < 1) { //fprintf(stderr, "File not specified\n"); exit(-1);     
    unsigned _sizes0[] = { 10, 100, 1*KB, 10*KB, 100*KB, 1*MB, 10*MB, 20*MB, 30*MB, 40*MB, 50*MB, 100*MB, 0 },
//             _sizes1[] = { 10*KB, 100*KB, 1*MB, 10*MB, 30*MB, 0 }, *sizes = bid?_sizes0:_sizes1;
             _sizes1[] = { 1000, 1001, 1002, 1003, 1004, 0 }, *sizes = bid?_sizes0:_sizes1;
    unsigned n = 100*Mb, insize = n, outsize  = turbob64len(insize)*2;
    unsigned char *_in;
    if(!(_in = (unsigned char*)_valloc(insize+1024,1))) die("malloc error in size=%u\n", insize); //_in[insize]=0;
  
    unsigned char *_cpy = _in, *cpy=_cpy, *in = _in, *_out = (unsigned char*)_valloc(outsize,2),*out=_out;  if(!_out) die("malloc error out size=%u\n", outsize);
    if(cmp && !(_cpy = (unsigned char*)_valloc(outsize,3))) die("malloc error cpy size=%u\n", insize);

    for(int s = 0; sizes[s]; s++) {
      n = sizes[s];  if(b!=(1<<30) && n<b) continue;
      
      if(fuzz & 1) { in  = (_in +insize)-n; }  //in[n]=0;
      if(fuzz & 2) { out = (_out+outsize)-turbob64len(n); cpy = (_cpy+insize)-n; } 
      srand(0); for(int i = 0; i < n; i++) in[i] = rand()&0xff;
      if(!bid) {
        if(n>=1000000) printf("size=%uMb (Mb=1.000.000)\n", n/1000000);
        else           printf("size=%uKb (Kb=1.000)\n", n/1000);
      }
      char *p = scmd?scmd:_scmd;
      do { 
        unsigned id = strtoul(p, &p, 10),idx = id, i;    
        while(isspace(*p)) p++; if(*p == '-') { if((idx = strtoul(p+1, &p, 10)) < id) idx = id; if(idx > ID_MEMCPY) idx = ID_MEMCPY; } 
        for(i = id; i <= idx; i++)
          bench(in, n, out,cpy,i);
      } while(*p++);
    }
    //_vfree(_in); free(_out); if(cpy) free(cpy);   
    exit(0);
  }
  
  {
    unsigned char *in = NULL, *out = NULL, *cpy = NULL;
    for(fno = optind; fno < argc; fno++) {
      uint64_t flen;
      int n,i;    
      char *inname = argv[fno];                                     
      FILE *fi = fopen(inname, "rb");                           if(!fi ) { perror(inname); continue; }  
      fseek(fi, 0, SEEK_END); 
      flen = ftell(fi); 
      fseek(fi, 0, SEEK_SET);
    
      if(flen > b) flen = b;
      n = flen; 
      if(!(in  =        (unsigned char*)malloc(n+64)))                 { fprintf(stderr, "malloc error\n"); exit(-1); } cpy = in;
      if(!(out =        (unsigned char*)malloc(turbob64len(flen)+64))) { fprintf(stderr, "malloc error\n"); exit(-1); } 
      if(cmp && !(cpy = (unsigned char*)malloc(n+64)))                 { fprintf(stderr, "malloc error\n"); exit(-1); }
      n = fread(in, 1, n, fi);                                           printf("File='%s' Length=%u\n", inname, n);            
      fclose(fi);
      if(n <= 0) exit(0);
      tm_init(tm_Rep, tm_Rep2);  

      tb64ini(0); 
      printf("detected simd=%s\n\n", cpustr(cpuini(0))); 

      printf("  E MB/s    size     ratio    D MB/s   function\n");  

      char *p = scmd?scmd:_scmd;
      do { 
        unsigned id = strtoul(p, &p, 10),idx = id, i;    
        while(isspace(*p)) p++; if(*p == '-') { if((idx = strtoul(p+1, &p, 10)) < id) idx = id; if(idx > ID_MEMCPY) idx = ID_MEMCPY; } 
        for(i = id; i <= idx; i++)
          bench(in,n,out,cpy,i);    
      } while(*p++);
    }
    if (in) free(in);
    if (out) free(out);
    if (cpy) free(cpy);
  }
}
