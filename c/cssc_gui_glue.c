/*
 * cssc_gui_glue.c -- glue for linking the gui/game/extras host runtime into the
 * transembly host DLL (cssc_rt_host.c). Those three files were written against
 * the native compiler's LLVM-emitted runtime, so they reference a few symbols
 * the transembly host doesn't provide: the seeded-RNG state global and the
 * typed int->int map used by cssc_video's resolution getter. This file supplies
 * real implementations of exactly those, so no symbol is left as a stub.
 *
 * Compiled ONLY into the gui-enabled DLL (build_rt.py --gui), alongside
 * cssc_host_gui.c + cssc_host_game.c + cssc_host_extras.c.
 */
#include <stdint.h>
#include <stdlib.h>

/* Seeded-RNG state global. cssc_host_game.c (cssc_video_seed / terrain gen)
 * writes and reads it; the native build emits it as an LLVM global. Same seed
 * default as cssc_softfloat.c so behaviour matches the compiled path. */
uint64_t cssc_rng_state = 0x9E3779B97F4A7C15ULL;

/* Typed int->int map (keys and values are raw i64). cssc_host_game.c builds a
 * "0 -> width, 1 -> height" pair for cssc_video's resolution getter. A real
 * linear (keys,vals) map -- distinct from cssc_rt_host.c's string-keyed map. */
typedef struct { int64_t len, cap; int64_t *keys, *vals; } cssc_map_ii;

void *cssc_map_ii_new(int64_t cap_hint) {
    int64_t cap = cap_hint < 4 ? 4 : cap_hint;
    cssc_map_ii *m = (cssc_map_ii *)malloc(sizeof(cssc_map_ii));
    if (!m) return 0;
    m->len = 0;
    m->cap = cap;
    m->keys = (int64_t *)malloc((size_t)cap * sizeof(int64_t));
    m->vals = (int64_t *)malloc((size_t)cap * sizeof(int64_t));
    return m;
}

void cssc_map_ii_set(void *p, int64_t key, int64_t val) {
    cssc_map_ii *m = (cssc_map_ii *)p;
    if (!m) return;
    for (int64_t i = 0; i < m->len; i++) {
        if (m->keys[i] == key) { m->vals[i] = val; return; }
    }
    if (m->len >= m->cap) {
        int64_t nc = m->cap * 2;
        int64_t *nk = (int64_t *)realloc(m->keys, (size_t)nc * sizeof(int64_t));
        int64_t *nv = (int64_t *)realloc(m->vals, (size_t)nc * sizeof(int64_t));
        if (!nk || !nv) return;
        m->keys = nk;
        m->vals = nv;
        m->cap = nc;
    }
    m->keys[m->len] = key;
    m->vals[m->len] = val;
    m->len++;
}

/* ---- SysV<->MS ABI bridge + framebuffer checksum -------------------------
 * The transembly backend emits SysV calls, and cssc_rt_host.c's rtsym funcs are
 * __attribute__((sysv_abi)). But cssc_host_gui/game/extras.c are compiled with
 * mingw's DEFAULT (MS x64) ABI. So each gui rtsym is reached through a SysV
 * wrapper here that forwards to the MS-ABI gui function -- the compiler emits
 * the ABI bridge (same technique as the freestanding float formatter). */
#define SYSV __attribute__((sysv_abi))

extern void *cssc_gui_screen_new(int64_t w, int64_t h, int64_t fps);
extern void  cssc_gui_screen_clear(void *s, int64_t argb);
extern void  cssc_gui_screen_present(void *s);
extern int64_t cssc_gui_screen_isopen(void *s);
extern void  cssc_gui_screen_close(void *s);
extern int64_t cssc_gui_screen_width(void *s);
extern int64_t cssc_gui_screen_height(void *s);
extern void  cssc_gui_screen_fillrect(void *s, int64_t x, int64_t y,
                                      int64_t w, int64_t h, int64_t argb);
extern void  cssc_gui_screen_drawrect(void *s, int64_t x, int64_t y,
                                      int64_t w, int64_t h, int64_t argb);
extern void  cssc_gui_screen_drawtext(void *s, int64_t x, int64_t y,
                                      const char *text, int64_t argb, int64_t scale);
extern uint32_t *cssc_video_backbuf(void *v);

SYSV void   *cssc_rt_gui_screen_new(int64_t w, int64_t h, int64_t fps) { return cssc_gui_screen_new(w, h, fps); }
SYSV void    cssc_rt_gui_screen_clear(void *s, int64_t argb)           { cssc_gui_screen_clear(s, argb); }
SYSV void    cssc_rt_gui_screen_present(void *s)                       { cssc_gui_screen_present(s); }
SYSV int64_t cssc_rt_gui_screen_isopen(void *s)                        { return cssc_gui_screen_isopen(s); }
SYSV void    cssc_rt_gui_screen_close(void *s)                         { cssc_gui_screen_close(s); }
SYSV int64_t cssc_rt_gui_screen_width(void *s)                         { return cssc_gui_screen_width(s); }
SYSV int64_t cssc_rt_gui_screen_height(void *s)                        { return cssc_gui_screen_height(s); }
SYSV void    cssc_rt_gui_screen_fillrect(void *s, int64_t x, int64_t y, int64_t w, int64_t h, int64_t argb) { cssc_gui_screen_fillrect(s, x, y, w, h, argb); }
SYSV void    cssc_rt_gui_screen_drawrect(void *s, int64_t x, int64_t y, int64_t w, int64_t h, int64_t argb) { cssc_gui_screen_drawrect(s, x, y, w, h, argb); }
/* text arg arrives as a transembly cssc_str* {u32 refcount; u32 size; char data[]}
 * -- the raw bytes start at offset 8. Extract + forward to the MS-ABI drawtext. */
SYSV void    cssc_rt_gui_screen_drawtext(void *s, int64_t x, int64_t y, void *str, int64_t argb, int64_t scale) {
    const char *text = str ? (const char *)((char *)str + 8) : "";
    cssc_gui_screen_drawtext(s, x, y, text, argb, scale);
}

/* Deterministic FNV-1a checksum of the screen's backbuffer (w*h native-format
 * DWORDs). The interpreter hashes its own framebuffer identically, so a gui
 * program can print `screen->fbChecksum()` and be stdout-parity-gated even
 * though it renders pixels, not text. The screen struct starts with {vid, w, h}
 * (cssc_host_gui.c: cssc_gui_screen), so read them by offset. */
