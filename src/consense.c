#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

int compress_backup(const char *input_path, const char *output_path);

static int is_unreserved(unsigned char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) return 1;
    if (c == '-' || c == '_' || c == '.' || c == '~') return 1;
    return 0;
}

static char hex_upper(unsigned int v) {
    return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

static char *url_encode(const char *s) {
    size_t len = strlen(s);
    size_t out_cap = len * 3 + 1;
    char *out = (char *)malloc(out_cap);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (is_unreserved(c)) {
            out[j++] = (char)c;
        } else {
            out[j++] = '%';
            out[j++] = hex_upper((c >> 4) & 0xF);
            out[j++] = hex_upper(c & 0xF);
        }
    }
    out[j] = '\0';
    return out;
}

static char *unescape_backslashes(const char *s) {
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\\' && i + 1 < len) {
            unsigned char n = (unsigned char)s[i + 1];
            switch (n) {
                case 'n': out[j++] = '\n'; i++; break;
                case 'r': out[j++] = '\r'; i++; break;
                case 't': out[j++] = '\t'; i++; break;
                case '\\': out[j++] = '\\'; i++; break;
                case '"': out[j++] = '"'; i++; break;
                case '\'': out[j++] = '\''; i++; break;
                case 'b': out[j++] = '\b'; i++; break;
                case 'f': out[j++] = '\f'; i++; break;
                case '0': out[j++] = '\0'; i++; break;
                default:
                    // Unknown escape, keep as-is
                    out[j++] = (char)c;
                    break;
            }
        } else {
            out[j++] = (char)c;
        }
    }
    out[j] = '\0';
    return out;
}

static char *config_path(void) {
    const char *home = getenv("HOME");
    if (!home) return NULL;
    size_t path_len = strlen(home) + strlen("/.consense_project") + 1;
    char *path = (char *)malloc(path_len);
    if (!path) return NULL;
    snprintf(path, path_len, "%s/.consense_project", home);
    return path;
}

static char *read_project_from_file(void) {
    char *path = config_path();
    if (!path) return NULL;

    FILE *f = fopen(path, "r");
    free(path);
    if (!f) return NULL;

    char buf[256];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return NULL;
    }
    fclose(f);

    size_t n = strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ' || buf[n - 1] == '\t')) {
        buf[--n] = '\0';
    }
    size_t start = 0;
    while (buf[start] == ' ' || buf[start] == '\t') start++;
    if (buf[start] == '\0') return NULL;
    char *res = strdup(buf + start);
    return res;
}

static int write_project_to_file(const char *project) {
    if (!project || !*project) return -1;
    char *path = config_path();
    if (!path) return -1;
    FILE *f = fopen(path, "w");
    free(path);
    if (!f) return -1;
    int rc = 0;
    if (fprintf(f, "%s\n", project) < 0) rc = -1;
    if (fclose(f) != 0) rc = -1;
    return rc;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage:\n"
                "  %s compress <input.json> [output.json]\n"
                "  %s decompress <input.json> [output.json]\n"
                "  %s set-project <project>\n"
                "  %s <page-name> [content]\n",
                argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "compress") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s compress <input.json> [output.json]\n", argv[0]);
            return 1;
        }
        const char *in = argv[2];
        const char *out = (argc >= 4) ? argv[3] : NULL;
        return compress_backup(in, out);
    }

    if (strcmp(argv[1], "decompress") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s decompress <input.json> [output.json]\n", argv[0]);
            return 1;
        }
        extern int decompress_backup(const char *input_path, const char *output_path);
        const char *in = argv[2];
        const char *out = (argc >= 4) ? argv[3] : NULL;
        return decompress_backup(in, out);
    }

    if (strcmp(argv[1], "set-project") == 0) {
        if (argc < 3) {
            fprintf(stderr, "set-project requires a <project> name.\n");
            return 1;
        }
        const char *project = argv[2];
        if (strchr(project, ' ')) {
            fprintf(stderr, "Warning: project contains spaces; using verbatim.\n");
        }
        if (write_project_to_file(project) != 0) {
            fprintf(stderr, "Failed to save project.\n");
            return 1;
        }
        printf("Project set to '%s'.\n", project);
        return 0;
    }

    char *project = read_project_from_file();
    if (!project || !*project) {
        fprintf(stderr, "Project not set. Run: %s set-project <project>\n", argv[0]);
        free(project);
        return 1;
    }

    // Determine if content is provided as the last argument
    int has_content = (argc >= 3);
    size_t page_args_end = argc - (has_content ? 1 : 0);

    size_t total_len = 0;
    for (int i = 1; i < (int)page_args_end; i++) total_len += strlen(argv[i]) + (i + 1 < (int)page_args_end ? 1 : 0);
    char *page = (char *)malloc(total_len + 1);
    if (!page) {
        fprintf(stderr, "Allocation failed.\n");
        free(project);
        return 1;
    }
    page[0] = '\0';
    for (int i = 1; i < (int)page_args_end; i++) {
        strcat(page, argv[i]);
        if (i + 1 < (int)page_args_end) strcat(page, " ");
    }

    char *encoded = url_encode(page);
    if (!encoded) {
        fprintf(stderr, "Encoding failed.\n");
        free(project);
        free(page);
        return 1;
    }

    const char *base = "https://scrapbox.io";
    char *encoded_body = NULL;
    if (has_content) {
        char *unesc = unescape_backslashes(argv[argc - 1]);
        if (!unesc) {
            fprintf(stderr, "Allocation failed.\n");
            free(project);
            free(page);
            free(encoded);
            return 1;
        }
        encoded_body = url_encode(unesc);
        free(unesc);
        if (!encoded_body) {
            fprintf(stderr, "Encoding failed.\n");
            free(project);
            free(page);
            free(encoded);
            return 1;
        }
    }

    size_t url_len = strlen(base) + 1 + strlen(project) + 1 + strlen(encoded) + (has_content ? (6 + 1 + strlen(encoded_body)) : 0) + 1;
    char *url = (char *)malloc(url_len);
    if (!url) {
        fprintf(stderr, "Allocation failed.\n");
        free(project);
        free(page);
        free(encoded);
        if (encoded_body) free(encoded_body);
        return 1;
    }
    if (has_content) {
        snprintf(url, url_len, "%s/%s/%s?body=%s", base, project, encoded, encoded_body);
    } else {
        snprintf(url, url_len, "%s/%s/%s", base, project, encoded);
    }

    int rc = 0;
#if defined(__APPLE__)
    if (execlp("open", "open", url, (char *)NULL) == -1) rc = 1;
#elif defined(__linux__)
    if (execlp("xdg-open", "xdg-open", url, (char *)NULL) == -1) rc = 1;
#else
    rc = 1;
#endif

    if (rc != 0) {
        fprintf(stderr, "Could not open browser. URL: %s\n", url);
    }

    free(project);
    free(page);
    free(encoded);
    if (encoded_body) free(encoded_body);
    free(url);
    return rc;
}
