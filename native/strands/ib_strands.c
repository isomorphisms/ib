#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ib_strands.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <openssl/evp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void problem(char *dst, size_t size, const char *fmt, ...) {
    if (!dst || !size) return;
    va_list args;
    va_start(args, fmt);
    (void)vsnprintf(dst, size, fmt, args);
    va_end(args);
}
static int plain_id(const char *id, size_t maximum) {
    size_t n;
    if (!id || (n = strnlen(id, maximum + 1)) == 0 || n > maximum) return 0;
    if (n == 1 && id[0] == '-') return 0;
    for (size_t i = 0; i < n; ++i)
        if (!((id[i] >= 'a' && id[i] <= 'z') ||
              (id[i] >= '0' && id[i] <= '9') || id[i] == '_' || id[i] == '-')) return 0;
    return 1;
}
static int digest_valid(const char *hex) {
    if (!hex || strnlen(hex, 66) != 64) return 0;
    for (int i = 0; i != 64; ++i)
        if (!((hex[i] >= '0' && hex[i] <= '9') || (hex[i] >= 'a' && hex[i] <= 'f'))) return 0;
    return 1;
}
static uint64_t nanostamp(time_t sec, long ns) {
    return (uint64_t)sec * UINT64_C(1000000000) + (uint64_t)ns;
}
static void stamp(ib_source *s, const struct stat *st) {
    s->stamp_size = (uint64_t)st->st_size;
    s->stamp_mtime_ns = nanostamp(st->st_mtim.tv_sec, st->st_mtim.tv_nsec);
    s->stamp_ctime_ns = nanostamp(st->st_ctim.tv_sec, st->st_ctim.tv_nsec);
    s->stamp_dev = (uint64_t)st->st_dev;
    s->stamp_ino = (uint64_t)st->st_ino;
}
static int unchanged(const ib_source *s) {
    struct stat st;
    if (fstat(s->fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 0) return 0;
    return s->stamp_size == (uint64_t)st.st_size &&
           s->stamp_mtime_ns == nanostamp(st.st_mtim.tv_sec, st.st_mtim.tv_nsec) &&
           s->stamp_ctime_ns == nanostamp(st.st_ctim.tv_sec, st.st_ctim.tv_nsec) &&
           s->stamp_dev == (uint64_t)st.st_dev && s->stamp_ino == (uint64_t)st.st_ino;
}
int ib_source_open(ib_source *s, const char *path, const char *id,
                   const char *hex, char *error, size_t error_length) {
    if (!s || !path || !plain_id(id, 64) || !digest_valid(hex)) {
        problem(error, error_length, "invalid source identity/digest"); return -1;
    }
    memset(s, 0, sizeof(*s)); s->fd = -1;
    s->fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
    if (s->fd < 0) { problem(error, error_length, "source open: %s", strerror(errno)); return -1; }
    struct stat st;
    if (fstat(s->fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 0) goto corrupt;
    s->bytes = (uint64_t)st.st_size;
    stamp(s, &st);
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) goto corrupt;
    int good = EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == 1;
    unsigned char buffer[IB_MAX_WINDOW];
    uint64_t cursor = 0;
    while (good && cursor < s->bytes) {
        size_t want = (s->bytes - cursor < sizeof buffer) ? (size_t)(s->bytes - cursor) : sizeof buffer;
        if (cursor > INT64_MAX) { good = 0; break; }
        ssize_t count = pread(s->fd, buffer, want, (off_t)cursor);
        if (count > 0 && (size_t)count <= want) {
            good = EVP_DigestUpdate(ctx, buffer, (size_t)count) == 1;
            cursor += (uint64_t)count;
        } else if (count < 0 && errno == EINTR) continue;
        else good = 0;
    }
    unsigned char hash[32]; unsigned int result_bytes = 0;
    if (good) good = EVP_DigestFinal_ex(ctx, hash, &result_bytes) == 1 && result_bytes == 32;
    EVP_MD_CTX_free(ctx);
    if (!good || !unchanged(s)) goto corrupt;
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < 32; ++i) {
        if (hex[i * 2] != digits[hash[i] >> 4] || hex[i * 2 + 1] != digits[hash[i] & 15]) goto corrupt;
    }
    memcpy(s->id, id, strlen(id) + 1);
    memcpy(s->expected_sha256, hex, 65);
    return 0;
corrupt:
    problem(error, error_length, "source changed, not regular, or SHA-256 mismatch");
    ib_source_close(s);
    return -1;
}
void ib_source_close(ib_source *s) { if (s && s->fd >= 0) { close(s->fd); s->fd = -1; } }
int ib_source_window(ib_source *s, uint64_t start, unsigned char *out,
                     size_t capacity, size_t *received) {
    if (!s || s->fd < 0 || !received || capacity > IB_MAX_WINDOW ||
        (capacity && !out) || start > s->bytes || !unchanged(s)) return -1;
    *received = 0;
    size_t want = (s->bytes - start < capacity) ? (size_t)(s->bytes - start) : capacity;
    while (*received < want) {
        if (start + *received > INT64_MAX) return -1;
        ssize_t n = pread(s->fd, out + *received, want - *received, (off_t)(start + *received));
        if (n > 0) *received += (size_t)n;
        else if (n < 0 && errno == EINTR) continue;
        else return -1;
    }
    return unchanged(s) ? 0 : -1;
}

