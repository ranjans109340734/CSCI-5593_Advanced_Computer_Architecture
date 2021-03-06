#ifndef __MEM_RUBY_STRUCTURES_ADPGPOLICY_HH__
#define __MEM_RUBY_STRUCTURES_ADPGPOLICY_HH__
#include "mem/ruby/structures/AbstractReplacementPolicy.hh"
#include "params/ADPGReplacementPolicy.hh"
#include "debug/ACA.hh"
#define PARTS 4

struct Cell {
	uint8_t priority :2;

	Cell() :
			priority(0) {
	}
};

class Partition {
	int ptr = 0, prePTR = 0, histPTR = 0;
	int startSet, endSet;

public:

	void init(int startSet, int endSet) {
		this->startSet = startSet;
		this->endSet = endSet;
	}

	void setPTR(Cell **cache, int m_assoc) {
		ptr = 0;
		for (int i = this->startSet; i <= this->endSet; i++) {
			for (int j = 0; j < m_assoc; j++) {
				ptr += cache[i][j].priority;
			}
		}
	}

	bool getPTRFluctuation() {
		if (ptr > prePTR)
			return 1; //increment in partition
		else
			return 0; //equals or decrement in partition value
	}

	int getEndSet() const {
		return endSet;
	}

	int getStartSet() const {
		return startSet;
	}

	int getPtr() const {
		return ptr;
	}

	int getPrePtr() const {
		return prePTR;
	}

	void setPrePtr(int prePtr = 0) {
		prePTR = prePtr;
	}

	int getHistPtr() const {
		return histPTR;
	}

	void setHistPtr(int histPtr = 0) {
		histPTR = histPtr;
	}
};

/* Simple true ADPG replacement policy */

class ADPGPolicy: public AbstractReplacementPolicy {
public:
	typedef ADPGReplacementPolicyParams Params;
	ADPGPolicy(const Params * p);
	~ADPGPolicy();

	void touch(int64_t set, int64_t way, Tick time);
	int64_t getVictim(int64_t set);

	Cell **cache = new Cell*[m_num_sets];
	int *sub = new int[m_num_sets];
	int gTR = 0, preGTR = 0, maxGTR = 0, access_count = 0; // GTR Registers
	uint8_t state = 1, victim_count = 0, skip_call_count = 0;
	bool replace_flag = 0;

	Partition partition[PARTS];

	void getPartitions(int m_num_sets) {
		int partValue = m_num_sets / PARTS;
		int startSetIndex = 0, endSetIndex = (partValue - 1), i;

		for (i = 0; i < PARTS - 1; i++) {

			partition[i].init(startSetIndex, endSetIndex);
			startSetIndex = endSetIndex + 1;
			endSetIndex += partValue;

		}

		partition[i].init(startSetIndex, m_num_sets - 1);

	}

	void setGTR() {
		gTR = 0;

		for (int i = 0; i < PARTS; i++) {
			gTR += partition[i].getPtr();
		}

		if (maxGTR < gTR) {
			maxGTR = gTR;
		}

	}

	int getGTR() {
		return gTR;
	}

	int getPreGTR() {
		return preGTR;
	}

	void changeStateSet1() {
		switch (state) {
		case 1: {
			state = 2;
			break;
		}
		case 2: {
			state = 3;
			break;
		}
		}
	}

	void changeStateSet2() {
		switch (state) {
		case 3: {
			state = 4;
			break;
		}
		case 4: {
			state = 1;
			break;
		}
		}
	}

	void setState() {

		if (access_count % (m_num_sets / 2) == 0) {
			//if (access_count > 10000) {
			access_count = 0;
			preGTR = 0;
			for (int i = 0; i < PARTS; i++) {
				if (partition[i].getPtr() == partition[i].getPrePtr()) {
					partition[i].setPrePtr(partition[i].getHistPtr());
				} else {
					partition[i].setHistPtr(partition[i].getPrePtr());
				}
				preGTR += partition[i].getPrePtr();
			}

			//if ((preGTR - gTR) > (maxGTR / 32)) {
			if ((preGTR - gTR) > 4) {
				bool flag = 1;
				for (int i = 0; i < PARTS; i++) {
					if (partition[i].getPTRFluctuation()) {
						flag = 0;
						break;
					}
				}

				if (flag) {
					changeStateSet1();
				}

				//} else if (preGTR != 0 && ((gTR - preGTR) > (maxGTR / 64))) {
			} else if (preGTR != 0 && ((gTR - preGTR) > 4)) {
				changeStateSet2();
			}

			/*
			 for (int i = 0; i < PARTS; i++)
			 DPRINTF(ACA, "For Partition %d PTR = %d , prePTR = %d \n", i,
			 partition[i].getPtr(), partition[i].getPrePtr());

			 DPRINTF(ACA, "GTR = %d , preGTR = %d \n", getGTR(), getPreGTR());
			 */

			for (int i = 0; i < PARTS; i++) {
				partition[i].setPrePtr(partition[i].getPtr());
			}

			//preGTR = gTR;

		}
	}

//IPDS
	void insertionPolicy(uint8_t priority, int64_t set, int64_t index);
	void highPriority(int64_t set, int64_t index);
	void frequencyPriority(int64_t set, int64_t index);
	void halfOfAverage(int64_t set);
	void average(int64_t set);
	void demote(int64_t set);
	int64_t leftSideSelection(int64_t set);
	int64_t randomSelection(int64_t set);

	void executeInsertion(int64_t set, int64_t index);
	void executePromotion(int64_t set, int64_t index);
	void executeDemotion(int64_t set);
	int64_t executeSelection(int64_t set);

};

#endif // __MEM_RUBY_STRUCTURES_ADPGPOLICY_HH__
