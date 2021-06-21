#ifndef TLEVELCHEST_H
#define TLEVELCHEST_H

enum class LevelItemType;

class TLevelChest
{
	public:
		TLevelChest(char nx, char ny, LevelItemType itemIdx, char signIdx)
			: itemIndex(itemIdx), signIndex(signIdx), x(nx), y(ny) {
		}

		LevelItemType getItemIndex() const {
			return itemIndex;
		}

		int getSignIndex() const {
			return signIndex;
		}

		int getX() const {
			return x;
		}

		int getY() const {
			return y;
		}

	private:
		LevelItemType itemIndex;
		int signIndex, x, y;
};


#endif // TLEVELCHEST_H