typedef struct {char id[IB_ID_SIZE]; uint32_t index;} name_index;
static int compare_names(const void *a, const void *b) {
    return strcmp(((const name_index *)a)->id, ((const name_index *)b)->id);
}
static int compare_edge(const ib_strand *s,uint32_t a,uint32_t b,int incoming) {
    const ib_link *x=&s->links[a], *y=&s->links[b];
    uint32_t left=incoming?x->to:x->from,right=incoming?y->to:y->from;
    if(left!=right)return left<right?-1:1;
    int k=strcmp(x->kind,y->kind);
    if(k)return k;
    left=incoming?x->from:x->to;right=incoming?y->from:y->to;
    return (left>right)-(left<right);
}
/* Reentrant, bounded-memory stable merge sort. No global comparator state:
 * two independent read workers can build graph indexes concurrently. */
static int sort_edges(const ib_strand *s,uint32_t *items,int incoming) {
    size_t n=s->link_count;
    uint32_t *buffer=malloc((n?n:1)*sizeof(uint32_t));
    if(!buffer)return -1;
    uint32_t *src=items,*dst=buffer;
    for(size_t width=1;width<n;) {
        for(size_t start=0;start<n;) {
            size_t mid=start+((n-start)<width?(n-start):width);
            size_t end=mid+((n-mid)<width?(n-mid):width);
            size_t i=start,j=mid,k=start;
            while(i<mid&&j<end) {
                if(compare_edge(s,src[i],src[j],incoming)<=0)dst[k++]=src[i++];
                else dst[k++]=src[j++];
            }
            while(i<mid)dst[k++]=src[i++];
            while(j<end)dst[k++]=src[j++];
            start=end;
        }
        uint32_t *temp=src;src=dst;dst=temp;
        if(width>n/2)break;
        width*=2;
    }
    if(src!=items)memcpy(items,src,n*sizeof(uint32_t));
    free(buffer);return 0;
}
int ib_fragment_index(const ib_strand *s, const char *name, uint32_t *result) {
    if (!s || !name || !result) return -1;
    /* Cold debug/import lookup only: hot traversal uses direct ordinals. */
    for (size_t i = 0; i < s->fragment_count; ++i) {
        if (strcmp(s->fragments[i].id, name) == 0) { *result = (uint32_t)i; return 0; }
    }
    return -1;
}
static int parent_cycles(const ib_strand *s) {
    uint32_t *parent = calloc(s->fragment_count ? s->fragment_count : 1, sizeof(*parent));
    unsigned char *color = calloc(s->fragment_count ? s->fragment_count : 1, 1);
    if (!parent || !color) { free(parent); free(color); return -1; }
    for (size_t i = 0; i < s->fragment_count; ++i) parent[i] = UINT32_MAX;
    for (size_t i = 0; i < s->link_count; ++i) {
        const ib_link *e = &s->links[i];
        if (strcmp(e->kind, "parent") == 0) {
            if (e->from == e->to || parent[e->from] != UINT32_MAX) goto bad;
            parent[e->from] = e->to;
        }
    }
    for (size_t i = 0; i < s->fragment_count; ++i) {
        uint32_t current = (uint32_t)i;
        while (current != UINT32_MAX && color[current] == 0) {
            color[current] = 1;
            current = parent[current];
        }
        if (current != UINT32_MAX && color[current] == 1) goto bad;
        current = (uint32_t)i;
        while (current != UINT32_MAX && color[current] == 1) {
            color[current] = 2;
            current = parent[current];
        }
    }
    free(parent); free(color); return 0;
bad:
    free(parent); free(color); return -1;
}
int ib_strand_index(ib_strand *s, char *error, size_t error_length) {
    if (!s || (!s->sources && s->source_count) || (!s->fragments && s->fragment_count) ||
        (!s->visits && s->visit_count) || (!s->links && s->link_count) ||
        s->fragment_count > UINT32_MAX || s->link_count > UINT32_MAX ||
        s->visit_count > SIZE_MAX / sizeof(uint64_t) - 1) goto invalid;
    ib_strand_release(s);
    for (size_t i = 0; i < s->source_count; ++i) {
        if (s->sources[i].fd < 0 || !unchanged(&s->sources[i]) ||
            !plain_id(s->sources[i].id, 64)) goto invalid;
        for (size_t j = i+1; j < s->source_count; ++j)
            if (!strcmp(s->sources[i].id,s->sources[j].id)) goto invalid;
    }
    name_index *names = calloc(s->fragment_count ? s->fragment_count : 1, sizeof(*names));
    if (!names) goto invalid;
    for (size_t i = 0; i < s->fragment_count; ++i) {
        const ib_fragment *f = &s->fragments[i];
        if (!plain_id(f->id, 64) || f->source >= s->source_count ||
            f->start > f->end || f->end > s->sources[f->source].bytes) { free(names); goto invalid; }
        memcpy(names[i].id, f->id, strlen(f->id) + 1);
        names[i].index = (uint32_t)i;
    }
    qsort(names, s->fragment_count, sizeof(*names), compare_names);
    for (size_t i = 1; i < s->fragment_count; ++i)
        if (strcmp(names[i-1].id,names[i].id)==0) { free(names); goto invalid; }
    free(names);
    s->prefix = calloc(s->visit_count + 1, sizeof(uint64_t));
    if (!s->prefix) goto invalid;
    for (size_t i = 0; i < s->visit_count; ++i) {
        if (s->visits[i].fragment >= s->fragment_count) goto invalid;
        const ib_fragment *f = &s->fragments[s->visits[i].fragment];
        uint64_t length = f->end - f->start;
        if (s->prefix[i] > UINT64_MAX - length) goto invalid;
        s->prefix[i+1] = s->prefix[i] + length;
    }
    s->links_out = calloc(s->link_count ? s->link_count : 1, sizeof(uint32_t));
    s->links_in = calloc(s->link_count ? s->link_count : 1, sizeof(uint32_t));
    if (!s->links_out || !s->links_in) goto invalid;
    for (size_t i = 0; i < s->link_count; ++i) {
        if (s->links[i].from >= s->fragment_count || s->links[i].to >= s->fragment_count ||
            !plain_id(s->links[i].kind, 32)) goto invalid;
        s->links_out[i] = (uint32_t)i;
        s->links_in[i] = (uint32_t)i;
    }
    if(sort_edges(s,s->links_out,0)||sort_edges(s,s->links_in,1))goto invalid;
    for (size_t i = 1; i < s->link_count; ++i)
        if (compare_edge(s,s->links_out[i-1],s->links_out[i],0) == 0) goto invalid;
    if (parent_cycles(s)) goto invalid;
    return 0;
invalid:
    problem(error, error_length, "invalid source, fragment, strand, or link graph");
    if (s) ib_strand_release(s);
    return -1;
}
void ib_strand_release(ib_strand *s) {
    if (!s) return;
    free(s->prefix); s->prefix = NULL;
    free(s->links_out); s->links_out = NULL;
    free(s->links_in); s->links_in = NULL;
}
uint64_t ib_strand_bytes(const ib_strand *s) {
    return s && s->prefix ? s->prefix[s->visit_count] : 0;
}
int ib_strand_window(ib_strand *s, uint64_t offset, unsigned char *out,
                     size_t capacity, size_t *received) {
    if (!s || !s->prefix || !received || capacity > IB_MAX_WINDOW ||
        (capacity && !out) || offset > ib_strand_bytes(s)) return -1;
    *received = 0;
    if (!capacity || offset == ib_strand_bytes(s)) return 0;
    size_t lo = 0, hi = s->visit_count;
    while (lo < hi) {
        size_t middle = lo + (hi-lo)/2;
        if (s->prefix[middle+1] <= offset) lo = middle+1;
        else hi = middle;
    }
    for (size_t i = lo; i < s->visit_count && *received < capacity; ++i) {
        const ib_fragment *f = &s->fragments[s->visits[i].fragment];
        uint64_t local = offset > s->prefix[i] ? offset-s->prefix[i] : 0;
        uint64_t remain = f->end-f->start-local;
        size_t wanted = (remain < capacity - *received) ? (size_t)remain : capacity-*received;
        if (wanted) {
            size_t got = 0;
            ib_source *source = &s->sources[f->source];
            if (ib_source_window(source, f->start+local, out+*received, wanted, &got) || got != wanted) return -1;
            *received += got;
        }
        offset = s->prefix[i+1];
    }
    return 0;
}
static size_t edges(const ib_strand *s, uint32_t node, const char *kind,
                    uint32_t *out, size_t capacity, int incoming) {
    if (!s || !kind || !s->links_out || !s->links_in || node >= s->fragment_count) return 0;
    const uint32_t *index = incoming ? s->links_in : s->links_out;
    size_t lo=0, hi=s->link_count;
    while(lo<hi) {
        size_t mid=lo+(hi-lo)/2;
        const ib_link *e=&s->links[index[mid]];
        uint32_t candidate=incoming ? e->to : e->from;
        if (candidate < node || (candidate == node && strcmp(e->kind,kind)<0)) lo=mid+1;
        else hi=mid;
    }
    size_t total=0;
    while (lo<s->link_count) {
        const ib_link *e=&s->links[index[lo++]];
        if ((incoming ? e->to : e->from) != node || strcmp(e->kind,kind)!=0) break;
        if (out && total<capacity) out[total]=incoming ? e->from : e->to;
        ++total;
    }
    return total;
}
size_t ib_links_from(const ib_strand *s, uint32_t fragment, const char *kind,
                     uint32_t *out, size_t capacity) { return edges(s,fragment,kind,out,capacity,0); }
