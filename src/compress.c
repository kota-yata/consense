#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len) *out_len = rd;
    return buf;
}

static int is_ws(char c) { return c==' '||c=='\t'||c=='\r'||c=='\n'; }
static size_t skip_ws(const char *s, size_t i, size_t n) { while (i<n && is_ws(s[i])) i++; return i; }
static size_t skip_string(const char *s, size_t i, size_t n) { i++; while (i<n) { char c=s[i++]; if (c=='\\') { if (i<n) i++; continue; } if (c=='"') break; } return i; }
static size_t skip_number(const char *s, size_t i, size_t n) { if (i<n && (s[i]=='-'||s[i]=='+')) i++; while (i<n && ((s[i]>='0'&&s[i]<='9')||s[i]=='.'||s[i]=='e'||s[i]=='E'||s[i]=='+'||s[i]=='-')) i++; return i; }
static size_t skip_literal(const char *s, size_t i, size_t n) { while (i<n && ((s[i]>='A'&&s[i]<='Z')||(s[i]>='a'&&s[i]<='z'))) i++; return i; }
static size_t skip_value(const char *s, size_t i, size_t n);
static size_t skip_array(const char *s, size_t i, size_t n) { i++; i=skip_ws(s,i,n); if (i<n && s[i]==']') return i+1; while (i<n) { i=skip_value(s,i,n); i=skip_ws(s,i,n); if (i<n && s[i]==',') { i++; i=skip_ws(s,i,n); continue; } if (i<n && s[i]==']') return i+1; break; } return i; }
static size_t skip_object(const char *s, size_t i, size_t n) { i++; i=skip_ws(s,i,n); if (i<n && s[i]=='}') return i+1; while (i<n) { if (s[i] != '"') return i; i=skip_string(s,i,n); i=skip_ws(s,i,n); if (i<n && s[i]==':') i++; else return i; i=skip_ws(s,i,n); i=skip_value(s,i,n); i=skip_ws(s,i,n); if (i<n && s[i]==',') { i++; i=skip_ws(s,i,n); continue; } if (i<n && s[i]=='}') return i+1; break; } return i; }
static size_t skip_value(const char *s, size_t i, size_t n) { if (i>=n) return i; char c=s[i]; if (c=='"') return skip_string(s,i,n); if (c=='{') return skip_object(s,i,n); if (c=='[') return skip_array(s,i,n); if (c=='t'||c=='f'||c=='n') return skip_literal(s,i,n); return skip_number(s,i,n); }
static int key_eq(const char *s, size_t k0, size_t k1, const char *key) { size_t len=k1-k0; size_t klen=strlen(key); return len==klen && strncmp(s+k0, key, len)==0; }
static void print_raw(FILE *out, const char *s, size_t a, size_t b) { fwrite(s+a, 1, b-a, out); }

