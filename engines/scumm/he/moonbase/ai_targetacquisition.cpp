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

#include "scumm/he/intern_he.h"
#include "scumm/he/moonbase/moonbase.h"

#include "scumm/he/moonbase/ai_targetacquisition.h"
#include "scumm/he/moonbase/ai_main.h"
#include "scumm/he/moonbase/ai_weapon.h"

namespace Scumm {

int Sortie::_sSourceX = 0;
int Sortie::_sSourceY = 0;

int Sortie::_sTargetX = 0;
int Sortie::_sTargetY = 0;

Sortie::~Sortie() {
	for (Common::Array<DefenseUnit *>::iterator k = _enemyDefenses.begin(); k != _enemyDefenses.end(); ++k) {
		delete *k;
	}
}

void Sortie::setEnemyDefenses(int enemyDefensesScummArray, int defendX, int defendY) {
	DefenseUnit *thisUnit;
	int currentPlayer = GetCurrentPlayer();

	for (int i = 0; i < 200; ++i) {
		int thisElement = _vm->_moonbase->readFromArray(enemyDefensesScummArray, 0, i);

		if (thisElement) {
			if (GetBuildingOwner(thisElement)) {
				if (GetPlayerTeam(currentPlayer) != GetBuildingTeam(thisElement)) {
					int type = GetBuildingType(thisElement);

					switch (type) {
					case BUILDING_ANTI_AIR:
						thisUnit = new AntiAirUnit();
						break;

					case BUILDING_SHIELD:
						thisUnit = new ShieldUnit();
						break;

					case BUILDING_EXPLOSIVE_MINE:
						if (GetDistance(GetHubX(thisElement), GetHubY(thisElement), defendX, defendY) < 90)
							thisUnit = new MineUnit();
						else
							thisUnit = NULL;

						break;

					case BUILDING_CRAWLER:
						thisUnit = NULL;
						break;

					default:
						thisUnit = NULL;
						break;
					}

					if (thisUnit != NULL) {
						thisUnit->setID(thisElement);
						thisUnit->setPos(GetHubX(thisElement), GetHubY(thisElement));

						if (GetBuildingState(thisElement)) thisUnit->setState(DUS_OFF);

						_enemyDefenses.push_back(thisUnit);
					}
				}
			}
		} else {
			i = 200;
		}
	}
}

int *Sortie::getShotPos() const {
	int *retVal = new int[2];

	retVal[0] = _shotPosX;
	retVal[1] = _shotPosY;

	return retVal;
}

int Sortie::numChildrenToGen() {
	int retVal = MAX<uint>(_enemyDefenses.size(), 1) * NUM_SHOT_POSITIONS * NUM_WEAPONS;
	return retVal;
}

IContainedObject *Sortie::createChildObj(int index, int &completionFlag) {
	float thisDamage;
	Sortie *retSortie = new Sortie;
	int activeDefenses = 0;

	Common::Array<DefenseUnit *> thisEnemyDefenses;

	// Copy the defensive unit list from the parent
	for (Common::Array<DefenseUnit *>::iterator k = _enemyDefenses.begin(); k != _enemyDefenses.end(); ++k) {
		DefenseUnit *temp;

		switch ((*k)->getType()) {
		case DUT_ANTI_AIR:
			temp = new AntiAirUnit(*k);
			break;

		case DUT_SHIELD:
			temp = new ShieldUnit(*k);
			break;

		case DUT_MINE:
			temp = new MineUnit(*k);
			break;

		case DUT_CRAWLER:
			temp = new CrawlerUnit(*k);
			break;

		default:
			temp = new ShieldUnit(*k);
			break;
		}

		thisEnemyDefenses.push_back(temp);
	}

	// Calculate the current target from the index
	DefenseUnit *currentTarget = *(thisEnemyDefenses.begin() + static_cast<int>(index / (NUM_WEAPONS * NUM_SHOT_POSITIONS)));

	assert(currentTarget);

	// Pick correct weapon according to index
	Weapon *currentWeapon = new Weapon(currentTarget->selectWeapon(index % NUM_WEAPONS));
	retSortie->setUnitType(currentWeapon->getTypeID());

	// Calculate distance from target to source hub
	int distance = GetDistance(currentTarget->getPosX(), currentTarget->getPosY(), getSourcePosX(), getSourcePosY());

	// Pick correct shot position according to index
	Common::Point *targetCoords;
	targetCoords = currentTarget->createTargetPos((static_cast<int>(index / NUM_WEAPONS) % NUM_SHOT_POSITIONS), distance, currentWeapon->getTypeID(), getSourcePosX(), getSourcePosY());
	retSortie->setShotPos(targetCoords->x, targetCoords->y);

	// Set the g value based on cost of the weapon
	retSortie->setValueG(getG() + currentWeapon->getCost());

	int AAcounter = 3;

	// Loop through defensive units, toggling anti-air units and deciding if this weapon will land safely
	for (Common::Array<DefenseUnit *>::iterator i = thisEnemyDefenses.begin(); i != thisEnemyDefenses.end(); ++i) {
		distance = GetDistance((*i)->getPosX(), (*i)->getPosY(), targetCoords->x, targetCoords->y);

		// Check to see if we're within an active defense's radius
		if ((distance < (*i)->getRadius()) && ((*i)->getState() == DUS_ON)) {
			activeDefenses++;

			// Turn off this anti-air and drop the coverage count
			if (((*i)->getType() == DUT_ANTI_AIR)) {
				(*i)->setState(DUS_OFF);

				if (currentWeapon->getTypeID() == ITEM_CLUSTER)
					AAcounter--;
				else
					AAcounter = 0;
			}

			// Essentially disable this weapon choice, due to its impact with a shield, or untriggered anti-air
			if (((*i)->getType() == DUT_SHIELD)  || !AAcounter) {
				retSortie->setValueG(1000);
				i = thisEnemyDefenses.end() - 1;
			}
		} else {
			// Turn on any anti-airs that were off the previous turn
			if (((*i)->getType() == DUT_ANTI_AIR) && ((*i)->getState() == DUS_OFF))
				(*i)->setState(DUS_ON);
		}
	}

	// Turn on all the non-anti-air units in preparation for emp's and the next turn
	for (Common::Array<DefenseUnit *>::iterator i = thisEnemyDefenses.begin(); i != thisEnemyDefenses.end(); ++i) {
		if ((*i)->getType() != DUT_ANTI_AIR) {
			(*i)->setState(DUS_ON);
		}
	}

	// If this weapon is still valid
	if (retSortie->getValueG() < 1000) {
		// Apply emp effects and damage to all units in range of weapon
		for (Common::Array<DefenseUnit *>::iterator i = thisEnemyDefenses.begin(); i != thisEnemyDefenses.end(); ) {
			// Special simulated crawler detonation location used, since it walks a bit
			if (currentWeapon->getTypeID() == ITEM_CRAWLER)
				distance = GetDistance((*i)->getPosX(), (*i)->getPosY(), currentTarget->getPosX(), currentTarget->getPosY());
			// Normal detonation location used here
			else {
				distance = GetDistance((*i)->getPosX(), (*i)->getPosY(), targetCoords->x, targetCoords->y);
			}

			if (distance < currentWeapon->getRadius()) {
				// Apply damage
				thisDamage = currentWeapon->getDamage();

				if ((AAcounter != 3) && (currentWeapon->getTypeID() == ITEM_CLUSTER))
					thisDamage = 0;

				if (!_vm->_rnd.getRandomNumber(4))
					currentWeapon->setTypeID(ITEM_MINE);

				(*i)->setDamage(thisDamage);

				// Apply emp effect
				if (currentWeapon->getTypeID() == ITEM_EMP) {
					(*i)->setState(DUS_OFF);
				}

				// Remove destroyed defenses
				if ((*i)->getArmor() <= 0) {
					delete *i;
					i = thisEnemyDefenses.erase(i);
				} else {
					++i;
				}
			} else {
				++i;
			}
		}
	}

	retSortie->setEnemyDefenses(thisEnemyDefenses);

	delete targetCoords;
	delete currentWeapon;
	return retSortie;
}

float Sortie::calcH() {
	float retValue = 0;
	Common::Array<DefenseUnit *> thisEnemyDefenses = getEnemyDefenses();

	for (Common::Array<DefenseUnit *>::iterator i = thisEnemyDefenses.begin(); i != thisEnemyDefenses.end(); ++i) {
		if ((*i)->getState() == DUS_ON) {
			switch ((*i)->getType()) {
			case DUT_ANTI_AIR:
				retValue += 1;

			case DUT_MINE:
				retValue += 1;
				break;

			case DUT_SHIELD:
				retValue += 1;
				break;
			}
		}
	}

	return retValue;
}

int Sortie::checkSuccess() {
	if (!_enemyDefenses.size()) return SUCCESS;

	int targetX = getTargetPosX();
	int targetY = getTargetPosY();

	int targetCheck = 0;

	for (Common::Array<DefenseUnit *>::iterator i = _enemyDefenses.begin(); i != _enemyDefenses.end(); ++i) {
		if (((*i)->getState() == DUS_ON) && ((*i)->getType() != DUT_HUB)) {
			return 0;
		}

		if (((*i)->getPosX() == targetX) && ((*i)->getPosY() == targetY)) targetCheck = 1;
	}

	if (!targetCheck)
		return SUCCESS;

	// If shot pos == target pos return SUCCESS;
	if ((targetX == getShotPosX()) && (getTargetPosY() == getShotPosY())) {
		return SUCCESS;
	}

	return 0;
}

float Sortie::calcT() {
	return (checkSuccess() != SUCCESS) ? (getG() + calcH()) : SUCCESS;
}

IContainedObject *Sortie::duplicate() {
	return this;
}


void Sortie::printEnemyDefenses() {
	for (Common::Array<DefenseUnit *>::iterator i = _enemyDefenses.begin(); i != _enemyDefenses.end(); ++i) {
		warning("Unit %d - Type: %d, Armor: %d, Status: %d", (*i)->getID(), (*i)->getType(), static_cast<int>((*i)->getArmor()), (*i)->getState());
	}
}

int Defender::calculateDefenseUnitPosition(int targetX, int targetY, int index) {
	int currentPlayer = GetCurrentPlayer();

	//Get list of near hubs
	int unitsArray = GetUnitsWithinRadius(targetX + 5, targetY, 480);

	const int NUM_HUBS = 10;
	//Order on dist
	int hubArray[NUM_HUBS] = { 0 };
	int hubIndex = 0;

	for (int i = 0; i < 200; ++i) {
		int thisUnit = _vm->_moonbase->readFromArray(unitsArray, 0, i);

		if (thisUnit) {
			if (((GetBuildingType(thisUnit) == BUILDING_MAIN_BASE) || (GetBuildingType(thisUnit) == BUILDING_OFFENSIVE_LAUNCHER))  && (GetBuildingOwner(thisUnit) == currentPlayer)) {
				for (int j = 0; j < NUM_HUBS; ++j) {
					if (hubArray[j]) {
						int distCurrent = GetDistance(targetX, targetY, GetHubX(thisUnit), GetHubY(thisUnit));
						int distSaved = GetDistance(targetX, targetY, GetHubX(hubArray[j]), GetHubY(hubArray[j]));

						if (distCurrent < distSaved) {
							hubArray[hubIndex] = hubArray[j];
							hubArray[j] = thisUnit;
							++hubIndex;
							j = 100;
						}
					} else {
						hubArray[j] = thisUnit;
						++hubIndex;
						j = 100;
					}
				}
			}
		}

		if (hubIndex >= NUM_HUBS) {
			hubIndex = NUM_HUBS;
			i = 200;
		}
	}

	_vm->nukeArray(unitsArray);

	//Check if repair is needed
	int targetUnit = GetClosestUnit(targetX + 5, targetY, 15, currentPlayer, 1, 0, 0, 0);

	if (targetUnit && (targetUnit != BUILDING_CRAWLER) && (GetBuildingTeam(targetUnit) == GetPlayerTeam(currentPlayer))) {
		int armor = GetBuildingArmor(targetUnit);

		if (armor < GetBuildingMaxArmor(targetUnit)) {
			unitsArray = GetUnitsWithinRadius(targetX + 5, targetY, 170);
			int defCount = 0;

			for (int i = 0; i < 200; ++i) {
				int thisUnit = _vm->_moonbase->readFromArray(unitsArray, 0, i);

				if (thisUnit) {
					if (((GetBuildingType(thisUnit) == BUILDING_SHIELD) || (GetBuildingType(thisUnit) == BUILDING_ANTI_AIR)) && (GetBuildingOwner(thisUnit) == currentPlayer) && (GetBuildingState(thisUnit) == 0)) {
						++defCount;
						i = 200;
					}
				}
			}

			_vm->nukeArray(unitsArray);

			if (defCount) {
				//repair
				int hubUnit = GetClosestUnit(targetX, targetY, 480, currentPlayer, 1, BUILDING_MAIN_BASE, 1, 110);

				if (hubUnit && (hubUnit != targetUnit)) {
					int powAngle = abs(GetPowerAngleFromPoint(GetHubX(hubUnit), GetHubY(hubUnit), targetX, targetY, 20));
					int power = powAngle / 360;
					int angle = powAngle - (power * 360);

					setTargetX(targetX);
					setTargetY(targetY);

					setSourceUnit(hubUnit);
					setUnit(ITEM_REPAIR);
					setPower(power);
					setAngle(angle);

					return 1;
				}
			}
		}
	}

	//For each hub
	for (int i = 0; i < MIN(NUM_HUBS, hubIndex); ++i) {
		int hubX = GetHubX(hubArray[i]);
		int hubY = GetHubY(hubArray[i]);
		//Get angle to hub
		int directAngleToHub = 0;

		//If this hub is the target
		if ((hubX == targetX) && (hubY == targetY)) {
			//make the angle seed point at the closest enemy
			int enemyUnit = GetClosestUnit(hubX, hubY, GetMaxX(), currentPlayer, 0, 0, 0);
			directAngleToHub = GetAngle(targetX, targetY, GetHubX(enemyUnit), GetHubY(enemyUnit));
		} else {
			directAngleToHub = GetAngle(targetX, targetY, hubX, hubY);
		}

		//Number of random chances to land
		for (int j = 0; j < 3; ++j) {
			//Pick random angle and dist within semicircle (-90 to +90) and (40 to 150)
			int randAngle = directAngleToHub + _vm->_rnd.getRandomNumber(179) - 90;
			int randDist = _vm->_rnd.getRandomNumber(109) + 40;

			int x = targetX + randDist * cos(degToRad(randAngle));
			int y = targetY + randDist * sin(degToRad(randAngle));

			int powAngle = GetPowerAngleFromPoint(hubX, hubY, x, y, 20);

			if (powAngle < 0)
				continue;

			int power = powAngle / 360;
			int angle = powAngle - (power * 360);

			int coords = 0;
			coords = SimulateBuildingLaunch(hubX, hubY, power, angle, 100, 0);

			//if valid, return
			if (coords > 0) {
				//warning("The prospective launching hub for this defensive unit is: %d", hubArray[i]);

				setSourceX(hubX);
				setSourceY(hubY);
				setTargetX((x + GetMaxX()) % GetMaxX());
				setTargetY((y + GetMaxY()) % GetMaxY());
				setSourceUnit(hubArray[i]);

				int unitsArray2 = GetUnitsWithinRadius(targetX + 5, targetY, 200);
				int shieldCount = 0;

				for (int k = 0; k < 200; ++k) {
					int thisUnit = _vm->_moonbase->readFromArray(unitsArray2, 0, k);

					if (thisUnit) {
						if ((GetBuildingType(thisUnit) == BUILDING_SHIELD) && (GetBuildingOwner(thisUnit) == currentPlayer))
							shieldCount++;

						if ((GetBuildingType(thisUnit) == BUILDING_BRIDGE) && (GetBuildingOwner(thisUnit) == currentPlayer)) {
							shieldCount--;
							shieldCount = MAX(-1, shieldCount);
						}
					}
				}

				if ((_vm->_rnd.getRandomNumber((int)pow(3, shieldCount + 1) - 1) == 0) && (GetPlayerEnergy() > 6))
					setUnit(ITEM_SHIELD);
				else
					setUnit(ITEM_ANTIAIR);

				setPower(power);
				setAngle(angle);

				_vm->nukeArray(unitsArray2);
				return 1;
			}

			if (coords < 0) {
				//drop a bridge for the cord
				int yCoord  = -coords / GetMaxX();
				int xCoord = -coords - (yCoord * GetMaxX());

				if (CheckIfWaterState(xCoord, yCoord)) {

					int terrainSquareSize = GetTerrainSquareSize();
					xCoord = ((xCoord / terrainSquareSize * terrainSquareSize) + (terrainSquareSize / 2));
					yCoord = ((yCoord / terrainSquareSize * terrainSquareSize) + (terrainSquareSize / 2));

					int xDist = xCoord - x;
					int yDist = yCoord - y;
					x = xCoord + (terrainSquareSize * 1.414 * (xDist / (abs(xDist) + 1)));
					y = yCoord + (terrainSquareSize * 1.414 * (yDist / (abs(yDist) + 1)));

					setTargetX(x);
					setTargetY(y);

					int nextUnit = GetClosestUnit(x, y, 480, GetCurrentPlayer(), 1, BUILDING_MAIN_BASE, 1, 120);
					powAngle = GetPowerAngleFromPoint(GetHubX(nextUnit), GetHubY(nextUnit), x, y, 15);

					powAngle = abs(powAngle);
					power = powAngle / 360;
					angle = powAngle - (power * 360);

					setSourceUnit(nextUnit);
					setUnit(ITEM_BRIDGE);
					setPower(power);
					setAngle(angle);

					return 1;
				}
			}
		}
	}

	// Else create new hub
	int count = 0;
	int coords = 0;

	if (hubIndex == 0) return -3;

	do {
		int sourceHub = hubArray[_vm->_rnd.getRandomNumber(hubIndex - 1)];

		setSourceX(GetHubX(sourceHub));
		setSourceY(GetHubY(sourceHub));
		setSourceUnit(sourceHub);
		setUnit(ITEM_HUB);
		setPower(_vm->_rnd.getRandomNumber(299) + 200);
		setAngle(_vm->_rnd.getRandomNumber(359));
		count++;

		if (count > (NUM_HUBS * 3)) break;

		coords = SimulateBuildingLaunch(getSourceX(), getSourceY(), getPower(), getAngle(), 100, 0);
	} while (coords <= 0);

	if (coords > 0) {
		setTargetX(coords % GetMaxX());
		setTargetY(coords / GetMaxX());
	} else {
		setTargetX(0);
		setTargetY(0);
	}

	return -1;
}

} // End of namespace Scumm