size_t ib_links_to(const ib_strand *s, uint32_t fragment, const char *kind,
                   uint32_t *out, size_t capacity) { return edges(s,fragment,kind,out,capacity,1); }

/* Derived index serialization has one fixed little-endian grammar. Original
 * source and Idriç graph remain canonical. This is deliberately not a C struct
 * dump: alignment, pointer size and host byte order cannot affect it. */
static void le32(unsigned char *b, uint32_t x) {
    for (unsigned i=0; i<4; ++i) b[i]=(unsigned char)(x>>(i*8));
}
static void le64(unsigned char *b, uint64_t x) {
    for (unsigned i=0; i<8; ++i) b[i]=(unsigned char)(x>>(i*8));
}
static int transfer(int fd, const void *bytes, size_t count, int verify) {
    const unsigned char *p = bytes;
    size_t done=0;
    unsigned char received[IB_MAX_WINDOW];
    while (done<count) {
        size_t batch=count-done < sizeof received ? count-done : sizeof received;
        ssize_t n;
        if (verify) {
            n=read(fd,received,batch);
            if (n > 0 && memcmp(received,p+done,(size_t)n)) return -1;
        } else n=write(fd,p+done,batch);
        if (n>0) done+=(size_t)n;
        else if (n<0 && errno==EINTR) continue;
        else return -1;
    }
    return 0;
}
static int transfer32(int fd, uint32_t v, int verify) {
    unsigned char b[4]; le32(b,v); return transfer(fd,b,4,verify);
}
static int transfer64(int fd, uint64_t v, int verify) {
    unsigned char b[8]; le64(b,v); return transfer(fd,b,8,verify);
}
static int index_serial(const ib_strand *s, int fd, const char *canonical, int verify) {
    if (!s || !s->prefix || !s->links_out || !s->links_in || !digest_valid(canonical)) return -1;
    if (transfer(fd,"IBSTRND1",8,verify) || transfer(fd,canonical,64,verify) ||
        transfer64(fd,s->source_count,verify) || transfer64(fd,s->fragment_count,verify) ||
        transfer64(fd,s->visit_count,verify) || transfer64(fd,s->link_count,verify)) return -1;
    for (size_t i=0;i<s->source_count;++i) {
        const ib_source *src=&s->sources[i];
        if (!unchanged(src) || transfer(fd,src->id,IB_ID_SIZE,verify) ||
            transfer(fd,src->expected_sha256,64,verify) ||
            transfer64(fd,src->bytes,verify)) return -1;
    }
    for (size_t i=0;i<s->fragment_count;++i) {
        const ib_fragment *f=&s->fragments[i];
        if (transfer(fd,f->id,IB_ID_SIZE,verify) || transfer32(fd,f->source,verify) ||
            transfer64(fd,f->start,verify) || transfer64(fd,f->end,verify)) return -1;
    }
    for (size_t i=0;i<s->visit_count;++i)
        if (transfer32(fd,s->visits[i].fragment,verify)) return -1;
    for (size_t i=0;i<=s->visit_count;++i)
        if (transfer64(fd,s->prefix[i],verify)) return -1;
    for (size_t i=0;i<s->link_count;++i) {
        const ib_link *e=&s->links[i];
        if (transfer32(fd,e->from,verify) || transfer32(fd,e->to,verify) ||
            transfer(fd,e->kind,IB_KIND_SIZE,verify)) return -1;
    }
    for (size_t i=0;i<s->link_count;++i)
        if (transfer32(fd,s->links_out[i],verify)) return -1;
    for (size_t i=0;i<s->link_count;++i)
        if (transfer32(fd,s->links_in[i],verify)) return -1;
    return 0;
}
int ib_index_write(const ib_strand *s, const char *path, const char *digest,
                   char *error, size_t error_length) {
    if (!path || !digest_valid(digest) || strlen(path) > 3900) goto invalid;
    size_t n=strlen(path);
    char *temp=malloc(n+16);
    if (!temp) goto invalid;
    (void)snprintf(temp,n+16,"%s.XXXXXX",path);
    int fd=mkstemp(temp);
    if (fd < 0) { free(temp); goto invalid; }
    int failed=index_serial(s,fd,digest,0) || fsync(fd);
    if (close(fd)) failed=1;
    if (!failed) {
        /* Linux/Android explicit no-replace publication: no lost old index. */
#ifdef __linux__
        if (renameat2(AT_FDCWD,temp,AT_FDCWD,path,RENAME_NOREPLACE)) failed=1;
#else
        failed=1;
#endif
        if (!failed) {
            /* Durable namespace publication requires syncing the directory,
             * not merely fsync of the completed staging file. */
            char parent[4096];
            memcpy(parent,path,n+1);
            char *slash=strrchr(parent,'/');
            if (!slash) strcpy(parent,".");
            else if (slash==parent) parent[1]=0;
            else *slash=0;
            int parentfd=open(parent,O_RDONLY|O_CLOEXEC|O_DIRECTORY|O_NOFOLLOW);
            if (parentfd<0 || fsync(parentfd)) failed=1;
            if (parentfd>=0 && close(parentfd)) failed=1;
        }
    }
    if (failed) unlink(temp);
    free(temp);
    if (failed) goto invalid;
    return 0;
invalid:
    problem(error,error_length,"index publication failed or unsupported");
    return -1;
}
int ib_index_check(const ib_strand *s, const char *path, const char *digest,
                   char *error, size_t error_length) {
    int fd = path ? open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK) : -1;
    if (fd<0) goto invalid;
    struct stat st;
    if (fstat(fd,&st) || !S_ISREG(st.st_mode)) { close(fd); goto invalid; }
    int failed=index_serial(s,fd,digest,1);
    unsigned char tail;
    if (!failed && read(fd,&tail,1)!=0) failed=1;
    if (close(fd)) failed=1;
    if (failed) goto invalid;
    return 0;
