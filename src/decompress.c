#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static char hexval(char c) {
    if (c>='0' && c<='9') return (char)(c-'0');
    if (c>='a' && c<='f') return (char)(10 + c - 'a');
    if (c>='A' && c<='F') return (char)(10 + c - 'A');
    return 0;
}

static void append_utf8(char **buf, size_t *cap, size_t *len, unsigned codepoint) {
    if (*len + 4 >= *cap) { *cap = (*cap==0?64:(*cap*2)); *buf = (char*)realloc(*buf, *cap); }
    if (codepoint <= 0x7F) { (*buf)[(*len)++] = (char)codepoint; }
    else if (codepoint <= 0x7FF) { (*buf)[(*len)++] = (char)(0xC0 | (codepoint>>6)); (*buf)[(*len)++] = (char)(0x80 | (codepoint & 0x3F)); }
    else if (codepoint <= 0xFFFF) { (*buf)[(*len)++] = (char)(0xE0 | (codepoint>>12)); (*buf)[(*len)++] = (char)(0x80 | ((codepoint>>6) & 0x3F)); (*buf)[(*len)++] = (char)(0x80 | (codepoint & 0x3F)); }
    else { (*buf)[(*len)++] = (char)(0xF0 | (codepoint>>18)); (*buf)[(*len)++] = (char)(0x80 | ((codepoint>>12)&0x3F)); (*buf)[(*len)++] = (char)(0x80 | ((codepoint>>6)&0x3F)); (*buf)[(*len)++] = (char)(0x80 | (codepoint & 0x3F)); }
}

static char *json_unescape_string(const char *s, size_t a, size_t b) {
    // s[a] == '"', s[b-1] == '"'
    size_t i = a+1, n=b-1; char *out=NULL; size_t cap=0, len=0;
    while (i<n) {
        char c = s[i++];
        if (c=='\\' && i<n) {
            char e = s[i++];
            switch (e) {
                case '"': case '\\': case '/': { if (len+1>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]=e; break; }
                case 'b': { if (len+1>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]='\b'; break; }
                case 'f': { if (len+1>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]='\f'; break; }
                case 'n': { if (len+1>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]='\n'; break; }
                case 'r': { if (len+1>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]='\r'; break; }
                case 't': { if (len+1>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]='\t'; break; }
                case 'u': {
                    if (i+4<=n) {
                        unsigned cp = (hexval(s[i])<<12) | (hexval(s[i+1])<<8) | (hexval(s[i+2])<<4) | hexval(s[i+3]);
                        i+=4; append_utf8(&out,&cap,&len,cp);
                    }
                    break;
                }
                default: { if (len+2>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]='\\'; out[len++]=e; break; }
            }
        } else {
            if (len+1>=cap) { cap=cap?cap*2:64; out=(char*)realloc(out,cap);} out[len++]=c;
        }
    }
    if (out==NULL) { out=(char*)malloc(1); len=0; }
    out[len]='\0';
    return out;
}