SYSV int64_t cssc_rt_gui_screen_fbchecksum(void *s) {
    if (!s) return 0;
    void *vid    = *(void **)s;
    int64_t w    = *(int64_t *)((char *)s + 8);
    int64_t h    = *(int64_t *)((char *)s + 16);
    uint32_t *fb = cssc_video_backbuf(vid);
    if (!fb || w <= 0 || h <= 0) return 0;
    uint64_t hsh = 1469598103934665603ULL;         /* FNV-1a 64 offset basis */
    int64_t n = w * h;
    for (int64_t i = 0; i < n; i++) {
        uint32_t px = fb[i];
        hsh = (hsh ^ (uint64_t)(px & 0xFFu))        * 1099511628211ULL;
        hsh = (hsh ^ (uint64_t)((px >> 8) & 0xFFu)) * 1099511628211ULL;
        hsh = (hsh ^ (uint64_t)((px >> 16) & 0xFFu))* 1099511628211ULL;
        hsh = (hsh ^ (uint64_t)((px >> 24) & 0xFFu))* 1099511628211ULL;
    }
    return (int64_t)(hsh & 0x7FFFFFFFFFFFFFFFULL);
}

/* ==== generated Font/Text/Button/Toolbar wrappers (guigen.py) ==== */
/* GUIGEN-EXTERNS-BEGIN (generated by harness/gui_gen.py -- do not hand-edit) */
extern void *cssc_gui_font_new(int64_t);
extern void cssc_gui_font_setscale(void*, int64_t);
extern int64_t cssc_gui_font_scale(void*);
extern void cssc_gui_font_setcolor(void*, int64_t);
extern int64_t cssc_gui_font_color(void*);
extern int64_t cssc_gui_font_measure(void*, const char*);
extern int64_t cssc_gui_font_height(void*);
extern void *cssc_gui_text_new(void*);
extern void cssc_gui_text_settext(void*, const char*);
extern void* cssc_gui_text_text(void*);
extern void cssc_gui_text_setpos(void*, int64_t, int64_t);
extern int64_t cssc_gui_text_x(void*);
extern int64_t cssc_gui_text_y(void*);
extern void cssc_gui_text_setcolor(void*, int64_t);
extern void cssc_gui_text_setscale(void*, int64_t);
extern void cssc_gui_text_setfont(void*, void*);
extern int64_t cssc_gui_text_measure(void*);
extern void cssc_gui_text_draw(void*);
extern void cssc_gui_text_hide(void*);
extern void cssc_gui_text_show(void*);
extern void *cssc_gui_button_new(void*);
extern void cssc_gui_button_setlabel(void*, const char*);
extern void* cssc_gui_button_label(void*);
extern void cssc_gui_button_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern void cssc_gui_button_position(void*, int64_t, int64_t);
extern void cssc_gui_button_size(void*, int64_t, int64_t);
extern void cssc_gui_button_setcolor(void*, int64_t);
extern void cssc_gui_button_sethovercolor(void*, int64_t);
extern void cssc_gui_button_settextcolor(void*, int64_t);
extern void cssc_gui_button_onclick(void*, int64_t);
extern int64_t cssc_gui_button_update(void*, void*);
extern int64_t cssc_gui_button_ishovered(void*);
extern int64_t cssc_gui_button_ispressed(void*);
extern void cssc_gui_button_draw(void*);
extern void cssc_gui_button_hide(void*);
extern void cssc_gui_button_show(void*);
extern void *cssc_gui_toolbar_new(void*);
extern void cssc_gui_toolbar_add(void*, void*);
extern void cssc_gui_toolbar_setpos(void*, int64_t, int64_t);
extern void cssc_gui_toolbar_setsize(void*, int64_t, int64_t);
extern void cssc_gui_toolbar_setorientation(void*, int64_t);
extern void cssc_gui_toolbar_setspacing(void*, int64_t);
extern void cssc_gui_toolbar_setrighttext(void*, const char*);
extern void cssc_gui_toolbar_setrightcolor(void*, int64_t);
extern int64_t cssc_gui_toolbar_update(void*, void*);
extern void cssc_gui_toolbar_draw(void*);
extern void *cssc_gui_textbox_new(void*);
extern void cssc_gui_textbox_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern void cssc_gui_textbox_settext(void*, const char*);
extern void* cssc_gui_textbox_text(void*);
extern int64_t cssc_gui_textbox_length(void*);
extern void cssc_gui_textbox_setfocus(void*, int64_t);
extern int64_t cssc_gui_textbox_focused(void*);
extern void cssc_gui_textbox_setcolor(void*, int64_t, int64_t);
extern void cssc_gui_textbox_setscale(void*, int64_t);
extern int64_t cssc_gui_textbox_update(void*, void*);
extern void cssc_gui_textbox_draw(void*);
extern void *cssc_gui_editor_new(void*);
extern void cssc_gui_editor_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern void cssc_gui_editor_settext(void*, const char*);
extern void* cssc_gui_editor_text(void*);
extern int64_t cssc_gui_editor_linecount(void*);
extern int64_t cssc_gui_editor_cursorline(void*);
extern int64_t cssc_gui_editor_cursorcol(void*);
extern int64_t cssc_gui_editor_revision(void*);
extern int64_t cssc_gui_editor_caretpixelx(void*);
extern int64_t cssc_gui_editor_caretpixely(void*);
extern int64_t cssc_gui_editor_lineheight(void*);
extern int64_t cssc_gui_editor_completereq(void*);
extern int64_t cssc_gui_editor_completeactive(void*);
extern void cssc_gui_editor_completecancel(void*);
extern void* cssc_gui_editor_completionquery(void*);
extern void cssc_gui_editor_setcompletions(void*, const char*);
extern int64_t cssc_gui_editor_hoverreq(void*);
extern int64_t cssc_gui_editor_hoveractive(void*);
extern void cssc_gui_editor_hovercancel(void*);
extern void* cssc_gui_editor_hoverquery(void*);
extern void cssc_gui_editor_sethover(void*, const char*);
extern void cssc_gui_editor_setdiagnostics(void*, const char*);
extern void cssc_gui_editor_setstickydiag(void*, const char*);
extern void cssc_gui_editor_clearstickydiag(void*);
extern void cssc_gui_editor_setcleanmarks(void*, const char*);
extern void cssc_gui_editor_clearcleanmarks(void*);
extern void cssc_gui_editor_setipline(void*, int64_t);
extern void cssc_gui_editor_revealip(void*);
extern int64_t cssc_gui_editor_stickycount(void*);
extern int64_t cssc_gui_editor_sigreq(void*);
extern int64_t cssc_gui_editor_sigactive(void*);
extern void cssc_gui_editor_sigcancel(void*);
extern void* cssc_gui_editor_sigquery(void*);
extern void cssc_gui_editor_setsignature(void*, const char*);
extern void cssc_gui_editor_gotoline(void*, int64_t);
extern void cssc_gui_editor_setcursor(void*, int64_t, int64_t);
extern void cssc_gui_editor_insert(void*, const char*);
extern int64_t cssc_gui_editor_hasselection(void*);
extern void* cssc_gui_editor_selectedtext(void*);
extern int64_t cssc_gui_editor_doubleclicked(void*);
extern int64_t cssc_gui_editor_rightclicked(void*);
extern int64_t cssc_gui_editor_clicked(void*);
extern int64_t cssc_gui_editor_scandecl(void*, const char*);
extern void* cssc_gui_editor_decltype(void*);
extern void* cssc_gui_editor_declbase(void*);
extern int64_t cssc_gui_editor_declbits(void*);
extern int64_t cssc_gui_editor_declisauto(void*);
extern int64_t cssc_gui_editor_valuebits(void*, const char*, const char*);
extern int64_t cssc_gui_editor_saverequested(void*);
extern void cssc_gui_editor_undo(void*);
extern void cssc_gui_editor_redo(void*);
extern void cssc_gui_editor_selectall(void*);
extern int64_t cssc_gui_editor_search(void*, const char*);
extern int64_t cssc_gui_editor_markall(void*, const char*);
extern void cssc_gui_editor_clearsearch(void*);
extern int64_t cssc_gui_editor_searchcount(void*);
extern int64_t cssc_gui_editor_replaceall(void*, const char*, const char*);
extern void cssc_gui_editor_setfocus(void*, int64_t);
extern void cssc_gui_editor_setscale(void*, int64_t);
extern void cssc_gui_editor_setcolor(void*, int64_t, int64_t);
extern void cssc_gui_editor_setgutter(void*, int64_t);
extern void cssc_gui_editor_setvisible(void*, int64_t);
extern void cssc_gui_editor_setlanguage(void*, int64_t);
extern void cssc_gui_editor_setlangforpath(void*, const char*);
extern void cssc_gui_editor_setownership(void*, const char*);
extern int64_t cssc_gui_editor_update(void*, void*);
extern void cssc_gui_editor_draw(void*);
extern void *cssc_gui_list_new(void*);
extern void cssc_gui_list_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern void cssc_gui_list_add(void*, const char*);
extern void cssc_gui_list_addat(void*, const char*, int64_t);
extern void cssc_gui_list_clear(void*);
extern int64_t cssc_gui_list_count(void*);
extern int64_t cssc_gui_list_selected(void*);
extern int64_t cssc_gui_list_rightclicked(void*);
extern void* cssc_gui_list_selectedtext(void*);
extern void cssc_gui_list_setselected(void*, int64_t);
extern void cssc_gui_list_setfocus(void*, int64_t);
extern void cssc_gui_list_setscale(void*, int64_t);
extern void cssc_gui_list_setcolor(void*, int64_t, int64_t);
extern int64_t cssc_gui_list_update(void*, void*);
extern void cssc_gui_list_draw(void*);
extern void *cssc_gui_tree_new(void*);
extern void cssc_gui_tree_setroot(void*, const char*);
extern void cssc_gui_tree_seticondir(void*, const char*);
extern void cssc_gui_tree_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern int64_t cssc_gui_tree_width(void*);
extern void cssc_gui_tree_refresh(void*);
extern int64_t cssc_gui_tree_count(void*);
extern int64_t cssc_gui_tree_selected(void*);
extern void* cssc_gui_tree_selectedpath(void*);
extern void* cssc_gui_tree_selectedname(void*);
extern int64_t cssc_gui_tree_selectedisdir(void*);
extern int64_t cssc_gui_tree_rightclicked(void*);
extern int64_t cssc_gui_tree_dropready(void*);
extern void* cssc_gui_tree_dropsrc(void*);
extern void* cssc_gui_tree_dropdst(void*);
extern void cssc_gui_tree_setfocus(void*, int64_t);
extern void cssc_gui_tree_setscale(void*, int64_t);
extern void cssc_gui_tree_setcolor(void*, int64_t, int64_t);
extern void cssc_gui_tree_setvisible(void*, int64_t);
extern int64_t cssc_gui_tree_update(void*, void*);
extern void cssc_gui_tree_draw(void*);
extern void *cssc_gui_terminal_new(void*);
extern void cssc_gui_terminal_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern void cssc_gui_terminal_setcwd(void*, const char*);
extern void cssc_gui_terminal_setfocus(void*, int64_t);
extern void cssc_gui_terminal_setscale(void*, int64_t);
extern void cssc_gui_terminal_setcolor(void*, int64_t, int64_t);
extern int64_t cssc_gui_terminal_isrunning(void*);
extern void cssc_gui_terminal_run(void*, const char*);
extern void cssc_gui_terminal_write(void*, const char*);
extern void cssc_gui_terminal_writeansi(void*, const char*);
extern void cssc_gui_terminal_clear(void*);
extern int64_t cssc_gui_terminal_ipline(void*);
extern void* cssc_gui_terminal_ipfile(void*);
extern void cssc_gui_terminal_setinputlock(void*, int64_t);
extern void cssc_gui_terminal_stop(void*);
extern void cssc_gui_terminal_setvisible(void*, int64_t);
extern int64_t cssc_gui_terminal_update(void*, void*);
extern void cssc_gui_terminal_draw(void*);
extern void *cssc_gui_menu_new(void*);
extern void cssc_gui_menu_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern int64_t cssc_gui_menu_addmenu(void*, const char*);
extern void cssc_gui_menu_additem(void*, int64_t, const char*, int64_t);
extern void cssc_gui_menu_setscale(void*, int64_t);
extern void cssc_gui_menu_setcolor(void*, int64_t, int64_t);
extern void cssc_gui_menu_setrighttext(void*, const char*);
extern void cssc_gui_menu_setrightcolor(void*, int64_t);
extern int64_t cssc_gui_menu_isopen(void*);
extern int64_t cssc_gui_menu_action(void*);
extern int64_t cssc_gui_menu_update(void*, void*);
extern void cssc_gui_menu_draw(void*);
extern void *cssc_gui_prompt_new(void*);
extern void cssc_gui_prompt_open(void*, const char*);
extern int64_t cssc_gui_prompt_isopen(void*);
extern int64_t cssc_gui_prompt_result(void*);
extern void* cssc_gui_prompt_text(void*);
extern void cssc_gui_prompt_setscale(void*, int64_t);
extern void cssc_gui_prompt_setcolor(void*, int64_t, int64_t);
extern int64_t cssc_gui_prompt_update(void*, void*);
extern void cssc_gui_prompt_draw(void*);
extern void *cssc_gui_tabs_new(void*);
extern void cssc_gui_tabs_setrect(void*, int64_t, int64_t, int64_t, int64_t);
extern void cssc_gui_tabs_add(void*, const char*);
extern int64_t cssc_gui_tabs_indexof(void*, const char*);
extern int64_t cssc_gui_tabs_count(void*);
extern int64_t cssc_gui_tabs_active(void*);
extern void* cssc_gui_tabs_activepath(void*);
extern void* cssc_gui_tabs_pathof(void*, int64_t);
extern void cssc_gui_tabs_setactive(void*, int64_t);
extern void cssc_gui_tabs_remove(void*, int64_t);
extern int64_t cssc_gui_tabs_clicked(void*);
extern int64_t cssc_gui_tabs_closerequested(void*);
extern void cssc_gui_tabs_setscale(void*, int64_t);
extern void cssc_gui_tabs_setcolor(void*, int64_t, int64_t);
extern int64_t cssc_gui_tabs_update(void*, void*);
extern void cssc_gui_tabs_draw(void*);
extern void *cssc_gui_browser_new(void*);
extern void cssc_gui_browser_open(void*, const char*, int64_t);
extern int64_t cssc_gui_browser_isopen(void*);
extern int64_t cssc_gui_browser_result(void*);
extern void* cssc_gui_browser_chosen(void*);
extern void cssc_gui_browser_setscale(void*, int64_t);
extern void cssc_gui_browser_setcolor(void*, int64_t, int64_t);
extern void cssc_gui_browser_seticondir(void*, const char*);
extern int64_t cssc_gui_browser_update(void*, void*);
extern void cssc_gui_browser_draw(void*);
extern void *cssc_gui_debugger_new(void*);
extern void cssc_gui_debugger_start(void*, const char*, const char*);
extern int64_t cssc_gui_debugger_update(void*);
extern int64_t cssc_gui_debugger_active(void*);
extern int64_t cssc_gui_debugger_focused(void*);
extern void cssc_gui_debugger_click(void*, int64_t, int64_t, int64_t);
extern void cssc_gui_debugger_draw(void*);
extern int64_t cssc_gui_debugger_ipline(void*);
extern int64_t cssc_gui_debugger_ipfollow(void*);
extern void* cssc_gui_debugger_ipfile(void*);
extern void* cssc_gui_debugger_takeoutput(void*);
extern int64_t cssc_gui_debugger_held(void*);
extern void cssc_gui_debugger_clearheld(void*);
extern void cssc_gui_debugger_close(void*);
extern void cssc_gui_debugger_key(void*, int64_t);
extern void cssc_gui_debugger_char(void*, int64_t);
/* GUIGEN-EXTERNS-END */