invalid:
    problem(error,error_length,"derived index disagrees with admitted source/graph");
    return -1;
}

static int read_exact(int fd, void *out, size_t count) {
    size_t done=0;
    while(done<count) {
        ssize_t n=read(fd,(unsigned char *)out+done,count-done);
        if(n>0)done+=(size_t)n;
        else if(n<0&&errno==EINTR)continue;
        else return -1;
    }
    return 0;
}
static uint32_t decode32(const unsigned char *p) {
    uint32_t result=0;for(int i=3;i>=0;i--)result=(result<<8)|p[i];return result;
}
static uint64_t decode64(const unsigned char *p) {
    uint64_t result=0;for(int i=7;i>=0;i--)result=(result<<8)|p[i];return result;
}
static int read32(int fd,uint32_t *out) {
    unsigned char p[4];if(read_exact(fd,p,sizeof p))return -1;
    *out=decode32(p);return 0;
}
static int read64(int fd,uint64_t *out) {
    unsigned char p[8];if(read_exact(fd,p,sizeof p))return -1;
    *out=decode64(p);return 0;
}
void ib_index_loaded_free(ib_strand *s) {
    if(!s)return;
    ib_strand_release(s);
    free(s->fragments);free(s->visits);free(s->links);
    s->fragments=NULL;s->visits=NULL;s->links=NULL;
    s->fragment_count=s->visit_count=s->link_count=0;
}
static int index_load_untrusted(ib_strand *out, ib_source *sources, size_t source_count,
                  const char *path, const char *canonical_digest,
                  char *error, size_t error_length) {
    if(!out||!sources||!path||!digest_valid(canonical_digest))goto bad;
    memset(out,0,sizeof(*out));
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0)goto bad;
    struct stat st;
    if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0){close(fd);goto bad;}
    unsigned char head[8],digest[64];
    uint64_t dimensions[4];
    int invalid=read_exact(fd,head,8)||memcmp(head,"IBSTRND1",8)||
                read_exact(fd,digest,64)||memcmp(digest,canonical_digest,64);
    for(size_t i=0;i<4&&!invalid;i++)invalid=read64(fd,&dimensions[i]);
    if(invalid||dimensions[0]!=source_count||dimensions[1]>1000000||
       dimensions[2]>1000000||dimensions[3]>2000000){close(fd);goto bad;}
    out->sources=sources;out->source_count=source_count;
    out->fragment_count=(size_t)dimensions[1];
    out->visit_count=(size_t)dimensions[2];
    out->link_count=(size_t)dimensions[3];
    out->fragments=calloc(out->fragment_count?out->fragment_count:1,sizeof(*out->fragments));
    out->visits=calloc(out->visit_count?out->visit_count:1,sizeof(*out->visits));
    out->links=calloc(out->link_count?out->link_count:1,sizeof(*out->links));
    uint64_t *saved_prefix=calloc(out->visit_count+1,sizeof(uint64_t));
    uint32_t *saved_out=calloc(out->link_count?out->link_count:1,sizeof(uint32_t));
    uint32_t *saved_in=calloc(out->link_count?out->link_count:1,sizeof(uint32_t));
    if(!out->fragments||!out->visits||!out->links||!saved_prefix||!saved_out||!saved_in)invalid=1;
    for(size_t i=0;i<source_count&&!invalid;i++){
        char id[IB_ID_SIZE]={0};char sha[64]={0};uint64_t length=0;
        invalid=read_exact(fd,id,sizeof id)||read_exact(fd,sha,sizeof sha)||read64(fd,&length);
        if(!invalid && (!plain_id(id,64)||strcmp(id,sources[i].id)||
                        memcmp(sha,sources[i].expected_sha256,64)||length!=sources[i].bytes))invalid=1;
    }
    for(size_t i=0;i<out->fragment_count&&!invalid;i++) {
        ib_fragment *f=&out->fragments[i];
        invalid=read_exact(fd,f->id,sizeof f->id)||read32(fd,&f->source)||
                read64(fd,&f->start)||read64(fd,&f->end);
    }
    for(size_t i=0;i<out->visit_count&&!invalid;i++)invalid=read32(fd,&out->visits[i].fragment);
    for(size_t i=0;i<=out->visit_count&&!invalid;i++)invalid=read64(fd,&saved_prefix[i]);
    for(size_t i=0;i<out->link_count&&!invalid;i++){
        ib_link *e=&out->links[i];
        invalid=read32(fd,&e->from)||read32(fd,&e->to)||
                read_exact(fd,e->kind,sizeof e->kind);
    }
    for(size_t i=0;i<out->link_count&&!invalid;i++)invalid=read32(fd,&saved_out[i]);
    for(size_t i=0;i<out->link_count&&!invalid;i++)invalid=read32(fd,&saved_in[i]);
    unsigned char extra;
    if(!invalid && read(fd,&extra,1)!=0)invalid=1;
    if(!invalid)invalid=ib_strand_index(out,error,error_length);
    if(!invalid){
        for(size_t i=0;i<=out->visit_count;i++)if(saved_prefix[i]!=out->prefix[i]){invalid=1;break;}
        for(size_t i=0;i<out->link_count;i++)
            if(saved_out[i]!=out->links_out[i]||saved_in[i]!=out->links_in[i]){invalid=1;break;}
    }
    free(saved_prefix);free(saved_out);free(saved_in);
    if(close(fd))invalid=1;
    if(invalid){ib_index_loaded_free(out);goto bad;}
    return 0;
