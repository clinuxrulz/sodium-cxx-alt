#ifndef __SODIUM_UNIT_H__
#define __SODIUM_UNIT_H__

typedef struct Unit {
    bool operator==(const Unit& rhs) {
        return true;
    }
} Unit;

#endif // __SODIUM_UNIT_H__
