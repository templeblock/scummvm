/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * MIT License:
 *
 * Copyright (c) 2009 Alexei Svitkine, Eugene Sandulenko
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef GRAPHICS_MACGUI_MACMENU_H
#define GRAPHICS_MACGUI_MACMENU_H

namespace Graphics {

struct MenuItem;
struct MenuSubItem;

struct MenuData {
	int menunum;
	const char *title;
	int action;
	byte shortcut;
	bool enabled;
};

class Menu : public BaseMacWindow {
public:
	Menu(int id, const Common::Rect &bounds, MacWindowManager *wm);
	~Menu();

	void setCommandsCallback(void (*callback)(int, Common::String &, void *), void *data) { _ccallback = callback; _cdata = data; }

	void addStaticMenus(const MenuData *data);
	void calcDimensions();

	int addMenuItem(const char *name);
	void addMenuSubItem(int id, const char *text, int action, int style = 0, char shortcut = 0, bool enabled = true);
	void createSubMenuFromString(int id, const char *string);
	void clearSubMenu(int id);

	bool draw(ManagedSurface *g, bool forceRedraw = false);
	bool processEvent(Common::Event &event);

	void enableCommand(int menunum, int action, bool state);
	void disableAllMenus();

	void setActive(bool active) { _menuActivated = active; }
	bool hasAllFocus() { return _menuActivated; }

	Common::Rect _bbox;

private:
	ManagedSurface _screen;
	ManagedSurface _tempSurface;

private:
	const Font *getMenuFont();
	const char *getAcceleratorString(MenuSubItem *item, const char *prefix);
	int calculateMenuWidth(MenuItem *menu);
	void calcMenuBounds(MenuItem *menu);
	void renderSubmenu(MenuItem *menu);

	bool keyEvent(Common::Event &event);
	bool mouseClick(int x, int y);
	bool mouseRelease(int x, int y);
	bool mouseMove(int x, int y);

	bool processMenuShortCut(byte flags, uint16 ascii);

	Common::Array<MenuItem *> _items;

	const Font *_font;

	bool _menuActivated;

	int _activeItem;
	int _activeSubItem;

	void (*_ccallback)(int action, Common::String &text, void *data);
	void *_cdata;
};

} // End of namespace Graphics

#endif