static void json_escape_and_print(FILE *out, const char *s) {
    fputc('"', out);
    for (const unsigned char *p=(const unsigned char*)s; *p; ++p) {
        unsigned char c = *p;
        switch (c) {
            case '\\': fputs("\\\\", out); break;
            case '"': fputs("\\\"", out); break;
            case '\b': fputs("\\b", out); break;
            case '\f': fputs("\\f", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if (c < 0x20) {
                    // Control char -> \u00XX
                    fprintf(out, "\\u%04X", c);
                } else {
                    fputc(c, out);
                }
        }
    }
    fputc('"', out);
}

static char *extract_first_user_id(const char *s, size_t a, size_t b) {
    // s[a..b) is users array
    size_t i = a; i=skip_ws(s,i,b); if (i<b && s[i]=='[') i++;
    i=skip_ws(s,i,b); if (i>=b || s[i]!= '{') return NULL;
    size_t uend = skip_object(s, i, b);
    size_t j = i+1;
    while (j < uend) {
        j=skip_ws(s,j,uend); if (j>=uend || s[j]=='}') break; if (s[j] != '"') { j++; continue; }
        size_t k0=j+1; size_t k1=skip_string(s,j,uend)-1; j=k1+1; j=skip_ws(s,j,uend); if (j<uend && s[j]==':') j++; j=skip_ws(s,j,uend);
        size_t v0=j; size_t v1=skip_value(s,j,uend);
        if (key_eq(s,k0,k1,"id") && v0<v1 && s[v0]=='"') {
            return json_unescape_string(s, v0, v1);
        }
        j=v1; j=skip_ws(s,j,uend); if (j<uend && s[j]==',') j++;
    }
    return NULL;
}

int decompress_backup(const char *input_path, const char *output_path) {
    // Read input
    FILE *f = fopen(input_path, "rb");
    if (!f) { fprintf(stderr, "Failed to open %s\n", input_path); return 1; }
    fseek(f,0,SEEK_END); long n = ftell(f); if (n<0) { fclose(f); return 1; } rewind(f);
    char *buf = (char*)malloc((size_t)n+1); if (!buf) { fclose(f); return 1; }
    size_t rd=fread(buf,1,(size_t)n,f); fclose(f); buf[rd]='\0'; size_t len=rd;

    // Find root
    size_t i=0; i=skip_ws(buf,i,len); if (i>=len || buf[i] != '{') { free(buf); fprintf(stderr, "Invalid JSON\n"); return 1; }
    size_t root_beg=i; size_t root_end=skip_object(buf,i,len);

    // Prepare output
    FILE *out = stdout; if (output_path && *output_path) { out=fopen(output_path, "wb"); if (!out) { free(buf); return 1; } }

    // Discover default userId from users[0].id if available
    char *default_user_id = NULL;
    {
        size_t sj = root_beg+1;
        while (sj < root_end) {
            sj = skip_ws(buf, sj, root_end); if (sj>=root_end || buf[sj]=='}') break; if (buf[sj] != '"') { sj++; continue; }
            size_t sk0=sj+1; size_t sk1=skip_string(buf,sj,root_end)-1; sj=sk1+1; sj=skip_ws(buf,sj,root_end); if (sj<root_end && buf[sj]==':') sj++; sj=skip_ws(buf,sj,root_end);
            size_t sv0=sj; size_t sv1=skip_value(buf,sj,root_end);
            if (key_eq(buf,sk0,sk1, "users")) {
                default_user_id = extract_first_user_id(buf, sv0, sv1);
                break;
            }
            sj=sv1; sj=skip_ws(buf,sj,root_end); if (sj<root_end && buf[sj]==',') sj++;
        }
    }

    fputc('{', out); int wrote=0;
    size_t j=root_beg+1;
    while (j<root_end) {
        j=skip_ws(buf,j,root_end); if (j>=root_end || buf[j]=='}') break; if (buf[j] != '"') { j++; continue; }
        size_t k0=j+1; size_t k1=skip_string(buf,j,root_end)-1; j=k1+1; j=skip_ws(buf,j,root_end); if (j<root_end && buf[j]==':') j++; j=skip_ws(buf,j,root_end);
        size_t v0=j; size_t v1=skip_value(buf,j,root_end);
        if (key_eq(buf,k0,k1,"name") || key_eq(buf,k0,k1,"displayName") || key_eq(buf,k0,k1,"exported")) {
            if (wrote++) fputc(',', out);
            fwrite(buf+k0-1,1,(k1-k0)+2,out); // key with quotes
            fputc(':', out);
            fwrite(buf+v0,1,v1-v0,out);
        } else if (key_eq(buf,k0,k1,"users")) {
            if (wrote++) fputc(',', out);
            fputs("\"users\":", out);
            fwrite(buf+v0,1,v1-v0,out);
        } else if (key_eq(buf,k0,k1,"pages")) {
            if (wrote++) fputc(',', out);
            fputs("\"pages\":[", out);
            // iterate pages
            size_t pi=v0; pi=skip_ws(buf,pi,v1); if (pi<v1 && buf[pi]=='[') pi++;
            int first_page=1;
            while (1) {
                pi=skip_ws(buf,pi,v1); if (pi>=v1 || buf[pi]==']') break; size_t po=pi; size_t pend=skip_object(buf, po, v1); pi=pend;
                // parse page fields
                size_t pt_a=0,pt_b=0,pid_a=0,pid_b=0,pcr_a=0,pcr_b=0,pup_a=0,pup_b=0,pv_a=0,pv_b=0,pc_a=0,pc_b=0,pm_a=0,pm_b=0,lk_a=0,lk_b=0;
                size_t pj=po+1;
                while (pj<pend) {
                    pj=skip_ws(buf,pj,pend); if (pj>=pend || buf[pj]=='}') break; if (buf[pj] != '"') { pj++; continue; }
                    size_t pk0=pj+1; size_t pk1=skip_string(buf,pj,pend)-1; pj=pk1+1; pj=skip_ws(buf,pj,pend); if (pj<pend && buf[pj]==':') pj++; pj=skip_ws(buf,pj,pend);
                    size_t pv0=pj; size_t pv1=skip_value(buf,pj,pend);
                    if (key_eq(buf,pk0,pk1,"title")) { pt_a=pv0; pt_b=pv1; }
                    else if (key_eq(buf,pk0,pk1,"id")) { pid_a=pv0; pid_b=pv1; }
                    else if (key_eq(buf,pk0,pk1,"created")) { pcr_a=pv0; pcr_b=pv1; }
                    else if (key_eq(buf,pk0,pk1,"updated")) { pup_a=pv0; pup_b=pv1; }
                    else if (key_eq(buf,pk0,pk1,"views")) { pv_a=pv0; pv_b=pv1; }
                    else if (key_eq(buf,pk0,pk1,"content")) { pc_a=pv0; pc_b=pv1; }
                    else if (key_eq(buf,pk0,pk1,"lineMeta")) { pm_a=pv0; pm_b=pv1; }
                    else if (key_eq(buf,pk0,pk1,"linksLc")) { lk_a=pv0; lk_b=pv1; }
                    pj=pv1; pj=skip_ws(buf,pj,pend); if (pj<pend && buf[pj]==',') pj++;
                }

                if (!first_page) fputc(',', out); first_page=0;
                fputc('{', out);
                // Title
                fputs("\"title\":", out); if (pt_a) fwrite(buf+pt_a,1,pt_b-pt_a,out); else fputs("\"\"", out);
                // Created/Updated/ID/Views
                if (pcr_a) { fputs(",\"created\":", out); fwrite(buf+pcr_a,1,pcr_b-pcr_a,out); }
                if (pup_a) { fputs(",\"updated\":", out); fwrite(buf+pup_a,1,pup_b-pup_a,out); }
                if (pid_a) { fputs(",\"id\":", out); fwrite(buf+pid_a,1,pid_b-pid_a,out); }
                if (pv_a) { fputs(",\"views\":", out); fwrite(buf+pv_a,1,pv_b-pv_a,out); }

                // lines reconstructed from title + content (+ optional lineMeta if present)
                fputs(",\"lines\":[", out);
                char *body = (pc_a && buf[pc_a]=='"') ? json_unescape_string(buf, pc_a, pc_b) : strdup("");
                size_t mi = pm_a; // points at '['
                if (mi && buf[mi]=='[') mi++;
                int first_line_written = 0;
                // 0) Title line (only if body doesn't already start with it)
                int add_title_line = 0;
                if (pt_a && buf[pt_a]=='"') {
                    char *title_text = json_unescape_string(buf, pt_a, pt_b);
                    size_t tlen = strlen(title_text);
                    if (!(strncmp(body, title_text, tlen)==0 && (body[tlen]=='\n' || body[tlen]=='\0'))) {
                        add_title_line = 1;
                    }
                    free(title_text);
                }
                if (add_title_line) {
                    fputs("{\"text\":", out);
                    fwrite(buf+pt_a,1,pt_b-pt_a,out);
                    // created
                    fputs(",\"created\":", out);
                    if (pcr_a) fwrite(buf+pcr_a,1,pcr_b-pcr_a,out); else fputs("0", out);
                    // updated: created if present, else updated
                    fputs(",\"updated\":", out);
                    if (pcr_a) fwrite(buf+pcr_a,1,pcr_b-pcr_a,out);
                    else if (pup_a) fwrite(buf+pup_a,1,pup_b-pup_a,out);
                    else fputs("0", out);
                    // userId: default from users if available
                    fputs(",\"userId\":", out);
                    if (default_user_id) { json_escape_and_print(out, default_user_id); }
                    else { fputs("null", out); }
                    fputc('}', out);
                    first_line_written = 1;
                }
                // iterate lines by splitting body on \n and consuming one meta object per line
                char *line = body;
                for (char *p = body; ; ++p) {
                    char c = *p;
                    if (c != '\n' && c != '\0') continue;
                    // read meta object for this line
                    size_t mm = skip_ws(buf, mi, pm_b);
                    if (mm<pm_b && buf[mm]==',') { mm++; mm=skip_ws(buf,mm,pm_b); }
                    size_t mobj_beg = mm;
                    size_t mobj_end = (mm<pm_b && buf[mm]=='{') ? skip_object(buf, mm, pm_b) : mm;
                    size_t mcr0=0,mcr1=0,mup0=0,mup1=0,muid0=0,muid1=0;
                    size_t mk = mobj_beg+1;
                    while (mk<mobj_end) {
                        mk=skip_ws(buf,mk,mobj_end); if (mk>=mobj_end || buf[mk]=='}') break; if (buf[mk] != '"') { mk++; continue; }
                        size_t mkey0=mk+1; size_t mkey1=skip_string(buf,mk,mobj_end)-1; mk=mkey1+1; mk=skip_ws(buf,mk,mobj_end); if (mk<mobj_end && buf[mk]==':') mk++; mk=skip_ws(buf,mk,mobj_end);
                        size_t mv0=mk; size_t mv1=skip_value(buf,mk,mobj_end);
                        if (key_eq(buf,mkey0,mkey1,"created")) { mcr0=mv0; mcr1=mv1; }
                        else if (key_eq(buf,mkey0,mkey1,"updated")) { mup0=mv0; mup1=mv1; }
                        else if (key_eq(buf,mkey0,mkey1,"userId")) { muid0=mv0; muid1=mv1; }
                        mk=mv1; mk=skip_ws(buf,mk,mobj_end); if (mk<mobj_end && buf[mk]==',') mk++;
                    }
                    mi = mobj_end;
                    // print this line
                    if (first_line_written) fputc(',', out); first_line_written = 1;
                    char save = *p; *p = '\0';
                    fputs("{\"text\":", out); json_escape_and_print(out, line);
                    *p = save;
                    fputs(",\"created\":", out); if (mcr0) fwrite(buf+mcr0,1,mcr1-mcr0,out); else fputs("0", out);
                    fputs(",\"updated\":", out); if (mup0) fwrite(buf+mup0,1,mup1-mup0,out); else fputs("0", out);
                    fputs(",\"userId\":", out);
                    if (muid0) fwrite(buf+muid0,1,muid1-muid0,out);
                    else if (default_user_id) json_escape_and_print(out, default_user_id);
                    else fputs("null", out);
                    fputc('}', out);
                    if (c=='\0') break;
                    line = p+1;
                }
                free(body);
                fputc(']', out);
                // linksLc passthrough if present
                if (lk_a) { fputs(",\"linksLc\":", out); fwrite(buf+lk_a,1,lk_b-lk_a,out); }
                fputc('}', out);
                // next page
                pi=skip_ws(buf,pi,v1); if (pi<v1 && buf[pi]==',') { pi++; continue; }
            }
            fputc(']', out);
        }
        j=v1; j=skip_ws(buf,j,root_end); if (j<root_end && buf[j]==',') j++;
    }
    fputc('}', out); fputc('\n', out);
    if (out!=stdout) fclose(out);
    if (default_user_id) free(default_user_id);
    free(buf);
    return 0;
}