bad:
    problem(error,error_length,"derived index is invalid, stale or cannot be opened");
    return -1;
}

/* Deliberately small K&R-style position record; never read an unbounded line
 * or use JSON, a database, or a renderer-owned session file. */
static int position_parse(const char *b,size_t n,ib_read_position *p) {
    if(!b||n>=256)return -1;
    char value[256];memcpy(value,b,n);value[n]=0;
    if(n==0||b[n-1]!='\n')return -1;
    const char *fixed="position 1\nsnapshot ";size_t prefix=strlen(fixed);
    if(strncmp(value,fixed,prefix))return -1;
    char *cursor=value+prefix,*newline=strchr(cursor,'\n');
    if(!newline||newline-cursor!=64)return -1;
    *newline=0;
    if(!digest_valid(cursor))return -1;
    memcpy(p->snapshot,cursor,65);
    cursor=newline+1;
    if(strncmp(cursor,"object ",7))return -1;
    cursor+=7;newline=strchr(cursor,'\n');if(!newline)return -1;
    *newline=0;
    if(!plain_id(cursor,64))return -1;
    memset(p->object,0,sizeof p->object);
    memcpy(p->object,cursor,strlen(cursor)+1);
    cursor=newline+1;
    if(strncmp(cursor,"offset ",7))return -1;
    cursor+=7;newline=strchr(cursor,'\n');if(!newline||newline[1])return -1;
    *newline=0;
    if(!*cursor||(cursor[0]=='0'&&cursor[1]))return -1;
    uint64_t number=0;
    for(;*cursor;cursor++) {
        if(*cursor<'0'||*cursor>'9'||number>(UINT64_MAX-(unsigned)(*cursor-'0'))/10)return -1;
        number=number*10+(unsigned)(*cursor-'0');
    }
    p->offset=number;
    return 0;
}
int ib_position_read(const char *path, ib_read_position *p) {
    if(!path||!p)return -1;
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
    if(fd<0)return errno==ENOENT?1:-1;
    struct stat st;
    if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0||st.st_size>255){close(fd);return -1;}
    char b[256];
    int fail=read_exact(fd,b,(size_t)st.st_size);
    unsigned char extra;
    if(!fail&&read(fd,&extra,1)!=0)fail=1;
    if(close(fd))fail=1;
    if(fail)return -1;
    return position_parse(b,(size_t)st.st_size,p);
}
int ib_position_matches(const ib_read_position *p,const char *snapshot,const char *object,uint64_t bytes) {
    return p && digest_valid(snapshot) && plain_id(object,64) &&
        !strcmp(p->snapshot,snapshot) && !strcmp(p->object,object) && p->offset<=bytes;
}
int ib_position_write(const char *path,const ib_read_position *p) {
    if(!path||!p||!digest_valid(p->snapshot)||!plain_id(p->object,64)||strlen(path)>3900)return -1;
    char record[256];
    int n=snprintf(record,sizeof record,"position 1\nsnapshot %s\nobject %s\noffset %" PRIu64 "\n",
                   p->snapshot,p->object,p->offset);
    if(n<0||(size_t)n>=sizeof record)return -1;
    size_t length=strlen(path);
    char *temp=malloc(length+16);if(!temp)return -1;
    snprintf(temp,length+16,"%s.XXXXXX",path);
    int fd=mkstemp(temp);if(fd<0){free(temp);return -1;}
    int failed=transfer(fd,record,(size_t)n,0)||fsync(fd);
    if(close(fd))failed=1;
    if(!failed && rename(temp,path))failed=1;
    if(!failed){
        char parent[4096];memcpy(parent,path,length+1);
        char *slash=strrchr(parent,'/');
        if(!slash)strcpy(parent,".");
        else if(slash==parent)parent[1]=0;
        else *slash=0;
        int dir=open(parent,O_RDONLY|O_NOFOLLOW|O_DIRECTORY|O_CLOEXEC);
        if(dir<0||fsync(dir))failed=1;
        if(dir>=0&&close(dir))failed=1;
    }
    if(failed)unlink(temp);
    free(temp);
    return failed?-1:0;
}

