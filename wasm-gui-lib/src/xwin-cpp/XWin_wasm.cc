#include "x++/XWin.h"
#include "x++/X.h"
#include <stdio.h>
#include <string.h>
#include <font/font.h>
#include <wasm_ewok_compat.h>

using namespace Ewok;

static void _on_repaint(xwin_t* xw, graph_t* g) {
	if(xw == NULL || g == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doRepaint(g);
}

static void _on_min(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doMin();
}

static void _on_resize(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;

	xwin->__doResize();
}

static void _on_update_theme(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;

	XTheme* theme = xwin->getTheme();
	if(theme == NULL)
		return;
	theme->loadSystem();
}

static void _on_move(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doMove();
}

static bool _on_close(xwin_t* xw) {
	if(xw == NULL)
		return false;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return false;
	return xwin->__doClose();
}

static void _on_focus(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doFocus();
}

static void _on_unfocus(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doUnfocus();
}

static void _on_reorg(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doReorg();
}

static void _on_event(xwin_t* xw, xevent_t* ev) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doEvent(ev);
}

XWin::XWin() {
	xwin = NULL;
	theme = new XTheme();
}

XWin::~XWin() {
	close();
	if(theme != NULL)
		delete theme;
}

bool XWin::open(X* x, int32_t display, int32_t x_pos, int32_t y_pos, int32_t w, int32_t h, const char* title, uint32_t style) {
	if(xwin != NULL)
		return false;
	
	xwin = xwin_open(&x->x, display, x_pos, y_pos, w, h, title, style);
	if(xwin == NULL)
		return false;

	xwin->data = this;
	xwin->on_repaint = _on_repaint;
	xwin->on_min = _on_min;
	xwin->on_resize = _on_resize;
	xwin->on_update_theme = _on_update_theme;
	xwin->on_move = _on_move;
	xwin->on_close = _on_close;
	xwin->on_focus = _on_focus;
	xwin->on_unfocus = _on_unfocus;
	xwin->on_reorg = _on_reorg;
	xwin->on_event = _on_event;
	return true;
}

void XWin::close(void) {
	if(xwin == NULL)
		return;
	xwin_close(xwin);
	xwin = NULL;
}

bool XWin::isOpened(void) {
	return (xwin != NULL);
}

void XWin::setVisible(bool visible) {
	if(xwin == NULL)
		return;
	xwin_set_visible(xwin, visible);
}

bool XWin::isVisible(void) {
	if(xwin == NULL || xwin->xinfo == NULL)
		return false;
	return xwin->xinfo->visible;
}

void XWin::repaint(void) {
	if(xwin == NULL)
		return;
	xwin_repaint(xwin);
}

int32_t XWin::getX(void) {
	if(xwin == NULL || xwin->xinfo == NULL)
		return 0;
	return xwin->xinfo->wsr.x;
}

int32_t XWin::getY(void) {
	if(xwin == NULL || xwin->xinfo == NULL)
		return 0;
	return xwin->xinfo->wsr.y;
}

int32_t XWin::getW(void) {
	if(xwin == NULL || xwin->xinfo == NULL)
		return 0;
	return xwin->xinfo->wsr.w;
}

int32_t XWin::getH(void) {
	if(xwin == NULL || xwin->xinfo == NULL)
		return 0;
	return xwin->xinfo->wsr.h;
}

void XWin::moveTo(int32_t x, int32_t y) {
	if(xwin == NULL)
		return;
	xwin_move_to(xwin, x, y);
}

void XWin::move(int32_t x, int32_t y) {
	if(xwin == NULL)
		return;
	xwin_move(xwin, x, y);
}

void XWin::resize(int32_t w, int32_t h) {
	if(xwin == NULL)
		return;
	xwin_resize(xwin, w, h);
}

void XWin::resizeTo(int32_t w, int32_t h) {
	if(xwin == NULL)
		return;
	xwin_resize_to(xwin, w, h);
}

graph_t* XWin::getGraph(graph_t* g) {
	if(xwin == NULL)
		return NULL;
	return xwin_fetch_graph(xwin, g);
}

const char* XWin::getTitle(void) {
	if(xwin == NULL || xwin->xinfo == NULL)
		return "";
	return xwin->xinfo->title;
}

XTheme* XWin::getTheme(void) {
	return theme;
}

void XWin::__doRepaint(graph_t* g) {
	onRepaint(g);
}

void XWin::__doMin() {
	onMin();
}

void XWin::__doResize() {
	onResize();
}

void XWin::__doMove() {
	onMove();
}

bool XWin::__doClose() {
	return onClose();
}

void XWin::__doFocus() {
	onFocus();
}

void XWin::__doUnfocus() {
	onUnfocus();
}

void XWin::__doReorg() {
	onReorg();
}

void XWin::__doEvent(xevent_t* ev) {
	onEvent(ev);
}

// Virtual event handlers - can be overridden by subclasses
void XWin::onRepaint(graph_t* g) {
	// Default: clear to white
	if(g) {
		graph_clear(g, 0xffffffff);
	}
}

void XWin::onMin() {
}

void XWin::onResize() {
}

void XWin::onMove() {
}

bool XWin::onClose() {
	return true; // Allow close by default
}

void XWin::onFocus() {
}

void XWin::onUnfocus() {
}

void XWin::onReorg() {
}

void XWin::onEvent(xevent_t* ev) {
}