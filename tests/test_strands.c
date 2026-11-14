#define _POSIX_C_SOURCE 200809L
#include "ib_strands.h"
#include <assert.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void write_all(int fd, const void *ptr, size_t len) {
    const unsigned char *bytes=ptr;
    while(len) { ssize_t n=write(fd,bytes,len); assert(n>0);bytes+=n;len-=(size_t)n; }
}
static void hex_hash(const void *p,size_t n,char hex[65]) {
    unsigned char result[32]; unsigned int written=0;
    EVP_MD_CTX *md=EVP_MD_CTX_new(); assert(md);
    assert(EVP_DigestInit_ex(md,EVP_sha256(),NULL)==1);
    assert(EVP_DigestUpdate(md,p,n)==1);
    assert(EVP_DigestFinal_ex(md,result,&written)==1 && written==32);
    EVP_MD_CTX_free(md);
    static const char digits[]="0123456789abcdef";
    for(size_t i=0;i<32;++i){hex[2*i]=digits[result[i]>>4]; hex[2*i+1]=digits[result[i]&15];}
    hex[64]=0;
}
static void file(const char *path,const void *p,size_t n) {
    int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0600);assert(fd>=0);write_all(fd,p,n);assert(!close(fd));
}
static void test_mixed_source(const char *work) {
    char path1[1024],path2[1024],indexpath[1024],error[256]={0},hash1[65],hash2[65];
    snprintf(path1,sizeof(path1),"%s/part1",work);
    snprintf(path2,sizeof(path2),"%s/part2",work);
    snprintf(indexpath,sizeof(indexpath),"%s/strand.idx",work);
    const unsigned char first[]={0, '#','#',' ','U','S','E','R','\n',0xcf,0x80,'\r','\n',0xff,'A'};
    const unsigned char second[]={0,'{','}','\n','\r','\n',0xe2,0x88,0xa7,'B'};
    file(path1,first,sizeof first);file(path2,second,sizeof second);
    hex_hash(first,sizeof first,hash1);hex_hash(second,sizeof second,hash2);
    ib_source sources[2];
    assert(!ib_source_open(&sources[0],path1,"tao-human",hash1,error,sizeof error));
    assert(!ib_source_open(&sources[1],path2,"tao-machine",hash2,error,sizeof error));
    ib_fragment fragments[]={
      {.id="f1",.source=0,.start=0,.end=4},
      {.id="f2",.source=0,.start=4,.end=sizeof first},
      {.id="empty",.source=1,.start=0,.end=0},
      {.id="f3",.source=1,.start=0,.end=sizeof second}
    };
    ib_visit visits[]={{0},{1},{2},{3}};
    ib_link links[]={
      {.from=1,.to=0,.kind="parent"},
      {.from=2,.to=1,.kind="parent"},
      {.from=0,.to=1,.kind="next"},
      {.from=1,.to=3,.kind="reply"},
      {.from=2,.to=3,.kind="citation"}
    };
    ib_strand s={.sources=sources,.source_count=2,.fragments=fragments,.fragment_count=4,
      .visits=visits,.visit_count=4,.links=links,.link_count=5};
    assert(!ib_strand_index(&s,error,sizeof error));
    assert(ib_strand_bytes(&s)==sizeof first+sizeof second);
    assert(s.prefix[2]==sizeof first && s.prefix[3]==sizeof first);
    uint32_t id=UINT32_MAX;
    assert(!ib_fragment_index(&s,"f3",&id) && id==3);
    assert(ib_fragment_index(&s,"absent",&id)!=0);
    uint32_t destinations[2]={UINT32_MAX,UINT32_MAX};
    assert(ib_links_from(&s,1,"reply",destinations,2)==1 && destinations[0]==3);
    assert(ib_links_to(&s,3,"reply",destinations,2)==1 && destinations[0]==1);
    assert(ib_links_to(&s,3,"citation",destinations,2)==1 && destinations[0]==2);
    unsigned char expected[sizeof first+sizeof second];
    memcpy(expected,first,sizeof first);memcpy(expected+sizeof first,second,sizeof second);
    unsigned char output[IB_MAX_WINDOW]; size_t got;
    for(size_t off=0;off<=sizeof expected;++off) {
        assert(!ib_strand_window(&s,off,output,sizeof output,&got));
        assert(got==sizeof expected-off && !memcmp(expected+off,output,got));
    }
    assert(!ib_strand_window(&s,sizeof first-2,output,7,&got));
    assert(got==7 && memcmp(expected+sizeof first-2,output,7)==0);
    assert(ib_strand_window(&s,0,output,4097,&got)!=0);
    assert(!ib_index_write(&s,indexpath,hash1,error,sizeof error));
    assert(!ib_index_check(&s,indexpath,hash1,error,sizeof error));
    ib_strand reopened={0};
    assert(!ib_index_load_checked(&reopened,&s,indexpath,hash1,error,sizeof error));
    assert(reopened.fragment_count==4 && reopened.visit_count==4 && reopened.link_count==5);
    assert(!ib_strand_window(&reopened,5,output,9,&got));
    assert(got==9 && !memcmp(expected+5,output,got));
    ib_index_loaded_free(&reopened);
    /* A malicious yet internally consistent index can carry the original
     * snapshot digest while replacing the selected traversal. Reject it. */
    char forgedpath[1024];snprintf(forgedpath,sizeof forgedpath,"%s/forged.idx",work);
    visits[0].fragment=1;
    assert(!ib_strand_index(&s,error,sizeof error));
    assert(!ib_index_write(&s,forgedpath,hash1,error,sizeof error));
    visits[0].fragment=0;
    assert(!ib_strand_index(&s,error,sizeof error));
    assert(ib_index_load_checked(&reopened,&s,forgedpath,hash1,error,sizeof error)!=0);
    assert(ib_index_write(&s,indexpath,hash1,error,sizeof error)!=0); /* no overwrite */
    assert(ib_index_check(&s,indexpath,hash2,error,sizeof error)!=0); /* wrong snapshot */
    /* Corrupt exactly one byte, keeping index length unchanged. */
    int fd=open(indexpath,O_WRONLY); assert(fd>=0);
    assert(pwrite(fd,"X",1,8)==1);assert(!close(fd));
    assert(ib_index_check(&s,indexpath,hash1,error,sizeof error)!=0);
    assert(ib_index_load_checked(&reopened,&s,indexpath,hash1,error,sizeof error)!=0);
    /* A cycle in explicit source-parent edges must never be admitted. */
    links[0].from=0;links[0].to=2;strcpy(links[3].kind,"parent");links[3].to=0;
    assert(ib_strand_index(&s,error,sizeof error)!=0);
    links[0].from=1;links[0].to=0;strcpy(links[3].kind,"reply");links[3].to=3;
    assert(!ib_strand_index(&s,error,sizeof error));
    /* Duplicate stable fragment IDs, wrong ranges and unknown nodes. */
    strcpy(fragments[3].id,"f2");
    assert(ib_strand_index(&s,error,sizeof error)!=0);
    strcpy(fragments[3].id,"f3");
    fragments[3].end++;
    assert(ib_strand_index(&s,error,sizeof error)!=0);
    fragments[3].end--;
    visits[1].fragment=77;
    assert(ib_strand_index(&s,error,sizeof error)!=0);
    visits[1].fragment=1;
    assert(!ib_strand_index(&s,error,sizeof error));
    /* Same-length source mutation cannot escape a fresh admission. */
    fd=open(path2,O_WRONLY);assert(fd>=0);assert(pwrite(fd,"Z",1,2)==1);assert(!close(fd));
    assert(ib_source_window(&sources[1],0,output,5,&got)!=0);
    ib_source changed;
    assert(ib_source_open(&changed,path2,"tao-machine",hash2,error,sizeof error)!=0);
    char symlink_path[1024];snprintf(symlink_path,sizeof symlink_path,"%s/redirect",work);
    assert(!symlink(path1,symlink_path));
    assert(ib_source_open(&changed,symlink_path,"tao-machine",hash1,error,sizeof error)!=0);
    assert(ib_source_open(&changed,path1,"../unsafe",hash1,error,sizeof error)!=0);
    ib_strand_release(&s);ib_source_close(&sources[0]);ib_source_close(&sources[1]);
    puts("PASS mixed roles, opaque bytes, boundaries, navigation, links, corruption, cycles");
}
static void test_saved_position(const char *work) {
    char path[1024];snprintf(path,sizeof path,"%s/position",work);
    const char *sha="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    ib_read_position original={.offset=4095};
    strcpy(original.snapshot,sha);strcpy(original.object,"message-4");
    ib_read_position read_back={0};
    assert(ib_position_read(path,&read_back)==1);
    assert(!ib_position_write(path,&original));
    assert(!ib_position_read(path,&read_back));
    assert(ib_position_matches(&read_back,sha,"message-4",4095));
    assert(!ib_position_matches(&read_back,sha,"message-4",4094));
    assert(!ib_position_matches(&read_back,sha,"message-5",5000));
    assert(!ib_position_matches(&read_back,
      "1123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef","message-4",5000));
    original.offset=0;
    assert(!ib_position_write(path,&original));
    assert(!ib_position_read(path,&read_back)&&read_back.offset==0);
    int fd=open(path,O_WRONLY|O_TRUNC);assert(fd>=0);
    write_all(fd,"position 1\nsnapshot 0\n",22);close(fd);
    assert(ib_position_read(path,&read_back)!=0);
    puts("PASS snapshot-bound restart positions, stale/invalid identity, atomic updates");
}
static void test_large(const char *work) {
    char path[1024],hash[65],error[256];
    snprintf(path,sizeof path,"%s/large",work);
    int fd=open(path,O_CREAT|O_EXCL|O_RDWR,0600);assert(fd>=0);
    const uint64_t size=UINT64_C(64)*1024*1024;
    assert(!ftruncate(fd,(off_t)size));
    /* Each end page has a distinct marker; an incorrect seek is detectable. */
    for(uint32_t i=0;i<512;i++) {
        uint64_t off=size-512*4096+(uint64_t)i*4096;
        unsigned char mark[4]={(unsigned char)i,(unsigned char)(i>>8),0x83,0x7f};
        assert(pwrite(fd,mark,4,(off_t)off)==4);
    }
    /* Host fixture hashes the full 64MiB file once; reader windows do not. */
    EVP_MD_CTX *digest=EVP_MD_CTX_new();assert(digest && EVP_DigestInit_ex(digest,EVP_sha256(),NULL)==1);
    unsigned char bytes[4096];
    for(uint64_t off=0;off<size;off+=4096) {
        assert(pread(fd,bytes,sizeof bytes,(off_t)off)==sizeof bytes);
        assert(EVP_DigestUpdate(digest,bytes,sizeof bytes)==1);
    }
    unsigned char hash_bytes[32];unsigned int n=0;
    assert(EVP_DigestFinal_ex(digest,hash_bytes,&n)==1 && n==32);EVP_MD_CTX_free(digest);
    static const char digits[]="0123456789abcdef";
    for(size_t i=0;i<32;i++){hash[i*2]=digits[hash_bytes[i]>>4];hash[i*2+1]=digits[hash_bytes[i]&15];}hash[64]=0;
    assert(!close(fd));
    ib_source source;
    assert(!ib_source_open(&source,path,"large",hash,error,sizeof error));
    ib_fragment f[]={ {.id="huge",.source=0,.start=0,.end=size} };
    ib_visit v[]={{0}};
    ib_strand s={.sources=&source,.source_count=1,.fragments=f,.fragment_count=1,.visits=v,.visit_count=1};
    assert(!ib_strand_index(&s,error,sizeof error));
    size_t got;unsigned char out[IB_MAX_WINDOW];
    for(uint32_t i=0;i<512;i++) {
        uint64_t off=size-512*4096+(uint64_t)i*4096;
        assert(!ib_strand_window(&s,off,out,4096,&got) && got==4096);
        assert(out[0]==(unsigned char)i && out[1]==(unsigned char)(i>>8) && out[2]==0x83 && out[3]==0x7f);
    }
    assert(!ib_strand_window(&s,size-1,out,4096,&got) && got==1);
    assert(!ib_strand_window(&s,size,out,4096,&got) && got==0);
    ib_strand_release(&s);ib_source_close(&source);
    puts("PASS 64MiB source, 512 unique near-tail page seeks, 4096-byte max allocation");
}
static void test_utf8_boundary_projection(const char *work) {
    char path[1024],hash[65],error[256]={0};
    snprintf(path,sizeof path,"%s/utf8",work);
    const unsigned char text[]={ 'A',0xcf,0x80,0xf0,0x9f,0x92,0xa1,'B' };
    file(path,text,sizeof text);
    hex_hash(text,sizeof text,hash);
    ib_source source;
    assert(!ib_source_open(&source,path,"unicode",hash,error,sizeof error));
    ib_offset_span cp[]={{0,1},{1,3},{3,4},{4,4}}, actual[4]={0};
    assert(!ib_utf8_map_spans(&source,cp,4,actual,error,sizeof error));
    assert(actual[0].start==0 && actual[0].end==1);
    assert(actual[1].start==1 && actual[1].end==7);
    assert(actual[2].start==7 && actual[2].end==8);
    assert(actual[3].start==8 && actual[3].end==8);
    ib_offset_span invalid_ranges[]={{1,3},{2,4}};
    assert(ib_utf8_map_spans(&source,invalid_ranges,2,actual,error,sizeof error)!=0);
    ib_offset_span past_end[]={{5,6}};
    assert(ib_utf8_map_spans(&source,past_end,1,actual,error,sizeof error)!=0);
    ib_source_close(&source);

    char largepath[1024];snprintf(largepath,sizeof largepath,"%s/utf8-window",work);
    unsigned char across[4100];memset(across,'x',4095);
    const unsigned char glyph[]={0xf0,0x9f,0x92,0xa1};
    memcpy(across+4095,glyph,4);across[4099]='\n';
    file(largepath,across,sizeof across);hex_hash(across,sizeof across,hash);
    assert(!ib_source_open(&source,largepath,"utf8-window",hash,error,sizeof error));
    ib_offset_span wanted[]={{4095,4096},{4096,4097}};
    assert(!ib_utf8_map_spans(&source,wanted,2,actual,error,sizeof error));
    assert(actual[0].start==4095 && actual[0].end==4099);
    assert(actual[1].start==4099 && actual[1].end==4100);
    ib_source_close(&source);

    char badpath[1024];snprintf(badpath,sizeof badpath,"%s/bad-utf8",work);
    const unsigned char wrong[]={0xcf,0x80,0xff};
    file(badpath,wrong,sizeof wrong);hex_hash(wrong,sizeof wrong,hash);
    assert(!ib_source_open(&source,badpath,"bad-utf8",hash,error,sizeof error));
    ib_offset_span range[]={{0,1}};
    assert(ib_utf8_map_spans(&source,range,1,actual,error,sizeof error)!=0);
    ib_source_close(&source);
    puts("PASS Idric code-point to exact-byte mapping, split UTF-8, malformed rejection");
}
int main(void) {
    char dir[]="/tmp/ib-strands-XXXXXXXX";
    assert(mkdtemp(dir));
    test_mixed_source(dir);
    test_saved_position(dir);
    test_large(dir);
    test_utf8_boundary_projection(dir);
    return 0;
}
