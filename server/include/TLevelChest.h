#ifndef TLEVELCHEST_H
#define TLEVELCHEST_H

class TLevelChest
{
	public:
		TLevelChest(char nx, char ny, char itemIdx, char signIdx)
			: itemIndex(itemIdx), signIndex(signIdx), x(nx), y(ny) {
		}

		inline int getItemIndex() const {
			return itemIndex;
		}

		inline int getSignIndex() const {
			return signIndex;
		}

		inline int getX() const {
			return x;
		}

		inline int getY() const {
			return y;
		}

	private:
		int itemIndex, signIndex, x, y;
};


#endif // TLEVELCHEST_H