/* GUIGEN-WRAPPERS-BEGIN (generated by harness/gui_gen.py -- do not hand-edit) */
SYSV void *cssc_rt_cssc_gui_font_new(int64_t a0) { return cssc_gui_font_new(a0); }
SYSV void cssc_rt_cssc_gui_font_setscale(void *s, int64_t a0) { cssc_gui_font_setscale(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_font_scale(void *s) { return cssc_gui_font_scale(s); }
SYSV void cssc_rt_cssc_gui_font_setcolor(void *s, int64_t a0) { cssc_gui_font_setcolor(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_font_color(void *s) { return cssc_gui_font_color(s); }
SYSV int64_t cssc_rt_cssc_gui_font_measure(void *s, void* a0) { return cssc_gui_font_measure(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_font_height(void *s) { return cssc_gui_font_height(s); }
SYSV void *cssc_rt_cssc_gui_text_new(void* a0) { return cssc_gui_text_new(a0); }
SYSV void cssc_rt_cssc_gui_text_settext(void *s, void* a0) { cssc_gui_text_settext(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void * cssc_rt_cssc_gui_text_text(void *s) { return cssc_gui_text_text(s); }
SYSV void cssc_rt_cssc_gui_text_setpos(void *s, int64_t a0, int64_t a1) { cssc_gui_text_setpos(s, a0, a1); }
SYSV int64_t cssc_rt_cssc_gui_text_x(void *s) { return cssc_gui_text_x(s); }
SYSV int64_t cssc_rt_cssc_gui_text_y(void *s) { return cssc_gui_text_y(s); }
SYSV void cssc_rt_cssc_gui_text_setcolor(void *s, int64_t a0) { cssc_gui_text_setcolor(s, a0); }
SYSV void cssc_rt_cssc_gui_text_setscale(void *s, int64_t a0) { cssc_gui_text_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_text_setfont(void *s, void* a0) { cssc_gui_text_setfont(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_text_measure(void *s) { return cssc_gui_text_measure(s); }
SYSV void cssc_rt_cssc_gui_text_draw(void *s) { cssc_gui_text_draw(s); }
SYSV void cssc_rt_cssc_gui_text_hide(void *s) { cssc_gui_text_hide(s); }
SYSV void cssc_rt_cssc_gui_text_show(void *s) { cssc_gui_text_show(s); }
SYSV void *cssc_rt_cssc_gui_button_new(void* a0) { return cssc_gui_button_new(a0); }
SYSV void cssc_rt_cssc_gui_button_setlabel(void *s, void* a0) { cssc_gui_button_setlabel(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void * cssc_rt_cssc_gui_button_label(void *s) { return cssc_gui_button_label(s); }
SYSV void cssc_rt_cssc_gui_button_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_button_setrect(s, a0, a1, a2, a3); }
SYSV void cssc_rt_cssc_gui_button_position(void *s, int64_t a0, int64_t a1) { cssc_gui_button_position(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_button_size(void *s, int64_t a0, int64_t a1) { cssc_gui_button_size(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_button_setcolor(void *s, int64_t a0) { cssc_gui_button_setcolor(s, a0); }
SYSV void cssc_rt_cssc_gui_button_sethovercolor(void *s, int64_t a0) { cssc_gui_button_sethovercolor(s, a0); }
SYSV void cssc_rt_cssc_gui_button_settextcolor(void *s, int64_t a0) { cssc_gui_button_settextcolor(s, a0); }
SYSV void cssc_rt_cssc_gui_button_onclick(void *s, int64_t a0) { cssc_gui_button_onclick(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_button_update(void *s, void* a0) { return cssc_gui_button_update(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_button_ishovered(void *s) { return cssc_gui_button_ishovered(s); }
SYSV int64_t cssc_rt_cssc_gui_button_ispressed(void *s) { return cssc_gui_button_ispressed(s); }
SYSV void cssc_rt_cssc_gui_button_draw(void *s) { cssc_gui_button_draw(s); }
SYSV void cssc_rt_cssc_gui_button_hide(void *s) { cssc_gui_button_hide(s); }
SYSV void cssc_rt_cssc_gui_button_show(void *s) { cssc_gui_button_show(s); }
SYSV void *cssc_rt_cssc_gui_toolbar_new(void* a0) { return cssc_gui_toolbar_new(a0); }
SYSV void cssc_rt_cssc_gui_toolbar_add(void *s, void* a0) { cssc_gui_toolbar_add(s, a0); }
SYSV void cssc_rt_cssc_gui_toolbar_setpos(void *s, int64_t a0, int64_t a1) { cssc_gui_toolbar_setpos(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_toolbar_setsize(void *s, int64_t a0, int64_t a1) { cssc_gui_toolbar_setsize(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_toolbar_setorientation(void *s, int64_t a0) { cssc_gui_toolbar_setorientation(s, a0); }
SYSV void cssc_rt_cssc_gui_toolbar_setspacing(void *s, int64_t a0) { cssc_gui_toolbar_setspacing(s, a0); }
SYSV void cssc_rt_cssc_gui_toolbar_setrighttext(void *s, void* a0) { cssc_gui_toolbar_setrighttext(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_toolbar_setrightcolor(void *s, int64_t a0) { cssc_gui_toolbar_setrightcolor(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_toolbar_update(void *s, void* a0) { return cssc_gui_toolbar_update(s, a0); }
SYSV void cssc_rt_cssc_gui_toolbar_draw(void *s) { cssc_gui_toolbar_draw(s); }
SYSV void *cssc_rt_cssc_gui_textbox_new(void* a0) { return cssc_gui_textbox_new(a0); }
SYSV void cssc_rt_cssc_gui_textbox_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_textbox_setrect(s, a0, a1, a2, a3); }
SYSV void cssc_rt_cssc_gui_textbox_settext(void *s, void* a0) { cssc_gui_textbox_settext(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void * cssc_rt_cssc_gui_textbox_text(void *s) { return cssc_gui_textbox_text(s); }
SYSV int64_t cssc_rt_cssc_gui_textbox_length(void *s) { return cssc_gui_textbox_length(s); }
SYSV void cssc_rt_cssc_gui_textbox_setfocus(void *s, int64_t a0) { cssc_gui_textbox_setfocus(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_textbox_focused(void *s) { return cssc_gui_textbox_focused(s); }
SYSV void cssc_rt_cssc_gui_textbox_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_textbox_setcolor(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_textbox_setscale(void *s, int64_t a0) { cssc_gui_textbox_setscale(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_textbox_update(void *s, void* a0) { return cssc_gui_textbox_update(s, a0); }
SYSV void cssc_rt_cssc_gui_textbox_draw(void *s) { cssc_gui_textbox_draw(s); }
SYSV void *cssc_rt_cssc_gui_editor_new(void* a0) { return cssc_gui_editor_new(a0); }
SYSV void cssc_rt_cssc_gui_editor_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_editor_setrect(s, a0, a1, a2, a3); }
SYSV void cssc_rt_cssc_gui_editor_settext(void *s, void* a0) { cssc_gui_editor_settext(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void * cssc_rt_cssc_gui_editor_text(void *s) { return cssc_gui_editor_text(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_linecount(void *s) { return cssc_gui_editor_linecount(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_cursorline(void *s) { return cssc_gui_editor_cursorline(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_cursorcol(void *s) { return cssc_gui_editor_cursorcol(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_revision(void *s) { return cssc_gui_editor_revision(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_caretpixelx(void *s) { return cssc_gui_editor_caretpixelx(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_caretpixely(void *s) { return cssc_gui_editor_caretpixely(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_lineheight(void *s) { return cssc_gui_editor_lineheight(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_completereq(void *s) { return cssc_gui_editor_completereq(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_completeactive(void *s) { return cssc_gui_editor_completeactive(s); }
SYSV void cssc_rt_cssc_gui_editor_completecancel(void *s) { cssc_gui_editor_completecancel(s); }
SYSV void * cssc_rt_cssc_gui_editor_completionquery(void *s) { return cssc_gui_editor_completionquery(s); }
SYSV void cssc_rt_cssc_gui_editor_setcompletions(void *s, void* a0) { cssc_gui_editor_setcompletions(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_editor_hoverreq(void *s) { return cssc_gui_editor_hoverreq(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_hoveractive(void *s) { return cssc_gui_editor_hoveractive(s); }
SYSV void cssc_rt_cssc_gui_editor_hovercancel(void *s) { cssc_gui_editor_hovercancel(s); }
SYSV void * cssc_rt_cssc_gui_editor_hoverquery(void *s) { return cssc_gui_editor_hoverquery(s); }
SYSV void cssc_rt_cssc_gui_editor_sethover(void *s, void* a0) { cssc_gui_editor_sethover(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_setdiagnostics(void *s, void* a0) { cssc_gui_editor_setdiagnostics(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_setstickydiag(void *s, void* a0) { cssc_gui_editor_setstickydiag(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_clearstickydiag(void *s) { cssc_gui_editor_clearstickydiag(s); }
SYSV void cssc_rt_cssc_gui_editor_setcleanmarks(void *s, void* a0) { cssc_gui_editor_setcleanmarks(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_clearcleanmarks(void *s) { cssc_gui_editor_clearcleanmarks(s); }
SYSV void cssc_rt_cssc_gui_editor_setipline(void *s, int64_t a0) { cssc_gui_editor_setipline(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_revealip(void *s) { cssc_gui_editor_revealip(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_stickycount(void *s) { return cssc_gui_editor_stickycount(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_sigreq(void *s) { return cssc_gui_editor_sigreq(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_sigactive(void *s) { return cssc_gui_editor_sigactive(s); }
SYSV void cssc_rt_cssc_gui_editor_sigcancel(void *s) { cssc_gui_editor_sigcancel(s); }
SYSV void * cssc_rt_cssc_gui_editor_sigquery(void *s) { return cssc_gui_editor_sigquery(s); }
SYSV void cssc_rt_cssc_gui_editor_setsignature(void *s, void* a0) { cssc_gui_editor_setsignature(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_gotoline(void *s, int64_t a0) { cssc_gui_editor_gotoline(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_setcursor(void *s, int64_t a0, int64_t a1) { cssc_gui_editor_setcursor(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_editor_insert(void *s, void* a0) { cssc_gui_editor_insert(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_editor_hasselection(void *s) { return cssc_gui_editor_hasselection(s); }
SYSV void * cssc_rt_cssc_gui_editor_selectedtext(void *s) { return cssc_gui_editor_selectedtext(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_doubleclicked(void *s) { return cssc_gui_editor_doubleclicked(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_rightclicked(void *s) { return cssc_gui_editor_rightclicked(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_clicked(void *s) { return cssc_gui_editor_clicked(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_scandecl(void *s, void* a0) { return cssc_gui_editor_scandecl(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void * cssc_rt_cssc_gui_editor_decltype(void *s) { return cssc_gui_editor_decltype(s); }
SYSV void * cssc_rt_cssc_gui_editor_declbase(void *s) { return cssc_gui_editor_declbase(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_declbits(void *s) { return cssc_gui_editor_declbits(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_declisauto(void *s) { return cssc_gui_editor_declisauto(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_valuebits(void *s, void* a0, void* a1) { return cssc_gui_editor_valuebits(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0), (const char *)(a1 ? (char *)a1 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_editor_saverequested(void *s) { return cssc_gui_editor_saverequested(s); }
SYSV void cssc_rt_cssc_gui_editor_undo(void *s) { cssc_gui_editor_undo(s); }
SYSV void cssc_rt_cssc_gui_editor_redo(void *s) { cssc_gui_editor_redo(s); }
SYSV void cssc_rt_cssc_gui_editor_selectall(void *s) { cssc_gui_editor_selectall(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_search(void *s, void* a0) { return cssc_gui_editor_search(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_editor_markall(void *s, void* a0) { return cssc_gui_editor_markall(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_clearsearch(void *s) { cssc_gui_editor_clearsearch(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_searchcount(void *s) { return cssc_gui_editor_searchcount(s); }
SYSV int64_t cssc_rt_cssc_gui_editor_replaceall(void *s, void* a0, void* a1) { return cssc_gui_editor_replaceall(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0), (const char *)(a1 ? (char *)a1 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_setfocus(void *s, int64_t a0) { cssc_gui_editor_setfocus(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_setscale(void *s, int64_t a0) { cssc_gui_editor_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_editor_setcolor(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_editor_setgutter(void *s, int64_t a0) { cssc_gui_editor_setgutter(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_setvisible(void *s, int64_t a0) { cssc_gui_editor_setvisible(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_setlanguage(void *s, int64_t a0) { cssc_gui_editor_setlanguage(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_setlangforpath(void *s, void* a0) { cssc_gui_editor_setlangforpath(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_editor_setownership(void *s, void* a0) { cssc_gui_editor_setownership(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_editor_update(void *s, void* a0) { return cssc_gui_editor_update(s, a0); }
SYSV void cssc_rt_cssc_gui_editor_draw(void *s) { cssc_gui_editor_draw(s); }
SYSV void *cssc_rt_cssc_gui_list_new(void* a0) { return cssc_gui_list_new(a0); }
SYSV void cssc_rt_cssc_gui_list_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_list_setrect(s, a0, a1, a2, a3); }
SYSV void cssc_rt_cssc_gui_list_add(void *s, void* a0) { cssc_gui_list_add(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_list_addat(void *s, void* a0, int64_t a1) { cssc_gui_list_addat(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0), a1); }
SYSV void cssc_rt_cssc_gui_list_clear(void *s) { cssc_gui_list_clear(s); }
SYSV int64_t cssc_rt_cssc_gui_list_count(void *s) { return cssc_gui_list_count(s); }
SYSV int64_t cssc_rt_cssc_gui_list_selected(void *s) { return cssc_gui_list_selected(s); }
SYSV int64_t cssc_rt_cssc_gui_list_rightclicked(void *s) { return cssc_gui_list_rightclicked(s); }
SYSV void * cssc_rt_cssc_gui_list_selectedtext(void *s) { return cssc_gui_list_selectedtext(s); }
SYSV void cssc_rt_cssc_gui_list_setselected(void *s, int64_t a0) { cssc_gui_list_setselected(s, a0); }
SYSV void cssc_rt_cssc_gui_list_setfocus(void *s, int64_t a0) { cssc_gui_list_setfocus(s, a0); }
SYSV void cssc_rt_cssc_gui_list_setscale(void *s, int64_t a0) { cssc_gui_list_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_list_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_list_setcolor(s, a0, a1); }
SYSV int64_t cssc_rt_cssc_gui_list_update(void *s, void* a0) { return cssc_gui_list_update(s, a0); }
SYSV void cssc_rt_cssc_gui_list_draw(void *s) { cssc_gui_list_draw(s); }
SYSV void *cssc_rt_cssc_gui_tree_new(void* a0) { return cssc_gui_tree_new(a0); }
SYSV void cssc_rt_cssc_gui_tree_setroot(void *s, void* a0) { cssc_gui_tree_setroot(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_tree_seticondir(void *s, void* a0) { cssc_gui_tree_seticondir(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_tree_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_tree_setrect(s, a0, a1, a2, a3); }
SYSV int64_t cssc_rt_cssc_gui_tree_width(void *s) { return cssc_gui_tree_width(s); }
SYSV void cssc_rt_cssc_gui_tree_refresh(void *s) { cssc_gui_tree_refresh(s); }
SYSV int64_t cssc_rt_cssc_gui_tree_count(void *s) { return cssc_gui_tree_count(s); }
SYSV int64_t cssc_rt_cssc_gui_tree_selected(void *s) { return cssc_gui_tree_selected(s); }
SYSV void * cssc_rt_cssc_gui_tree_selectedpath(void *s) { return cssc_gui_tree_selectedpath(s); }
SYSV void * cssc_rt_cssc_gui_tree_selectedname(void *s) { return cssc_gui_tree_selectedname(s); }
SYSV int64_t cssc_rt_cssc_gui_tree_selectedisdir(void *s) { return cssc_gui_tree_selectedisdir(s); }
SYSV int64_t cssc_rt_cssc_gui_tree_rightclicked(void *s) { return cssc_gui_tree_rightclicked(s); }
SYSV int64_t cssc_rt_cssc_gui_tree_dropready(void *s) { return cssc_gui_tree_dropready(s); }
SYSV void * cssc_rt_cssc_gui_tree_dropsrc(void *s) { return cssc_gui_tree_dropsrc(s); }
SYSV void * cssc_rt_cssc_gui_tree_dropdst(void *s) { return cssc_gui_tree_dropdst(s); }
SYSV void cssc_rt_cssc_gui_tree_setfocus(void *s, int64_t a0) { cssc_gui_tree_setfocus(s, a0); }
SYSV void cssc_rt_cssc_gui_tree_setscale(void *s, int64_t a0) { cssc_gui_tree_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_tree_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_tree_setcolor(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_tree_setvisible(void *s, int64_t a0) { cssc_gui_tree_setvisible(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_tree_update(void *s, void* a0) { return cssc_gui_tree_update(s, a0); }
SYSV void cssc_rt_cssc_gui_tree_draw(void *s) { cssc_gui_tree_draw(s); }
SYSV void *cssc_rt_cssc_gui_terminal_new(void* a0) { return cssc_gui_terminal_new(a0); }
SYSV void cssc_rt_cssc_gui_terminal_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_terminal_setrect(s, a0, a1, a2, a3); }
SYSV void cssc_rt_cssc_gui_terminal_setcwd(void *s, void* a0) { cssc_gui_terminal_setcwd(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_terminal_setfocus(void *s, int64_t a0) { cssc_gui_terminal_setfocus(s, a0); }
SYSV void cssc_rt_cssc_gui_terminal_setscale(void *s, int64_t a0) { cssc_gui_terminal_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_terminal_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_terminal_setcolor(s, a0, a1); }
SYSV int64_t cssc_rt_cssc_gui_terminal_isrunning(void *s) { return cssc_gui_terminal_isrunning(s); }
SYSV void cssc_rt_cssc_gui_terminal_run(void *s, void* a0) { cssc_gui_terminal_run(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_terminal_write(void *s, void* a0) { cssc_gui_terminal_write(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_terminal_writeansi(void *s, void* a0) { cssc_gui_terminal_writeansi(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_terminal_clear(void *s) { cssc_gui_terminal_clear(s); }
SYSV int64_t cssc_rt_cssc_gui_terminal_ipline(void *s) { return cssc_gui_terminal_ipline(s); }
SYSV void * cssc_rt_cssc_gui_terminal_ipfile(void *s) { return cssc_gui_terminal_ipfile(s); }
SYSV void cssc_rt_cssc_gui_terminal_setinputlock(void *s, int64_t a0) { cssc_gui_terminal_setinputlock(s, a0); }
SYSV void cssc_rt_cssc_gui_terminal_stop(void *s) { cssc_gui_terminal_stop(s); }
SYSV void cssc_rt_cssc_gui_terminal_setvisible(void *s, int64_t a0) { cssc_gui_terminal_setvisible(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_terminal_update(void *s, void* a0) { return cssc_gui_terminal_update(s, a0); }
SYSV void cssc_rt_cssc_gui_terminal_draw(void *s) { cssc_gui_terminal_draw(s); }
SYSV void *cssc_rt_cssc_gui_menu_new(void* a0) { return cssc_gui_menu_new(a0); }
SYSV void cssc_rt_cssc_gui_menu_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_menu_setrect(s, a0, a1, a2, a3); }
SYSV int64_t cssc_rt_cssc_gui_menu_addmenu(void *s, void* a0) { return cssc_gui_menu_addmenu(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_menu_additem(void *s, int64_t a0, void* a1, int64_t a2) { cssc_gui_menu_additem(s, a0, (const char *)(a1 ? (char *)a1 + 8 : (void *)0), a2); }
SYSV void cssc_rt_cssc_gui_menu_setscale(void *s, int64_t a0) { cssc_gui_menu_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_menu_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_menu_setcolor(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_menu_setrighttext(void *s, void* a0) { cssc_gui_menu_setrighttext(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV void cssc_rt_cssc_gui_menu_setrightcolor(void *s, int64_t a0) { cssc_gui_menu_setrightcolor(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_menu_isopen(void *s) { return cssc_gui_menu_isopen(s); }
SYSV int64_t cssc_rt_cssc_gui_menu_action(void *s) { return cssc_gui_menu_action(s); }
SYSV int64_t cssc_rt_cssc_gui_menu_update(void *s, void* a0) { return cssc_gui_menu_update(s, a0); }
SYSV void cssc_rt_cssc_gui_menu_draw(void *s) { cssc_gui_menu_draw(s); }
SYSV void *cssc_rt_cssc_gui_prompt_new(void* a0) { return cssc_gui_prompt_new(a0); }
SYSV void cssc_rt_cssc_gui_prompt_open(void *s, void* a0) { cssc_gui_prompt_open(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_prompt_isopen(void *s) { return cssc_gui_prompt_isopen(s); }
SYSV int64_t cssc_rt_cssc_gui_prompt_result(void *s) { return cssc_gui_prompt_result(s); }
SYSV void * cssc_rt_cssc_gui_prompt_text(void *s) { return cssc_gui_prompt_text(s); }
SYSV void cssc_rt_cssc_gui_prompt_setscale(void *s, int64_t a0) { cssc_gui_prompt_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_prompt_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_prompt_setcolor(s, a0, a1); }
SYSV int64_t cssc_rt_cssc_gui_prompt_update(void *s, void* a0) { return cssc_gui_prompt_update(s, a0); }
SYSV void cssc_rt_cssc_gui_prompt_draw(void *s) { cssc_gui_prompt_draw(s); }
SYSV void *cssc_rt_cssc_gui_tabs_new(void* a0) { return cssc_gui_tabs_new(a0); }
SYSV void cssc_rt_cssc_gui_tabs_setrect(void *s, int64_t a0, int64_t a1, int64_t a2, int64_t a3) { cssc_gui_tabs_setrect(s, a0, a1, a2, a3); }
SYSV void cssc_rt_cssc_gui_tabs_add(void *s, void* a0) { cssc_gui_tabs_add(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_tabs_indexof(void *s, void* a0) { return cssc_gui_tabs_indexof(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_tabs_count(void *s) { return cssc_gui_tabs_count(s); }
SYSV int64_t cssc_rt_cssc_gui_tabs_active(void *s) { return cssc_gui_tabs_active(s); }
SYSV void * cssc_rt_cssc_gui_tabs_activepath(void *s) { return cssc_gui_tabs_activepath(s); }
SYSV void * cssc_rt_cssc_gui_tabs_pathof(void *s, int64_t a0) { return cssc_gui_tabs_pathof(s, a0); }
SYSV void cssc_rt_cssc_gui_tabs_setactive(void *s, int64_t a0) { cssc_gui_tabs_setactive(s, a0); }
SYSV void cssc_rt_cssc_gui_tabs_remove(void *s, int64_t a0) { cssc_gui_tabs_remove(s, a0); }
SYSV int64_t cssc_rt_cssc_gui_tabs_clicked(void *s) { return cssc_gui_tabs_clicked(s); }
SYSV int64_t cssc_rt_cssc_gui_tabs_closerequested(void *s) { return cssc_gui_tabs_closerequested(s); }
SYSV void cssc_rt_cssc_gui_tabs_setscale(void *s, int64_t a0) { cssc_gui_tabs_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_tabs_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_tabs_setcolor(s, a0, a1); }
SYSV int64_t cssc_rt_cssc_gui_tabs_update(void *s, void* a0) { return cssc_gui_tabs_update(s, a0); }
SYSV void cssc_rt_cssc_gui_tabs_draw(void *s) { cssc_gui_tabs_draw(s); }
SYSV void *cssc_rt_cssc_gui_browser_new(void* a0) { return cssc_gui_browser_new(a0); }
SYSV void cssc_rt_cssc_gui_browser_open(void *s, void* a0, int64_t a1) { cssc_gui_browser_open(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0), a1); }
SYSV int64_t cssc_rt_cssc_gui_browser_isopen(void *s) { return cssc_gui_browser_isopen(s); }
SYSV int64_t cssc_rt_cssc_gui_browser_result(void *s) { return cssc_gui_browser_result(s); }
SYSV void * cssc_rt_cssc_gui_browser_chosen(void *s) { return cssc_gui_browser_chosen(s); }
SYSV void cssc_rt_cssc_gui_browser_setscale(void *s, int64_t a0) { cssc_gui_browser_setscale(s, a0); }
SYSV void cssc_rt_cssc_gui_browser_setcolor(void *s, int64_t a0, int64_t a1) { cssc_gui_browser_setcolor(s, a0, a1); }
SYSV void cssc_rt_cssc_gui_browser_seticondir(void *s, void* a0) { cssc_gui_browser_seticondir(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_browser_update(void *s, void* a0) { return cssc_gui_browser_update(s, a0); }
SYSV void cssc_rt_cssc_gui_browser_draw(void *s) { cssc_gui_browser_draw(s); }
SYSV void *cssc_rt_cssc_gui_debugger_new(void* a0) { return cssc_gui_debugger_new(a0); }
SYSV void cssc_rt_cssc_gui_debugger_start(void *s, void* a0, void* a1) { cssc_gui_debugger_start(s, (const char *)(a0 ? (char *)a0 + 8 : (void *)0), (const char *)(a1 ? (char *)a1 + 8 : (void *)0)); }
SYSV int64_t cssc_rt_cssc_gui_debugger_update(void *s) { return cssc_gui_debugger_update(s); }
SYSV int64_t cssc_rt_cssc_gui_debugger_active(void *s) { return cssc_gui_debugger_active(s); }
SYSV int64_t cssc_rt_cssc_gui_debugger_focused(void *s) { return cssc_gui_debugger_focused(s); }
SYSV void cssc_rt_cssc_gui_debugger_click(void *s, int64_t a0, int64_t a1, int64_t a2) { cssc_gui_debugger_click(s, a0, a1, a2); }
SYSV void cssc_rt_cssc_gui_debugger_draw(void *s) { cssc_gui_debugger_draw(s); }
SYSV int64_t cssc_rt_cssc_gui_debugger_ipline(void *s) { return cssc_gui_debugger_ipline(s); }
SYSV int64_t cssc_rt_cssc_gui_debugger_ipfollow(void *s) { return cssc_gui_debugger_ipfollow(s); }
SYSV void * cssc_rt_cssc_gui_debugger_ipfile(void *s) { return cssc_gui_debugger_ipfile(s); }
SYSV void * cssc_rt_cssc_gui_debugger_takeoutput(void *s) { return cssc_gui_debugger_takeoutput(s); }
SYSV int64_t cssc_rt_cssc_gui_debugger_held(void *s) { return cssc_gui_debugger_held(s); }
SYSV void cssc_rt_cssc_gui_debugger_clearheld(void *s) { cssc_gui_debugger_clearheld(s); }
SYSV void cssc_rt_cssc_gui_debugger_close(void *s) { cssc_gui_debugger_close(s); }
SYSV void cssc_rt_cssc_gui_debugger_key(void *s, int64_t a0) { cssc_gui_debugger_key(s, a0); }
SYSV void cssc_rt_cssc_gui_debugger_char(void *s, int64_t a0) { cssc_gui_debugger_char(s, a0); }
/* GUIGEN-WRAPPERS-END */

/* ==== video:: module — SysV wrappers over cssc_host_game.c / _extras.c ======
 * `#include("video") vid; #video[w,h,fps] s; vid::clear(s,c); …` — the video
 * context is cssc_video_new (a headless BGRA backbuffer, w*h uint32). Methods
 * take the handle as arg-0. Same SysV<->MS-ABI bridge as the gui wrappers. */
extern void   *cssc_video_new(int64_t w, int64_t h, int64_t fps);
extern void    cssc_video_clear(void *p, int64_t argb);
extern void    cssc_video_pixel(void *p, int64_t x, int64_t y, int64_t argb);
extern int64_t cssc_video_get_pixel(void *p, int64_t x, int64_t y);
extern void    cssc_video_fillrect(void *p, int64_t x, int64_t y, int64_t w, int64_t h, int64_t argb);
extern void    cssc_video_draw_rect(void *p, int64_t x, int64_t y, int64_t w, int64_t h, int64_t argb);
extern void    cssc_video_draw_text(void *p, int64_t x, int64_t y, const char *text, int64_t argb, int64_t scale);
extern void    cssc_video_present(void *p);

SYSV void   *cssc_rt_video_new(int64_t w, int64_t h, int64_t fps) { return cssc_video_new(w, h, fps); }
SYSV void    cssc_rt_video_clear(void *p, int64_t argb)           { cssc_video_clear(p, argb); }
SYSV void    cssc_rt_video_pixel(void *p, int64_t x, int64_t y, int64_t argb) { cssc_video_pixel(p, x, y, argb); }
SYSV int64_t cssc_rt_video_get_pixel(void *p, int64_t x, int64_t y) { return cssc_video_get_pixel(p, x, y); }
SYSV void    cssc_rt_video_fillrect(void *p, int64_t x, int64_t y, int64_t w, int64_t h, int64_t argb) { cssc_video_fillrect(p, x, y, w, h, argb); }
SYSV void    cssc_rt_video_draw_rect(void *p, int64_t x, int64_t y, int64_t w, int64_t h, int64_t argb) { cssc_video_draw_rect(p, x, y, w, h, argb); }
SYSV void    cssc_rt_video_draw_text(void *p, int64_t x, int64_t y, void *str, int64_t argb, int64_t scale) {
    const char *text = str ? (const char *)((char *)str + 8) : "";
    cssc_video_draw_text(p, x, y, text, argb, scale);
}
SYSV void    cssc_rt_video_present(void *p)                       { cssc_video_present(p); }

/* Deterministic FNV-1a checksum of the video backbuffer (w*h BGRA DWORDs) — the
 * verify handle for byte-parity testing, exactly like the gui screen's. The
 * cssc_video_t struct starts {int32 w; int32 h; …; uint32* backbuf@16}. */
SYSV int64_t cssc_rt_video_checksum(void *p) {
    if (!p) return 0;
    int64_t w = (int64_t)(*(int32_t *)((char *)p + 0));
    int64_t h = (int64_t)(*(int32_t *)((char *)p + 4));
    uint32_t *fb = cssc_video_backbuf(p);
    if (!fb || w <= 0 || h <= 0) return 0;
    uint64_t hsh = 1469598103934665603ULL;
    int64_t n = w * h;
    for (int64_t i = 0; i < n; i++) {
        uint32_t px = fb[i];
        hsh = (hsh ^ (uint64_t)(px & 0xFFu))         * 1099511628211ULL;
        hsh = (hsh ^ (uint64_t)((px >> 8) & 0xFFu))  * 1099511628211ULL;
        hsh = (hsh ^ (uint64_t)((px >> 16) & 0xFFu)) * 1099511628211ULL;
        hsh = (hsh ^ (uint64_t)((px >> 24) & 0xFFu)) * 1099511628211ULL;
    }
    return (int64_t)(hsh & 0x7FFFFFFFFFFFFFFFULL);
}