/* Explicit coordinate-unit conversion for Idriç's Unicode-codepoint graph.
 * A failed conversion returns no usable mappings; callers must discard bytes.
 * No conversion is needed for the opaque conversation byte source representation. */
int ib_utf8_map_spans(ib_source *source, const ib_offset_span *cp,
                      size_t count, ib_offset_span *result,
                      char *error, size_t error_length) {
    if (!source || source->fd < 0 || (count && (!cp || !result))) goto invalid;
    for (size_t i=0; i<count; ++i) {
        if (cp[i].start > cp[i].end ||
            (i && cp[i].start < cp[i-1].end)) goto invalid;
    }
    size_t active=0;
    int begun=0;
    uint64_t character=0, cursor=0, window_start=0;
    unsigned char window[IB_MAX_WINDOW];
    size_t available=0;
    unsigned char leading=0;
    unsigned following=0, width=0;
    while (cursor <= source->bytes) {
        if (following==0) {
            for (;;) {
                if (active==count) break;
                if (!begun) {
                    if (character < cp[active].start) break;
                    if (character != cp[active].start) goto invalid;
                    result[active].start=cursor;
                    begun=1;
                }
                if (character < cp[active].end) break;
                if (character != cp[active].end) goto invalid;
                result[active].end=cursor;
                active++;
                begun=0;
            }
        }
        if (cursor==source->bytes) break;
        if (!available || cursor>=window_start+available) {
            window_start=cursor;
            if (ib_source_window(source,window_start,window,sizeof window,&available) ||
                available==0) goto invalid;
        }
        unsigned char byte=window[(size_t)(cursor-window_start)];
        if (following) {
            if ((byte & 0xc0)!=0x80) goto invalid;
            if (following == width-1 &&
                ((leading==0xe0 && byte<0xa0) ||
                 (leading==0xed && byte>=0xa0) ||
                 (leading==0xf0 && byte<0x90) ||
                 (leading==0xf4 && byte>=0x90))) goto invalid;
            following--;
            if (!following) character++;
        } else if (byte<=0x7f) {
            character++;
        } else {
            leading=byte;
            if (byte>=0xc2 && byte<=0xdf) width=2;
            else if (byte>=0xe0 && byte<=0xef) width=3;
            else if (byte>=0xf0 && byte<=0xf4) width=4;
            else goto invalid;
            following=width-1;
        }
        cursor++;
    }
    if (following || active!=count) goto invalid;
    return 0;
invalid:
    problem(error,error_length,"invalid UTF-8, source changed, or code-point span out of range");
    return -1;
}

int ib_index_load_checked(ib_strand *out, const ib_strand *canonical,
                          const char *path, const char *digest,
                          char *error, size_t error_length) {
    if (!out || !canonical || !canonical->prefix ||
        ib_index_check(canonical,path,digest,error,error_length)) return -1;
    return index_load_untrusted(out,canonical->sources,canonical->source_count,
                                path,digest,error,error_length);
}