static void print_pages(FILE *out, const char *s, size_t a, size_t b) {
    fputs("\"pages\":[", out);
    size_t i=a; i=skip_ws(s,i,b); if (i<b && s[i]=='[') i++;
    int first_page=1;
    while (1) {
        i=skip_ws(s,i,b); if (i>=b || s[i]==']') break; size_t po=i; size_t pend=skip_object(s, po, b);
        size_t j=po+1; size_t title_a=0,title_b=0,id_a=0,id_b=0,created_a=0,created_b=0,updated_a=0,updated_b=0,views_a=0,views_b=0,lines_a=0,lines_b=0;
        while (j<pend) {
            j=skip_ws(s,j,pend); if (j>=pend || s[j]=='}') break; if (s[j] != '"') { j++; continue; }
            size_t k0=j+1; size_t k1=skip_string(s,j,pend)-1; j=k1+1; j=skip_ws(s,j,pend); if (j<pend && s[j]==':') j++; j=skip_ws(s,j,pend);
            size_t v0=j; size_t v1=skip_value(s,j,pend);
            if (key_eq(s,k0,k1,"title")) { title_a=v0; title_b=v1; }
            else if (key_eq(s,k0,k1,"id")) { id_a=v0; id_b=v1; }
            else if (key_eq(s,k0,k1,"created")) { created_a=v0; created_b=v1; }
            else if (key_eq(s,k0,k1,"updated")) { updated_a=v0; updated_b=v1; }
            else if (key_eq(s,k0,k1,"views")) { views_a=v0; views_b=v1; }
            else if (key_eq(s,k0,k1,"lines")) { lines_a=v0; lines_b=v1; }
            j=v1; j=skip_ws(s,j,pend); if (j<pend && s[j]==',') j++;
        }
        if (!first_page) fputc(',', out); first_page=0; fputc('{', out);
        fputs("\"title\":", out); if (title_a) print_raw(out, s, title_a, title_b); else fputs("\"\"", out);
        if (id_a) { fputs(",\"id\":", out); print_raw(out, s, id_a, id_b); }
        if (created_a) { fputs(",\"created\":", out); print_raw(out, s, created_a, created_b); }
        if (updated_a) { fputs(",\"updated\":", out); print_raw(out, s, updated_a, updated_b); }
        if (views_a) { fputs(",\"views\":", out); print_raw(out, s, views_a, views_b); }
        fputs(",\"content\":\"", out);
        // Build body by joining line texts with \n, dropping duplicated title line if present
        int first_line=1; if (lines_a && s[lines_a]=='[') {
            size_t li=lines_a+1; while (1) {
                li=skip_ws(s,li,lines_b); if (li>=lines_b || s[li]==']') break; size_t lobj=li; size_t lobj_end=skip_object(s, lobj, lines_b); li=lobj_end;
                size_t pj=lobj+1; size_t lv0=0,lv1=0; while (pj<lobj_end) {
                    pj=skip_ws(s,pj,lobj_end); if (pj>=lobj_end||s[pj]=='}') break; if (s[pj] != '"') { pj++; continue; }
                    size_t lk0=pj+1; size_t lk1=skip_string(s,pj,lobj_end)-1; pj=lk1+1; pj=skip_ws(s,pj,lobj_end); if (pj<lobj_end && s[pj]==':') pj++; pj=skip_ws(s,pj,lobj_end);
                    size_t v0=pj; size_t v1=skip_value(s,pj,lobj_end); if (key_eq(s,lk0,lk1,"text")) { lv0=v0; lv1=v1; break; }
                    pj=v1; pj=skip_ws(s,pj,lobj_end); if (pj<lobj_end && s[pj]==',') pj++;
                }
                if (lv0 && s[lv0]=='"') {
                    // Skip duplicated title line if first and identical to title
                    if (first_line && title_a && s[title_a]=='"' && (lv1-lv0)==(title_b-title_a) && strncmp(s+lv0, s+title_a, lv1-lv0)==0) {
                        first_line = 0; // consumed first line (title), but do not write
                        continue;
                    }
                    if (!first_line) { fputc('\\', out); fputc('n', out); }
                    first_line=0;
                    print_raw(out, s, lv0+1, lv1-1);
                }
            }
        }
        fputc('"', out);

        // Note: previously we emitted a lineMeta array here. Removed to shrink output size.

        // linksLc if present
        size_t links_a=0, links_b=0; {
            size_t pj=po+1; while (pj<pend) {
                pj=skip_ws(s,pj,pend); if (pj>=pend || s[pj]=='}') break; if (s[pj] != '"') { pj++; continue; }
                size_t lk0=pj+1; size_t lk1=skip_string(s,pj,pend)-1; pj=lk1+1; pj=skip_ws(s,pj,pend); if (pj<pend && s[pj]==':') pj++; pj=skip_ws(s,pj,pend);
                size_t v0=pj; size_t v1=skip_value(s,pj,pend);
                if (key_eq(s,lk0,lk1,"linksLc")) { links_a=v0; links_b=v1; break; }
                pj=v1; pj=skip_ws(s,pj,pend); if (pj<pend && s[pj]==',') pj++;
            }
        }
        if (links_a) { fputs(",\"linksLc\":", out); print_raw(out, s, links_a, links_b); }
        fputc('}', out);
        i=skip_ws(s,pend,b); if (i<b && s[i]==',') { i++; continue; }
    }
    fputc(']', out);
}

int compress_backup(const char *input_path, const char *output_path) {
    size_t len=0; char *src=read_file(input_path, &len); if (!src) { fprintf(stderr, "Failed to read %s\n", input_path); return 1; }
    size_t i=0,n=len; i=skip_ws(src,i,n); if (i>=n || src[i] != '{') { fprintf(stderr, "Invalid JSON (no root object)\n"); free(src); return 1; }
    size_t root_beg=i; size_t root_end=skip_object(src,i,n);
    FILE *out=stdout; if (output_path && *output_path) { out=fopen(output_path, "wb"); if (!out) { fprintf(stderr, "Failed to open %s\n", output_path); free(src); return 1; } }
    fputc('{', out); int wrote=0; size_t j=root_beg+1; while (j<root_end) {
        j=skip_ws(src,j,root_end); if (j>=root_end || src[j]=='}') break; if (src[j] != '"') { j++; continue; }
        size_t k0=j+1; size_t k1=skip_string(src,j,root_end)-1; j=k1+1; j=skip_ws(src,j,root_end); if (j<root_end && src[j]==':') j++; j=skip_ws(src,j,root_end);
        size_t v0=j; size_t v1=skip_value(src,j,root_end);
        if (key_eq(src,k0,k1,"name")) { if (wrote++) fputc(',', out); fputs("\"name\":", out); print_raw(out, src, v0, v1); }
        else if (key_eq(src,k0,k1,"displayName")) { if (wrote++) fputc(',', out); fputs("\"displayName\":", out); print_raw(out, src, v0, v1); }
        else if (key_eq(src,k0,k1,"exported")) { if (wrote++) fputc(',', out); fputs("\"exported\":", out); print_raw(out, src, v0, v1); }
        else if (key_eq(src,k0,k1,"users")) { if (wrote++) fputc(',', out); fputs("\"users\":", out); print_raw(out, src, v0, v1); }
        else if (key_eq(src,k0,k1,"pages")) { if (wrote++) fputc(',', out); print_pages(out, src, v0, v1); }
        j=v1; j=skip_ws(src,j,root_end); if (j<root_end && src[j]==',') j++;
    }
    fputc('}', out); fputc('\n', out);
    if (out!=stdout) fclose(out); free(src); return 0;
}
