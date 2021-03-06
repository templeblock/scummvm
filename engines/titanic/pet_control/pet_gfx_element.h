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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef TITANIC_PET_GFX_ELEMENT_H
#define TITANIC_PET_GFX_ELEMENT_H

#include "titanic/pet_control/pet_element.h"

namespace Titanic {

class CPetGfxElement: public CPetElement {
public:
	CGameObject *_object0;
	CGameObject *_object1;
	CGameObject *_object2;
public:
	CPetGfxElement() : CPetElement(), _object0(nullptr), _object1(nullptr),
		_object2(nullptr) {}

	/**
	 * Setup the element
	 */
	virtual void setup(PetElementMode mode, const CString &name,
		CPetControl *petControl);

	/**
	 * Reset the element
	 */
	virtual void reset(const CString &name, CPetControl *petControl,
		PetElementMode mode = MODE_UNSELECTED);

	/**
	 * Draw the item
	 */
	virtual void draw(CScreenManager *screenManager);

	/**
	 * Draw the item
	 */
	virtual void draw(CScreenManager *screenManager, const Common::Point &destPos);

	/**
	 * Get the bounds for the element
	 */
	virtual Rect getBounds() const;

	/**
	 * Get the game object associated with this item
	 */
	virtual CGameObject *getObject() const;
};

} // End of namespace Titanic

#endif /* TITANIC_PET_GFX_ELEMENT_H */
